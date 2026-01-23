#include "io/PlyReader.hpp"
#include "io/PlyWriter.hpp"
#include "utils/Timer.hpp"
#include "codec/MortonOrder.hpp"
#include "codec/Transform.hpp"
#include "codec/Quantization.hpp"
#include <cstdint>

const std::string ROOT_PATH = "G:\\code\\cpp\\gaussian-stream\\";
const std::string INPUT_PATH = "G:\\code\\icip2026\\datasets\\coffee_martini_origin_ply_\\";
const std::string ENCODED_PLY_PATH = ROOT_PATH + "output\\encoded-ply\\";
const std::string ENCODED_DRC_PATH = ROOT_PATH + "output\\encoded-drc\\";
const std::string DECODED_PLY_PATH = ROOT_PATH + "output\\decoded-ply\\";
const std::string DRACO_ENCODER = "G:\\code\\cpp\\gaussian-stream\\draco_encoder.exe";
const std::string DRACO_DECODER = "G:\\code\\cpp\\gaussian-stream\\draco_decoder.exe";

void dracoEncode(const std::string& inputFile, const std::string& outputFile, int cl = 10, int qp = 16) {
    std::string command = DRACO_ENCODER + " -i " + inputFile + " -o " + outputFile + " -qp " + std::to_string(qp) + " -cl " + std::to_string(cl);
    auto res = std::system(command.c_str());
    if(res != 0) {
        SPDLOG_ERROR("Draco encoding failed for file: {}", inputFile);
    }
}
void dracoDecode(const std::string& inputFile, const std::string& outputFile) {
    std::string command = DRACO_DECODER + " -i " + inputFile + " -o " + outputFile;
    auto res = std::system(command.c_str());
    if(res != 0) {
        SPDLOG_ERROR("Draco decoding failed for file: {}", inputFile);
    }
}

int main(int argc, char **argv) {

    auto files = FileTools::findFilesMatchingPattern(INPUT_PATH, R"(.*\.ply)");
    SPDLOG_INFO("Found {} PLY files in input directory.", files.size());
    for(const auto& filePath : files) {

        auto data = PlyReader::readDataFromFile(filePath.string());
        auto positions = data.getTypedProperties<float>("vertex", {"x", "y", "z"});
        auto attributes = data.getTypedProperties<float>("vertex", 
            {"f_dc_0", "f_dc_1", "f_dc_2", 
            "opacity","scale_0", "scale_1", "scale_2",
            "rot_0", "rot_1", "rot_2", "rot_3"});
        // 编码的流程
        auto bbox = BoundingBox3D::calculateFromPoints(positions);

        // 变换量化
        Transform::logTransformInPlace(positions, bbox);
        auto quantizedPositions = Quantization::quantizePositionWithBBox<uint16_t, float, 16>(positions, bbox);
        
        // 计算莫顿序
        auto indices = MortonEncoder::calculateMortonIndices<uint64_t>(
            Quantization::castVectors<uint32_t>(quantizedPositions)
        );

        // 对量化后的数据重排莫顿序
        for(auto& position : quantizedPositions){
            Transform::sortInPlaceWithIndices(position, indices);
        }
        
        for(auto& attr : attributes){
            Transform::sortInPlaceWithIndices(attr, indices);
        }

        // 写入几何信息的PLY码流
        auto encodedPlyFilePath = ENCODED_PLY_PATH + filePath.filename().string();
        auto quantizedPositionsFP32 = Quantization::castVectors<float>(quantizedPositions);
        data.setProperties("vertex", {"x", "y", "z"}, quantizedPositionsFP32);
        PlyWriter::writeDataToFileWithPropertyMasks(encodedPlyFilePath, data, {"x", "y", "z"});

        // 使用Draco压缩几何信息
        auto dracoEncodedFilePath = ENCODED_DRC_PATH + filePath.stem().string() + ".drc";
        dracoEncode(encodedPlyFilePath, dracoEncodedFilePath, 10, 14);
        // 使用Draco解压缩几何信息
        auto dracoDecodedPlyFilePath = DECODED_PLY_PATH + filePath.filename().string();
        dracoDecode(dracoEncodedFilePath, dracoDecodedPlyFilePath);

        // 读取解码后的几何信息
        auto decodedData = PlyReader::readDataFromFile(dracoDecodedPlyFilePath);
        auto decodedQuantizedPositions = decodedData.getTypedProperties<float>("vertex", {"x", "y", "z"});
        // auto decodedQuantizedPositions = quantizedPositionsFP32; // 使用原始的量化数据进行反量化反变换测试

        // 计算解码后数据的莫顿序
        auto decodedIndices = MortonEncoder::calculateMortonIndices<uint64_t>(
            Quantization::castVectors<uint32_t>(decodedQuantizedPositions)
        );
        // 重排序
        for(auto& position : decodedQuantizedPositions){
            Transform::sortInPlaceWithIndices(position, decodedIndices);
        }
        
        // 反量化反变换
        auto dequantizedPositions = Quantization::dequantizePositionWithBBox<float, float, 16>(decodedQuantizedPositions, bbox);
        Transform::inverseLogTransformInPlace(dequantizedPositions, bbox);
        
        data.setProperties("vertex", {"x", "y", "z"}, dequantizedPositions);
        data.setProperties("vertex", 
            {"f_dc_0", "f_dc_1", "f_dc_2", 
            "opacity","scale_0", "scale_1", "scale_2",
            "rot_0", "rot_1", "rot_2", "rot_3"}, attributes);
        
        // 保存最终解码结果
        auto finalDecodedPlyFilePath = DECODED_PLY_PATH + filePath.filename().string();
        PlyWriter::writeDataToFile(finalDecodedPlyFilePath, data);
        // PlyWriter::writeDataToFileWithPropertyMasks(finalDecodedPlyFilePath, data, {"x", "y", "z"});
    }

    return 0;
}

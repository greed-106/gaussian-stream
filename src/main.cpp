#include "io/PlyReader.hpp"
#include "io/PlyWriter.hpp"
#include "utils/Timer.hpp"
#include "codec/MortonOrder.hpp"
#include "codec/Transform.hpp"
#include "codec/Quantization.hpp"

int main(int argc, char **argv) {

    auto data = PlyReader::readDataFromFile("G:\\code\\cpp\\gaussian-stream\\test.ply");

    auto positions = data.getTypedProperties<float>("vertex", {"x", "y", "z"});

    TICK(morton);
    auto bbox = BoundingBox3D::calculateFromPoints(positions);
    auto indices = MortonEncoder::calculateMortonIndices<uint64_t>(
        Quantization::quantizePositionWithBBox<uint32_t, float, mortonnd::MortonNDBmi<3, uint64_t>::FieldBits>(positions, bbox)
    );
    Transform::logTransformInPlace(positions, bbox);
    auto quantizedPositions = Quantization::quantizePositionWithBBox<uint16_t, float, 16>(positions, bbox);
    TOCK(morton);

    std::vector<std::vector<float>> dequantizedPositions(3, std::vector<float>(positions[0].size()));
    for(const auto& axis : {0,1,2}) {
        for(size_t i = 0; i < positions[0].size(); ++i) {
            dequantizedPositions[axis][i] = static_cast<float>(quantizedPositions[axis][i]);
        }
    }
    data.setProperties("vertex", {"x", "y", "z"}, dequantizedPositions);
    PlyWriter::writeDataToFile("G:\\code\\cpp\\gaussian-stream\\out-withoutlog.ply", data, PlyFormat::BINARY_LITTLE_ENDIAN);

    return 0;
}

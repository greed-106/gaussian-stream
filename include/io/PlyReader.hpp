#pragma once

#include "FileTools.hpp"
#include "PlySchema.hpp"
#include "PlyData.hpp"
#include "PlyFormat.hpp"
#include <functional>
#include <tuple>
#include <mio/mmap.hpp>

// 基于内存映射的streambuf，将mmap区域包装为std::istream可用的缓冲区
class MmapStreambuf : public std::streambuf {
public:
    MmapStreambuf(const char* data, size_t size) {
        char* begin = const_cast<char*>(data);
        char* end = begin + size;
        setg(begin, begin, end);
    }
};

class PlyReader{

private:
    static std::tuple<std::vector<ElementSchema>, PlyFormat> parseHeader(std::istream& file){
        std::string line;
        std::vector<ElementSchema> elements;
        PlyFormat format = PlyFormat::ASCII; // 默认ASCII格式

        // 读取Header
        while (std::getline(file, line)) {
            if (line == "end_header") {
                break;
            }
            std::istringstream iss(line);
            std::string token;
            iss >> token;

            if (token == "format") {

                std::string formatType;
                iss >> formatType;
                if(formatType == "ascii") {
                    format = PlyFormat::ASCII;
                } else if(formatType == "binary_little_endian") {
                    format = PlyFormat::BINARY_LITTLE_ENDIAN;
                } else {
                    SPDLOG_ERROR("Unsupported PLY format: {}", formatType);
                    throw std::runtime_error("Unsupported PLY format: " + formatType);
                }

            } else if (token == "element") {

                std::string elementName;
                int elementCount;
                iss >> elementName >> elementCount;
                elements.push_back({elementName, elementCount});

            } else if (token == "property") {

                std::string propertyType, propertyName;
                iss >> propertyType >> propertyName;
                const auto& currentElementName = elements.back().getNameRef();

                if(RegisteredSchema::isSchemaRegistered(currentElementName, propertyName, propertyType)) {
                    elements.back().addProperty(currentElementName, propertyName, propertyType);
                } else {
                    SPDLOG_ERROR("Unregistered schema: element {}, property {}, type {}", currentElementName, propertyName, propertyType);
                    throw std::runtime_error("Unregistered schema: element " + currentElementName + ", property " + propertyName + ", type " + propertyType);
                }
            }
        }

        return {elements, format};
    }

    template<PlyFormat Format>
    static std::function<void(std::istream&, std::vector<PropertyValue>&)> createPropertyParser(PropertyStorageType storageType) {
        if constexpr (Format == PlyFormat::ASCII) {
            switch (storageType) {
                case PropertyStorageType::INT32:
                    return [](std::istream& is, std::vector<PropertyValue>& output) {
                        int32_t value;
                        is >> value;
                        output.emplace_back(value);
                    };
                case PropertyStorageType::FLOAT32:
                    return [](std::istream& is, std::vector<PropertyValue>& output) {
                        float value;
                        is >> value;
                        output.emplace_back(value);
                    };
                default:
                    throw std::runtime_error("Unsupported storage type for ASCII format");
            }
        }
        else if constexpr (Format == PlyFormat::BINARY_LITTLE_ENDIAN) {
            switch (storageType) {
                case PropertyStorageType::INT32:
                    return [](std::istream& is, std::vector<PropertyValue>& output) {
                        int32_t value;
                        is.read(reinterpret_cast<char*>(&value), sizeof(value));
                        output.emplace_back(value);
                    };
                case PropertyStorageType::FLOAT32:
                    return [](std::istream& is, std::vector<PropertyValue>& output) {
                        float value;
                        is.read(reinterpret_cast<char*>(&value), sizeof(value));
                        output.emplace_back(value);
                    };
                default:
                    throw std::runtime_error("Unsupported storage type for binary format");
            }
        }
        else {
            static_assert(Format == PlyFormat::ASCII || Format == PlyFormat::BINARY_LITTLE_ENDIAN, 
                        "Unsupported PlyFormat");
        }
    }

    static std::vector<std::function<void(std::istream&, std::vector<PropertyValue>&)>> 
        buildAllPropertyParsers(const ElementSchema& schema, PlyFormat format) {

        std::vector<std::function<void(std::istream&, std::vector<PropertyValue>&)>> parsers;
        auto propertyStorageTypes = schema.getPropertyStorageTypes();
        for(int i = 0; i < schema.getNumberOfProperties(); ++i) {
            switch (format) {
                case PlyFormat::ASCII:
                    parsers.push_back(createPropertyParser<PlyFormat::ASCII>(propertyStorageTypes[i]));
                    break;
                case PlyFormat::BINARY_LITTLE_ENDIAN:
                    parsers.push_back(createPropertyParser<PlyFormat::BINARY_LITTLE_ENDIAN>(propertyStorageTypes[i]));
                    break;
                default:
                    SPDLOG_ERROR("Unsupported PLY format during parser building");
                    throw std::runtime_error("Unsupported PLY format during parser building");
            }
        }

        return parsers;
    }

    static std::vector<std::vector<PropertyValue>> parseElement(std::istream& file, const ElementSchema& schema, PlyFormat format) {
        
        std::vector<std::vector<PropertyValue>> elementData(schema.getNumberOfProperties());
        for(int i = 0; i < schema.getNumberOfProperties(); ++i) {
            elementData[i].reserve(schema.getCount());
        }
        auto parsers = buildAllPropertyParsers(schema, format);
        std::string line;

        switch (format) {
            case PlyFormat::ASCII:
                for(int i = 0; i < schema.getCount(); ++i) {
                    std::getline(file, line);
                    std::istringstream iss(line);
                    for(int j = 0; j < parsers.size(); ++j) {
                        parsers[j](iss, elementData[j]);
                    }
                }
                break;
            case PlyFormat::BINARY_LITTLE_ENDIAN:
                for(int i = 0; i < schema.getCount(); ++i) {
                    for(int j = 0; j < parsers.size(); ++j) {
                        parsers[j](file, elementData[j]);
                    }
                }
                break;
            default:
                SPDLOG_ERROR("Unsupported PLY format during element parsing");
                throw std::runtime_error("Unsupported PLY format during element parsing");
        }

        return elementData;
    }

    static PlyData parseBody(std::istream& file, const std::vector<ElementSchema>& schemas, PlyFormat format){
        PlyData plyData;

        for (const auto& schema : schemas) {
            auto elementData = parseElement(file, schema, format);
            const auto& propertyNames = schema.getPropertyNames();
            for (size_t i = 0; i < propertyNames.size(); ++i) {
                plyData.setProperty(schema.getNameRef(), propertyNames[i], std::move(elementData[i]));
            }
        }

        return plyData;
    }

public:
    static PlyData readDataFromFile(const std::string& filename){
        FileTools::checkFileExists(filename);
        
        // 使用mmap映射文件到内存
        std::error_code error;
        mio::mmap_source mmap = mio::make_mmap_source(filename, error);
        if (error) {
            SPDLOG_ERROR("Failed to mmap file: {}, error: {}", filename, error.message());
            throw std::runtime_error("Failed to mmap file: " + filename + ", error: " + error.message());
        }

        // 将mmap包装为istream
        MmapStreambuf streambuf(mmap.data(), mmap.size());
        std::istream file(&streambuf);

        auto [schemas, format] = parseHeader(file);

        auto plyData = parseBody(file, schemas, format);
        plyData.setSchemas(std::move(schemas));

        return plyData;
    }
};
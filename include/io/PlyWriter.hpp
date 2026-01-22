#pragma once

#include "FileTools.hpp"
#include "PlySchema.hpp"
#include "PlyData.hpp"
#include <fstream>
#include <functional>
#include <unordered_set>
#include <spdlog/spdlog.h>

class PlyWriter {

private:
    static void writeHeader(std::ofstream& file, const PlyData& plyData, PlyFormat format) {
        file << "ply\n";
        
        switch (format) {
            case PlyFormat::ASCII:
                file << "format ascii 1.0\n";
                break;
            case PlyFormat::BINARY_LITTLE_ENDIAN:
                file << "format binary_little_endian 1.0\n";
                break;
            default:
                SPDLOG_ERROR("Unsupported PLY format for writing");
                throw std::runtime_error("Unsupported PLY format for writing");
        }

        for (const auto& schema : plyData.schemas) {
            file << "element " << schema.getNameRef() << " " << schema.getCount() << "\n";
            for (const auto& property : schema.properties) {
                file << "property " << property.currentHeaderType << " " << property.propertyName << "\n";
            }
        }

        file << "end_header\n";
    }

    template<PlyFormat Format>
    static std::function<void(std::ostream&, const PropertyValue&)> createPropertyWriter(PropertyStorageType storageType) {
        if constexpr (Format == PlyFormat::ASCII) {
            switch (storageType) {
                case PropertyStorageType::INT32:
                    return [](std::ostream& os, const PropertyValue& value) {
                        os << std::get<int32_t>(value);
                    };
                case PropertyStorageType::FLOAT32:
                    return [](std::ostream& os, const PropertyValue& value) {
                        os << std::get<float>(value);
                    };
                default:
                    throw std::runtime_error("Unsupported storage type for ASCII format");
            }
        }
        else if constexpr (Format == PlyFormat::BINARY_LITTLE_ENDIAN) {
            switch (storageType) {
                case PropertyStorageType::INT32:
                    return [](std::ostream& os, const PropertyValue& value) {
                        int32_t v = std::get<int32_t>(value);
                        os.write(reinterpret_cast<const char*>(&v), sizeof(v));
                    };
                case PropertyStorageType::FLOAT32:
                    return [](std::ostream& os, const PropertyValue& value) {
                        float v = std::get<float>(value);
                        os.write(reinterpret_cast<const char*>(&v), sizeof(v));
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

    static std::vector<std::function<void(std::ostream&, const PropertyValue&)>>
        buildAllPropertyWriters(const ElementSchema& schema, PlyFormat format) {

        std::vector<std::function<void(std::ostream&, const PropertyValue&)>> writers;
        auto propertyStorageTypes = schema.getPropertyStorageTypes();
        for (int i = 0; i < schema.getNumberOfProperties(); ++i) {
            switch (format) {
                case PlyFormat::ASCII:
                    writers.push_back(createPropertyWriter<PlyFormat::ASCII>(propertyStorageTypes[i]));
                    break;
                case PlyFormat::BINARY_LITTLE_ENDIAN:
                    writers.push_back(createPropertyWriter<PlyFormat::BINARY_LITTLE_ENDIAN>(propertyStorageTypes[i]));
                    break;
                default:
                    SPDLOG_ERROR("Unsupported PLY format during writer building");
                    throw std::runtime_error("Unsupported PLY format during writer building");
            }
        }

        return writers;
    }

    static void writeElement(std::ofstream& file, const PlyData& plyData, const ElementSchema& schema, PlyFormat format) {
        auto writers = buildAllPropertyWriters(schema, format);
        const auto& propertyNames = schema.getPropertyNames();
        const auto& elementName = schema.getNameRef();

        // 获取所有属性数据的引用
        std::vector<const std::vector<PropertyValue>*> propertyDataRefs;
        for (const auto& propName : propertyNames) {
            propertyDataRefs.push_back(&plyData.getPropertyRefWithName(elementName, propName));
        }

        switch (format) {
            case PlyFormat::ASCII:
                for (int i = 0; i < schema.getCount(); ++i) {
                    for (size_t j = 0; j < writers.size(); ++j) {
                        if (j > 0) file << " ";
                        writers[j](file, (*propertyDataRefs[j])[i]);
                    }
                    file << "\n";
                }
                break;
            case PlyFormat::BINARY_LITTLE_ENDIAN:
                for (int i = 0; i < schema.getCount(); ++i) {
                    for (size_t j = 0; j < writers.size(); ++j) {
                        writers[j](file, (*propertyDataRefs[j])[i]);
                    }
                }
                break;
            default:
                SPDLOG_ERROR("Unsupported PLY format during element writing");
                throw std::runtime_error("Unsupported PLY format during element writing");
        }
    }

    static void writeBody(std::ofstream& file, const PlyData& plyData, PlyFormat format) {
        for (const auto& schema : plyData.schemas) {
            writeElement(file, plyData, schema, format);
        }
    }

    static void writeHeaderWithPropertyMasks(std::ofstream& file, const PlyData& plyData, PlyFormat format, const std::vector<std::string>& propertyMasks) {
        file << "ply\n";
        
        switch (format) {
            case PlyFormat::ASCII:
                file << "format ascii 1.0\n";
                break;
            case PlyFormat::BINARY_LITTLE_ENDIAN:
                file << "format binary_little_endian 1.0\n";
                break;
            default:
                SPDLOG_ERROR("Unsupported PLY format for writing");
                throw std::runtime_error("Unsupported PLY format for writing");
        }

        // 将propertyMasks转换为unordered_set以便快速查找
        std::unordered_set<std::string> maskSet(propertyMasks.begin(), propertyMasks.end());

        for (const auto& schema : plyData.schemas) {
            // 统计有多少属性在mask中
            int validPropertyCount = 0;
            for (const auto& property : schema.properties) {
                if (maskSet.find(property.propertyName) != maskSet.end()) {
                    validPropertyCount++;
                }
            }

            // 只有当至少有一个属性在mask中时才写入element
            if (validPropertyCount > 0) {
                file << "element " << schema.getNameRef() << " " << schema.getCount() << "\n";
                for (const auto& property : schema.properties) {
                    if (maskSet.find(property.propertyName) != maskSet.end()) {
                        file << "property " << property.currentHeaderType << " " << property.propertyName << "\n";
                    }
                }
            }
        }

        file << "end_header\n";
    }

    static void writeElementWithPropertyMasks(std::ofstream& file, const PlyData& plyData, const ElementSchema& schema, PlyFormat format, const std::vector<std::string>& propertyMasks) {
        // 将propertyMasks转换为unordered_set以便快速查找
        std::unordered_set<std::string> maskSet(propertyMasks.begin(), propertyMasks.end());

        const auto& propertyNames = schema.getPropertyNames();
        const auto& elementName = schema.getNameRef();

        // 筛选出在mask中的属性
        std::vector<int> validPropertyIndices;
        std::vector<std::function<void(std::ostream&, const PropertyValue&)>> writers;
        std::vector<const std::vector<PropertyValue>*> propertyDataRefs;

        auto propertyStorageTypes = schema.getPropertyStorageTypes();
        for (size_t i = 0; i < propertyNames.size(); ++i) {
            if (maskSet.find(propertyNames[i]) != maskSet.end()) {
                validPropertyIndices.push_back(i);
                
                // 创建对应的writer
                switch (format) {
                    case PlyFormat::ASCII:
                        writers.push_back(createPropertyWriter<PlyFormat::ASCII>(propertyStorageTypes[i]));
                        break;
                    case PlyFormat::BINARY_LITTLE_ENDIAN:
                        writers.push_back(createPropertyWriter<PlyFormat::BINARY_LITTLE_ENDIAN>(propertyStorageTypes[i]));
                        break;
                    default:
                        SPDLOG_ERROR("Unsupported PLY format during writer building");
                        throw std::runtime_error("Unsupported PLY format during writer building");
                }

                propertyDataRefs.push_back(&plyData.getPropertyRefWithName(elementName, propertyNames[i]));
            }
        }

        // 如果没有有效属性，直接返回
        if (validPropertyIndices.empty()) {
            return;
        }

        switch (format) {
            case PlyFormat::ASCII:
                for (int i = 0; i < schema.getCount(); ++i) {
                    for (size_t j = 0; j < writers.size(); ++j) {
                        if (j > 0) file << " ";
                        writers[j](file, (*propertyDataRefs[j])[i]);
                    }
                    file << "\n";
                }
                break;
            case PlyFormat::BINARY_LITTLE_ENDIAN:
                for (int i = 0; i < schema.getCount(); ++i) {
                    for (size_t j = 0; j < writers.size(); ++j) {
                        writers[j](file, (*propertyDataRefs[j])[i]);
                    }
                }
                break;
            default:
                SPDLOG_ERROR("Unsupported PLY format during element writing");
                throw std::runtime_error("Unsupported PLY format during element writing");
        }
    }

    static void writeBodyWithPropertyMasks(std::ofstream& file, const PlyData& plyData, PlyFormat format, const std::vector<std::string>& propertyMasks) {
        for (const auto& schema : plyData.schemas) {
            writeElementWithPropertyMasks(file, plyData, schema, format, propertyMasks);
        }
    }

public:
    static void writeDataToFile(const std::string& filename, const PlyData& plyData, PlyFormat format = PlyFormat::BINARY_LITTLE_ENDIAN) {
        FileTools::checkAndCreateDir(filename);

        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            SPDLOG_ERROR("Failed to open file for writing: {}", filename);
            throw std::runtime_error("Failed to open file for writing: " + filename);
        }

        writeHeader(file, plyData, format);
        writeBody(file, plyData, format);

        file.close();
    }

    static void writeDataToFileWithPropertyMasks(const std::string& filename, const PlyData& plyData, const std::vector<std::string>& propertyMasks, PlyFormat format = PlyFormat::BINARY_LITTLE_ENDIAN) {
        FileTools::checkAndCreateDir(filename);

        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            SPDLOG_ERROR("Failed to open file for writing: {}", filename);
            throw std::runtime_error("Failed to open file for writing: " + filename);
        }

        writeHeaderWithPropertyMasks(file, plyData, format, propertyMasks);
        writeBodyWithPropertyMasks(file, plyData, format, propertyMasks);

        file.close();
    }
};

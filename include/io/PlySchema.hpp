#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_set>
#include <stdexcept>
#include <spdlog/spdlog.h>

enum class PlyFormat {
    ASCII,
    BINARY_LITTLE_ENDIAN
};

enum class PropertyStorageType {
    INT32,   // 存储为 int32_t
    FLOAT32   // 存储为 float
};

class PropertySchema {
public:
    std::string elementName;   // 元素名称
    std::string propertyName;  // 属性名称
    std::vector<std::string> availableHeaderTypes;   // PLY的Header信息中定义的类型列表
    std::string currentHeaderType;      // 当前使用的Header类型
    PropertyStorageType storageType;  // 实际在内存中的存储类型

    void setCurrentHeaderType(const std::string& headerType) {
        currentHeaderType = headerType;
    }
};

class RegisteredSchema{
private:
    inline static std::vector<PropertySchema> schemas = {};
public:
    static void registerSchema(const std::string elementName,
                        const std::string propertyName,
                        const std::vector<std::string>& availableHeaderTypes,
                        PropertyStorageType storageType) {
        if(availableHeaderTypes.empty()) {
            SPDLOG_ERROR("availableHeaderTypes cannot be empty");
            throw std::runtime_error("availableHeaderTypes cannot be empty");
        }
        PropertySchema schema = {
            elementName,
            propertyName,
            availableHeaderTypes,
            availableHeaderTypes.front(),
            storageType
        };
        schemas.push_back(schema);
    }

    static const PropertySchema getSchemaWithName(const std::string& elementName, const std::string& propertyName) {
        for (const auto& schema : schemas) {
            if (schema.elementName == elementName && schema.propertyName == propertyName) {
                return schema;
            }
        }
        SPDLOG_ERROR("Schema not found for element: {}, property: {}", elementName, propertyName);
        throw std::runtime_error("Schema not found for element: " + elementName + ", property: " + propertyName);
    }

    static bool isSchemaRegistered(const std::string& elementName, const std::string& propertyName, const std::string& headerType) {
        try{
            const PropertySchema& schema = getSchemaWithName(elementName, propertyName);
            for (const auto& type : schema.availableHeaderTypes) {
                if (type == headerType) {
                    return true;
                }
            }
        } catch (const std::runtime_error&) {
            return false;
        }
        return false;
    }
};

class ElementSchema {
private:
    std::unordered_set<std::string> propertyNamesSet;
    std::vector<std::string> propertyNamesVec;
public:
    std::vector<PropertySchema> properties;
    std::string name;
    int32_t count;

    bool propertyExists(const std::string& propertyName) const {
        return propertyNamesSet.find(propertyName) != propertyNamesSet.end();
    }

    void addProperty(const std::string& elementName, const std::string& propertyName, const std::string& headerType) {
        if(this->name != elementName){
            SPDLOG_ERROR("Element name mismatch when adding property schema: expected {}, got {}", this->name, elementName);
            throw std::runtime_error("Element name mismatch when adding property schema: expected " + this->name + ", got " + elementName);
        }
        auto schema = RegisteredSchema::getSchemaWithName(elementName, propertyName);
        schema.setCurrentHeaderType(headerType);
        if(propertyExists(propertyName)){
            SPDLOG_ERROR("Property {} already exists in element {}", propertyName, elementName);
            throw std::runtime_error("Property " + propertyName + " already exists in element " + elementName);
        }
        properties.push_back(schema);
        propertyNamesSet.insert(propertyName);
        propertyNamesVec.push_back(propertyName);
    }

    const std::string& getNameRef() const {
        return name;
    }

    const std::vector<std::string>& getPropertyNames() const {
        return propertyNamesVec;
    }

    const int getCount() const {
        return count;
    }

    const int getNumberOfProperties() const {
        return static_cast<int>(properties.size());
    }

    const std::vector<PropertyStorageType> getPropertyStorageTypes() const {
        std::vector<PropertyStorageType> storageTypes(properties.size());
        for (size_t i = 0; i < properties.size(); ++i) {
            storageTypes[i] = properties[i].storageType;
        }
        return storageTypes;
    }


    ElementSchema(const std::string& elementName, int32_t elementCount): name(elementName), count(elementCount) {}
};
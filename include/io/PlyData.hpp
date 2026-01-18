#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <variant>
#include <vector>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include "PlySchema.hpp"

using PropertyValue = std::variant<int32_t, float>;

class Element{
public:
    std::string name;
    std::unordered_map<std::string, std::vector<PropertyValue>> properties;

    const std::vector<PropertyValue>& getPropertyRefWithName(const std::string& propertyName) const {
        auto propIt = properties.find(propertyName);
        if (propIt == properties.end()) {
            SPDLOG_ERROR("Property not found: {} in element {}", propertyName, name);
            throw std::runtime_error("Property not found: " + propertyName + " in element " + name);
        }
        return propIt->second;
    }

    void setProperty(const std::string& propertyName, std::vector<PropertyValue>&& values) {
        properties[propertyName] = values;
    }

    void setName(const std::string& elementName) {
        name = elementName;
    }
};

class PlyData {
public:
    std::unordered_map<std::string, Element> elements;
    std::vector<ElementSchema> schemas;

    // 安全访问，获得常引用
    const std::vector<PropertyValue>& getPropertyRefWithName(const std::string& elementName, const std::string& propertyName) const {
        auto elemIt = elements.find(elementName);
        if (elemIt == elements.end()) {
            throw std::runtime_error("Element not found: " + elementName);
        }
        
        return elemIt->second.getPropertyRefWithName(propertyName);
    }

    template <typename T>
    const std::vector<std::vector<T>> getTypedProperties(const std::string& elementName, const std::vector<std::string>& propertyNames) const {
        auto elemIt = elements.find(elementName);
        if (elemIt == elements.end()) {
            throw std::runtime_error("Element not found: " + elementName);
        }

        std::vector<std::vector<T>> result;
        result.reserve(propertyNames.size());

        for (const auto& propName : propertyNames) {
            const auto& propValues = elemIt->second.getPropertyRefWithName(propName);
            std::vector<T> typedValues;
            typedValues.reserve(propValues.size());

            for (const auto& val : propValues) {
                typedValues.push_back(std::get<T>(val));
            }

            result.push_back(typedValues);
        }

        return result;
    }

    void setProperty(const std::string& elementName, const std::string& propertyName, std::vector<PropertyValue>&& values) {
        elements[elementName].setName(elementName);
        elements[elementName].setProperty(propertyName, std::move(values));
    }

    template <typename T>
    void setProperty(const std::string& elementName,
                    const std::string& propertyName,
                    const std::vector<T>& values) {

        std::vector<PropertyValue> converted;
        converted.reserve(values.size());
        for (const T& val : values) {
            converted.emplace_back(val);
        }
        setProperty(elementName, propertyName, std::move(converted));
    }

    template<typename T>
    void setProperties(const std::string& elementName,
                       const std::vector<std::string>& propertyNames,
                       const std::vector<std::vector<T>>& values) {
        if (propertyNames.size() != values.size()) {
            throw std::runtime_error("Property names and values size mismatch.");
        }
        for (size_t i = 0; i < propertyNames.size(); ++i) {
            setProperty(elementName, propertyNames[i], values[i]);
        }
    }

    void setSchemas(std::vector<ElementSchema>&& schemasVec) {
        schemas = std::move(schemasVec);
    }

    void printElementInfos() const {
        for (const auto& [elementName, element] : elements) {
            SPDLOG_INFO("Element: {}", elementName);
            for (const auto& [propertyName, values] : element.properties) {
                SPDLOG_INFO("  Property: {} ({} values)", propertyName, values.size());
            }
        }
    }
};
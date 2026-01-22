#pragma once

#include "Quantization.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

class Transform{
public:
    template<typename T>
    static void logTransformInPlace(std::vector<std::vector<T>>& positions, BoundingBox3D& bbox) {
        for(auto& axisPositions : positions) {
            std::transform(axisPositions.begin(), axisPositions.end(), axisPositions.begin(),
                [](T val) {
                    return std::signbit(val) ? -std::log(-val + 1.0f) : std::log(val + 1.0f);
                }
            );
        }
        
        // 更新边界框
        for(auto& val : bbox.data) {
            val = std::signbit(val) ? -std::log(-val + 1.0f) : std::log(val + 1.0f);
        }
    }

    template<typename T>
    static void inverseLogTransformInPlace(std::vector<std::vector<T>>& positions, BoundingBox3D& bbox) {
        for (auto& axisPositions : positions) {
            std::transform(axisPositions.begin(), axisPositions.end(), axisPositions.begin(),
                [](T val) {
                    if (val >= T{0}) {
                        return static_cast<T>(std::exp(val) - 1.0);
                    } else {
                        return static_cast<T>(-(std::exp(-val) - 1.0));
                    }
                }
            );
        }

        // 逆变换边界框
        for (auto& val : bbox.data) {
            if (val >= T{0}) {
                val = static_cast<T>(std::exp(static_cast<double>(val)) - 1.0);
            } else {
                val = static_cast<T>(-(std::exp(static_cast<double>(-val)) - 1.0));
            }
        }
    }

    template<typename PropertyType, typename IndicesType = uint64_t>
    static void sortInPlaceWithIndices(std::vector<PropertyType>& property, const std::vector<IndicesType>& indices) {
        size_t n = property.size();
        std::vector<PropertyType> sortedProperty(n);

        for(size_t i = 0; i < n; ++i) {
            sortedProperty[i] = property[indices[i]];
        }

        property = std::move(sortedProperty);
    }
};
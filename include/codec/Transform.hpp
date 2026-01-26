#pragma once

#include "Quantization.hpp"
#include <cstdint>
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
    // 将sh0转换为RGB颜色
    template<typename OutType = uint8_t, int ColorDepth = 8>
    static std::vector<std::vector<OutType>> sh0ToPlanarRGB(std::vector<std::vector<float>>& sh0) {
        // 判断OutType是否能够表示ColorDepth位的颜色值
        static_assert(std::is_integral<OutType>::value, "OutType must be an integral type");
        constexpr OutType maxColorValue = (1 << ColorDepth) - 1;
        static_assert(std::numeric_limits<OutType>::max() >= maxColorValue, "OutType cannot represent the specified ColorDepth");

        if(sh0.size() != 3) {
            throw std::runtime_error("sh0ToRGB requires 3 channels in sh0 data");
        }
        const size_t numPoints = sh0[0].size();
        const float SH0_FACTOR = 0.28209479177387814f; // sqrt(1/(4*pi))
        std::vector<std::vector<OutType>> colors(3, std::vector<OutType>(numPoints));
        for (size_t i = 0; i < numPoints; ++i) {
            OutType r = static_cast<OutType>((sh0[0][i] * SH0_FACTOR + 0.5f) * maxColorValue);
            OutType g = static_cast<OutType>((sh0[1][i] * SH0_FACTOR + 0.5f) * maxColorValue);
            OutType b = static_cast<OutType>((sh0[2][i] * SH0_FACTOR + 0.5f) * maxColorValue);

            colors[0][i] = r;
            colors[1][i] = g;
            colors[2][i] = b;
        }

        return colors;
    }

    template<typename OutType = uint8_t, int ColorDepth = 8>
    static std::vector<OutType> sh0ToPackedRGB(std::vector<std::vector<float>>& sh0) {
        // 判断OutType是否能够表示ColorDepth位的颜色值
        static_assert(std::is_integral<OutType>::value, "OutType must be an integral type");
        constexpr OutType maxColorValue = (1 << ColorDepth) - 1;
        static_assert(std::numeric_limits<OutType>::max() >= maxColorValue, "OutType cannot represent the specified ColorDepth");

        if(sh0.size() != 3) {
            throw std::runtime_error("sh0ToRGB requires 3 channels in sh0 data");
        }
        const size_t numPoints = sh0[0].size();
        const float SH0_FACTOR = 0.28209479177387814f; // sqrt(1/(4*pi))
        std::vector<OutType> colors(numPoints * 3);
        for (size_t i = 0; i < numPoints; ++i) {
            OutType r = static_cast<OutType>((sh0[0][i] * SH0_FACTOR + 0.5f) * maxColorValue);
            OutType g = static_cast<OutType>((sh0[1][i] * SH0_FACTOR + 0.5f) * maxColorValue);
            OutType b = static_cast<OutType>((sh0[2][i] * SH0_FACTOR + 0.5f) * maxColorValue);

            colors[i * 3 + 0] = r;
            colors[i * 3 + 1] = g;
            colors[i * 3 + 2] = b;
        }

        return colors;
    }

    template<typename InType = uint8_t, int ColorDepth = 8>
    static std::vector<std::vector<float>> packedRGBToSH0(const std::vector<InType>& packedRGB) {
        // 判断InType是否能够表示ColorDepth位的颜色值
        static_assert(std::is_integral<InType>::value, "InType must be an integral type");
        constexpr InType maxColorValue = (1 << ColorDepth) - 1;
        static_assert(std::numeric_limits<InType>::max() >= maxColorValue, "InType cannot represent the specified ColorDepth");

        const size_t numPoints = packedRGB.size() / 3;
        std::vector<std::vector<float>> sh0(3, std::vector<float>(numPoints));
        const float INV_SH0_FACTOR = 1.0f / 0.28209479177387814f; // sqrt(4*pi)
        for (size_t i = 0; i < numPoints; ++i) {
            float r = static_cast<float>(packedRGB[i * 3 + 0]) / maxColorValue;
            float g = static_cast<float>(packedRGB[i * 3 + 1]) / maxColorValue;
            float b = static_cast<float>(packedRGB[i * 3 + 2]) / maxColorValue;

            sh0[0][i] = (r - 0.5f) * INV_SH0_FACTOR;
            sh0[1][i] = (g - 0.5f) * INV_SH0_FACTOR;
            sh0[2][i] = (b - 0.5f) * INV_SH0_FACTOR;
        }

        return sh0;
    }
};
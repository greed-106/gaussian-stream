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
};
#pragma once

#include <cstdint>
#include <morton-nd/mortonND_BMI2.h>
#include <vector>
#include <stdexcept>
#include <numeric>

// Morton编码辅助类，基于morton-nd库实现
class MortonEncoder {
public:
    template<typename T = uint64_t>
    static std::vector<T> calculateMortonIndices(const std::vector<std::vector<uint32_t>>& coordinates) {
        const size_t Dimensions = 3;
        if(coordinates.size() != Dimensions) {
            throw std::runtime_error("Dimension mismatch in calculateMortonIndices");
        }

        size_t numPoints = coordinates[0].size();
        std::vector<T> mortonIndices(numPoints);

        std::iota(mortonIndices.begin(), mortonIndices.end(), 0);

        using MortonND = mortonnd::MortonNDBmi<Dimensions, T>;
        // auto [quantizedPositions, bbox] = Quantization::quantizePosition<uint32_t, float, MortonND::FieldBits>(coordinates);

        for(size_t i = 0; i < numPoints; ++i) {
            auto& x = coordinates[0][i];
            auto& y = coordinates[1][i];
            auto& z = coordinates[2][i];
            mortonIndices[i] = MortonND::Encode(z, y, x);
        }

        return mortonIndices;
    }
};
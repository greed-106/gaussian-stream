#pragma once

#include <cstdint>
#include <morton-nd/mortonND_BMI2.h>
#include <tuple>
#include <vector>
#include <stdexcept>
#include <numeric>
#include <algorithm>

// Morton编码辅助类，基于morton-nd库实现
class MortonEncoder {
public:
    template<typename IndicesType = uint64_t, typename CoordinateType = uint32_t>
    static std::vector<IndicesType> encode3DMortonIndices(const std::vector<std::vector<CoordinateType>>& coordinates) {
        const size_t Dimensions = 3;
        if(coordinates.size() != Dimensions) {
            throw std::runtime_error("Dimension mismatch in calculateMortonIndices");
        }

        size_t numPoints = coordinates[0].size();
        std::vector<IndicesType> mortonIndices(numPoints);
        std::vector<IndicesType> indices(numPoints);

        std::iota(indices.begin(), indices.end(), 0);

        using MortonND = mortonnd::MortonNDBmi<Dimensions, IndicesType>;
        // auto [quantizedPositions, bbox] = Quantization::quantizePosition<uint32_t, float, MortonND::FieldBits>(coordinates);

        for(size_t i = 0; i < numPoints; ++i) {
            auto& x = coordinates[0][i];
            auto& y = coordinates[1][i];
            auto& z = coordinates[2][i];
            mortonIndices[i] = MortonND::Encode(z, y, x);
        }

        std::sort(indices.begin(), indices.end(),
            [&mortonIndices](IndicesType a, IndicesType b) {
                return mortonIndices[a] < mortonIndices[b];
            }
        );

        return indices;
    }
};

class MortonDecoder{
public:
    template<typename T>
    static std::tuple<T, T> decode2DMortonIndex(T mortonIndex) {
        const size_t Dimensions = 2;
        using MortonND = mortonnd::MortonNDBmi<Dimensions, T>;

        return MortonND::Decode(mortonIndex);
    }

};
#pragma once

#include <vector>
#include <stdexcept>
#include <array>

class BoundingBox3D {
public:
    std::array<float, 6> data; // {minX, minY, minZ, maxX, maxY, maxZ}

    BoundingBox3D(float minX, float minY, float minZ,
                  float maxX, float maxY, float maxZ)
        : data{minX, minY, minZ, maxX, maxY, maxZ} {}

    float minX() const { return data[0]; }
    float minY() const { return data[1]; }
    float minZ() const { return data[2]; }
    float maxX() const { return data[3]; }
    float maxY() const { return data[4]; }
    float maxZ() const { return data[5]; }

    template<typename T>
    static BoundingBox3D calculateFromPoints(const std::vector<std::vector<T>>& points) {
        if (points.size() != 3 || points[0].empty()) {
            throw std::invalid_argument("Points must contain 3 non-empty coordinate arrays.");
        }

        const size_t n = points[0].size();
        if (points[1].size() != n || points[2].size() != n) {
            throw std::invalid_argument("All coordinate arrays must have the same length.");
        }

        float minX = points[0][0], maxX = points[0][0];
        float minY = points[1][0], maxY = points[1][0];
        float minZ = points[2][0], maxZ = points[2][0];

        for (size_t i = 1; i < n; ++i) {
            float x = points[0][i];
            float y = points[1][i];
            float z = points[2][i];

            if (x < minX) minX = x;
            else if (x > maxX) maxX = x;

            if (y < minY) minY = y;
            else if (y > maxY) maxY = y;

            if (z < minZ) minZ = z;
            else if (z > maxZ) maxZ = z;
        }

        return BoundingBox3D(minX, minY, minZ, maxX, maxY, maxZ);
    }
};

class Quantization {
public:

    template<typename OutType, typename InType, size_t BitsPerDimension>
    static std::vector<std::vector<OutType>> quantizePositionWithBBox(const std::vector<std::vector<InType>>& points, const BoundingBox3D& bbox) {
        if (points.size() != 3) {
            throw std::runtime_error("Only 3D points are supported for quantization.");
        }

        size_t numPoints = points[0].size();

        // auto bbox = BoundingBox3D::calculateFromPoints(points);
        float minX = bbox.minX();
        float minY = bbox.minY();
        float minZ = bbox.minZ();
        float maxX = bbox.maxX();
        float maxY = bbox.maxY();
        float maxZ = bbox.maxZ();

        uint32_t levels = 1 << BitsPerDimension;
        std::vector<std::vector<OutType>> quantizedData(3, std::vector<OutType>(numPoints));

        for (size_t i = 0; i < numPoints; ++i) {
            OutType qx = static_cast<OutType>(((points[0][i] - minX) / (maxX - minX)) * (levels - 1));
            OutType qy = static_cast<OutType>(((points[1][i] - minY) / (maxY - minY)) * (levels - 1));
            OutType qz = static_cast<OutType>(((points[2][i] - minZ) / (maxZ - minZ)) * (levels - 1));

            quantizedData[0][i] = qx;
            quantizedData[1][i] = qy;
            quantizedData[2][i] = qz;
        }

        return quantizedData;
    }

    template<typename OutType, typename InType, size_t BitsPerDimension>
    static std::vector<std::vector<OutType>> dequantizePositionWithBBox(const std::vector<std::vector<InType>>& quantizedPoints, const BoundingBox3D& bbox) {
        if (quantizedPoints.size() != 3) {
            throw std::runtime_error("Only 3D points are supported for dequantization.");
        }

        size_t numPoints = quantizedPoints[0].size();

        float minX = bbox.minX();
        float minY = bbox.minY();
        float minZ = bbox.minZ();
        float maxX = bbox.maxX();
        float maxY = bbox.maxY();
        float maxZ = bbox.maxZ();

        uint32_t levels = 1 << BitsPerDimension;
        std::vector<std::vector<OutType>> dequantizedData(3, std::vector<OutType>(numPoints));

        for (size_t i = 0; i < numPoints; ++i) {
            OutType x = static_cast<OutType>(quantizedPoints[0][i]) / (levels - 1) * (maxX - minX) + minX;
            OutType y = static_cast<OutType>(quantizedPoints[1][i]) / (levels - 1) * (maxY - minY) + minY;
            OutType z = static_cast<OutType>(quantizedPoints[2][i]) / (levels - 1) * (maxZ - minZ) + minZ;

            dequantizedData[0][i] = x;
            dequantizedData[1][i] = y;
            dequantizedData[2][i] = z;
        }

        return dequantizedData;
    }

    template<typename OutType, typename InType>
    static std::vector<OutType> castVector(const std::vector<InType>& input) {
        std::vector<OutType> output;
        output.reserve(input.size());
        for (const auto& val : input) {
            output.emplace_back(static_cast<OutType>(val));
        }
        return output;
    }
};
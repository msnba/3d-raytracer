#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "object.h"

class BVH
{
public:
    static constexpr int BVH_MAX_DEPTH = 20;
    static constexpr int BVH_LEAF_TRIANGLES = 6;
    static constexpr int SAH_NUM_BINS = 12;

    struct GPUNode
    {
        glm::vec4 min;
        glm::vec4 max;
        uint32_t left;
        uint32_t right;
        uint32_t triangleCount; // 0 = interior, >0 = leaf
        uint32_t pad;
    };

    std::vector<GPUNode> nodes_;
    std::vector<GPUTriangle> triangles_;

    BVH() = default;
    BVH(std::vector<GPUTriangle> &triangles);

private:
    void growNodeToInclude(GPUNode &node, const glm::vec3 &point);
    void growNodeToInclude(GPUNode &node, const GPUTriangle &triangle);
    void split(const uint32_t nodeIndex, const size_t depth = 0);

    float surfaceArea(const glm::vec3 &min, const glm::vec3 &max) const;
    float surfaceArea(const GPUNode &node) const;
    struct Bin
    {
        glm::vec4 min = glm::vec4(std::numeric_limits<float>::max());
        glm::vec4 max = glm::vec4(-std::numeric_limits<float>::max());
        int count = 0;
    };
    struct SplitResult
    {
        int axis;
        float pos;
        float cost;
    };
    void growBinToInclude(Bin &bin, const GPUTriangle &tri);
    SplitResult findBestSplit(const GPUNode &node);
};
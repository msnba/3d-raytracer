#ifndef BVH_H
#define BVH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <limits>
#include <vector>
#include "object.h"

struct BVH
{
    static constexpr size_t MAX_DEPTH = 20;
    static constexpr size_t LEAF_TRIANGLES = 6;

    struct GPUNode
    {
        glm::vec4 min;
        glm::vec4 max;
        uint32_t left;
        uint32_t right;
        uint32_t triangleCount; // 0 = interior, >0 = leaf
        uint32_t pad;
    };

    std::vector<GPUNode> nodes;
    std::vector<GPUTriangle> triangles;

    BVH(std::vector<GPUTriangle> &triangles);

    void split(const uint32_t nodeIndex, const int depth = 0);
    void growToInclude(GPUNode &node, const glm::vec3 point);
    void growToInclude(GPUNode &node, const GPUTriangle &triangle);
};

#endif
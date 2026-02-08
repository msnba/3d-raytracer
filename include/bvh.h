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
        glm::vec3 min;
        uint32_t leftFirst; // index of left child OR first triangle
        glm::vec3 max;
        uint32_t triangleCount; // 0 = interior, >0 = leaf
    };

    std::vector<GPUNode> nodes;
    std::vector<GPUTriangle> triangles;

    BVH(std::vector<GPUTriangle> &triangles);

    void split(const uint32_t nodeIndex, const int depth = 0);
    void growToInclude(GPUNode &node, const glm::vec3 point);
    void growToInclude(GPUNode &node, const GPUTriangle &triangle);
};

#endif
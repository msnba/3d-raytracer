#include "bvh.h"

BVH::BVH(std::vector<GPUTriangle> &triangles) : triangles(triangles)
{
    GPUNode node{};
    node.leftFirst = 0;
    node.triangleCount = triangles.size();

    constexpr float numeric_max = std::numeric_limits<float>::max();
    node.min = glm::vec3(numeric_max);
    node.max = glm::vec3(-numeric_max);

    for (const GPUTriangle &tri : triangles)
        BVH::growToInclude(node, tri);

    nodes.push_back(node);

    split(0);

    std::cout << "bvh built with: " << nodes.size() << " nodes\n";
}

void BVH::split(const uint32_t nodeIndex, const int depth)
{
    GPUNode &node = nodes[nodeIndex];

    if (depth == MAX_DEPTH || node.triangleCount <= LEAF_TRIANGLES)
        return;

    glm::vec3 size = node.max - node.min;
    int splitAxis = 0;
    if (size.y > size.x && size.y > size.z)
        splitAxis = 1;
    else if (size.z > size.x && size.z > size.y)
        splitAxis = 2;

    float splitPos = (node.min[splitAxis] + node.max[splitAxis]) * 0.5f;

    uint32_t begin = node.leftFirst;
    uint32_t end = begin + node.triangleCount;
    uint32_t mid = begin;

    for (uint32_t i = begin; i < end; i++)
    {
        const GPUTriangle &t = triangles[i];
        float center = (t.a[splitAxis] + t.b[splitAxis] + t.c[splitAxis]) / 3.0f;

        if (center < splitPos)
        {
            std::swap(triangles[i], triangles[mid]);
            mid++;
        }
    }

    uint32_t leftCount = mid - begin;
    uint32_t rightCount = end - mid;

    if (leftCount == 0 || rightCount == 0)
    {
        float avgCenter = 0.0f;
        for (uint32_t i = begin; i < end; i++)
        {
            const GPUTriangle &t = triangles[i];
            avgCenter += (t.a[splitAxis] + t.b[splitAxis] + t.c[splitAxis]) / 3.0f;
        }
        avgCenter /= node.triangleCount;

        splitPos = avgCenter;
        mid = begin;

        for (uint32_t i = begin; i < end; i++)
        {
            const GPUTriangle &t = triangles[i];
            float center = (t.a[splitAxis] + t.b[splitAxis] + t.c[splitAxis]) / 3.0f;

            if (center < splitPos)
            {
                std::swap(triangles[i], triangles[mid]);
                mid++;
            }
        }

        leftCount = mid - begin;
        rightCount = end - mid;

        if (leftCount == 0 || rightCount == 0)
        {
            mid = begin + node.triangleCount / 2;
            leftCount = mid - begin;
            rightCount = end - mid;
        }
    }

    uint32_t leftIdx = nodes.size();
    nodes.emplace_back();
    nodes.emplace_back();

    GPUNode &left = nodes[leftIdx];
    GPUNode &right = nodes[leftIdx + 1];

    left.leftFirst = begin;
    left.triangleCount = leftCount;
    right.leftFirst = mid;
    right.triangleCount = rightCount;

    constexpr float numeric_max = std::numeric_limits<float>::max();

    left.min = glm::vec3(numeric_max);
    left.max = glm::vec3(-numeric_max);
    right.min = glm::vec3(numeric_max);
    right.max = glm::vec3(-numeric_max);

    for (uint32_t i = begin; i < mid; i++)
        growToInclude(left, triangles[i]);

    for (uint32_t i = mid; i < end; i++)
        growToInclude(right, triangles[i]);

    node.leftFirst = leftIdx;
    node.triangleCount = 0;

    split(leftIdx, depth + 1);
    split(leftIdx + 1, depth + 1);
}

void BVH::growToInclude(GPUNode &node, glm::vec3 point)
{
    node.min = glm::min(node.min, point);
    node.max = glm::max(node.max, point);
}

void BVH::growToInclude(GPUNode &node, const GPUTriangle &triangle)
{
    BVH::growToInclude(node, triangle.a);
    BVH::growToInclude(node, triangle.b);
    BVH::growToInclude(node, triangle.c);
}
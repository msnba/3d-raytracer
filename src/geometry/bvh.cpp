#include <limits>

#include "tinytracer/geometry/bvh.h"

namespace tinytracer::geometry {

BVH::BVH(std::vector<tinytracer::renderer::GPUTriangle> &triangles_)
    : triangles_(triangles_) {
  tinytracer::renderer::GPUNode node{};
  node.triangleCount = static_cast<uint32_t>(triangles_.size());

  for (const auto &tri : triangles_)
    growNodeToInclude(node, tri);

  nodes_.reserve(2 * triangles_.size());
  nodes_.push_back(node);

  split(0);
}

void BVH::growNodeToInclude(tinytracer::renderer::GPUNode &node,
                            const glm::vec3 &point) {
  node.min = glm::min(node.min, glm::vec4(point, 0));
  node.max = glm::max(node.max, glm::vec4(point, 0));
}

void BVH::growNodeToInclude(tinytracer::renderer::GPUNode &node,
                            const tinytracer::renderer::GPUTriangle &triangle) {
  growNodeToInclude(node, triangle.a);
  growNodeToInclude(node, triangle.b);
  growNodeToInclude(node, triangle.c);
}

void BVH::split(const uint32_t nodeIndex, const size_t depth) {
  auto &node = nodes_[nodeIndex];

  if (depth >= BVH_MAX_DEPTH || node.triangleCount <= BVH_LEAF_TRIANGLES)
    return;

  SplitResult splitResult = findBestSplit(node);
  int splitAxis = splitResult.axis;
  float splitPos = splitResult.pos;

  uint32_t begin = node.left;
  uint32_t end = begin + node.triangleCount;
  uint32_t mid = begin;

  // puts triangles_[begin..mid) to the left and triangles_[mid..end) to the
  // right
  for (uint32_t i = begin; i < end; i++) {
    const auto &tri = triangles_[i];
    float center =
        (tri.a[splitAxis] + tri.b[splitAxis] + tri.c[splitAxis]) / 3.0f;

    // put triangle at front of the range if to the left of the split plane
    if (center < splitPos)
      std::swap(triangles_[i], triangles_[mid++]);
  }

  // prevents infinite recursion in the case of every triangle being on one side
  if (mid == begin || mid == end)
    mid = begin + (node.triangleCount / 2);

  uint32_t leftCount = mid - begin;
  uint32_t rightCount = end - mid;

  uint32_t leftIdx = static_cast<uint32_t>(nodes_.size());
  nodes_.emplace_back();
  nodes_.emplace_back();

  auto &left = nodes_[leftIdx];
  auto &right = nodes_[leftIdx + 1];

  left.left = begin;
  left.right = 0;
  left.triangleCount = leftCount;

  right.left = mid;
  right.right = 0;
  right.triangleCount = rightCount;

  node.left = leftIdx;
  node.right = leftIdx + 1;
  node.triangleCount = 0;

  constexpr float numeric_max = std::numeric_limits<float>::max();
  left.min = glm::vec4(numeric_max);
  left.max = glm::vec4(-numeric_max);
  right.min = glm::vec4(numeric_max);
  right.max = glm::vec4(-numeric_max);

  for (uint32_t i = begin; i < mid; i++)
    growNodeToInclude(left, triangles_[i]);
  for (uint32_t i = mid; i < end; i++)
    growNodeToInclude(right, triangles_[i]);

  split(leftIdx, depth + 1);
  split(leftIdx + 1, depth + 1);
}

float BVH::surfaceArea(const glm::vec3 &min, const glm::vec3 &max) const {
  glm::vec3 e = glm::vec3(max) - glm::vec3(min);
  return 2.0f * (e.x * e.y + e.y * e.z + e.z * e.x);
}

float BVH::surfaceArea(const tinytracer::renderer::GPUNode &node) const {
  return surfaceArea(node.min, node.max);
}

void BVH::growBinToInclude(Bin &bin,
                           const tinytracer::renderer::GPUTriangle &tri) {
  bin.min = glm::min(bin.min, {tri.a, 0});
  bin.min = glm::min(bin.min, {tri.b, 0});
  bin.min = glm::min(bin.min, {tri.c, 0});
  bin.max = glm::max(bin.max, {tri.a, 0});
  bin.max = glm::max(bin.max, {tri.b, 0});
  bin.max = glm::max(bin.max, {tri.c, 0});
}

BVH::SplitResult BVH::findBestSplit(const tinytracer::renderer::GPUNode &node) {
  SplitResult best = {0, 0.0f, std::numeric_limits<float>::max()};

  float parentArea = surfaceArea(node);

  for (int axis = 0; axis < 3; axis++) {
    float axisMin = node.min[axis];
    float axisMax = node.max[axis];
    if (axisMax - axisMin < 1e-6f)
      continue;

    std::vector<Bin> bins(SAH_NUM_BINS);
    float binSize = (axisMax - axisMin) / SAH_NUM_BINS;

    uint32_t begin = node.left;
    uint32_t end = begin + node.triangleCount;
    for (uint32_t i = begin; i < end; i++) {
      float centroid = (triangles_[i].a[axis] + triangles_[i].b[axis] +
                        triangles_[i].c[axis]) /
                       3.0f;
      long unsigned int binIdx = static_cast<long unsigned int>(std::min(
          static_cast<int>((centroid - axisMin) / binSize), SAH_NUM_BINS - 1));
      bins[binIdx].count++;
      growBinToInclude(bins[binIdx], triangles_[i]);
    }

    // Evaluate all NUM_BINS-1 split positions
    for (long unsigned int s = 1; s < SAH_NUM_BINS; s++) {
      // left
      glm::vec4 lMin(std::numeric_limits<float>::max());
      glm::vec4 lMax(-std::numeric_limits<float>::max());
      long unsigned int lCount = 0;
      for (long unsigned int b = 0; b < s; b++) {
        lMin = glm::min(lMin, bins[b].min);
        lMax = glm::max(lMax, bins[b].max);
        lCount += static_cast<long unsigned int>(bins[b].count);
      }
      // right
      glm::vec4 rMin(std::numeric_limits<float>::max());
      glm::vec4 rMax(-std::numeric_limits<float>::max());
      long unsigned int rCount = 0;
      for (long unsigned int b = s; b < SAH_NUM_BINS; b++) {
        rMin = glm::min(rMin, bins[b].min);
        rMax = glm::max(rMax, bins[b].max);
        rCount += static_cast<long unsigned int>(bins[b].count);
      }

      float cost = (static_cast<float>(lCount) * surfaceArea(lMin, lMax) +
                    static_cast<float>(rCount) * surfaceArea(rMin, rMax)) /
                   parentArea;

      if (cost < best.cost) {
        best.cost = cost;
        best.axis = axis;
        best.pos = axisMin + static_cast<float>(s) * binSize;
      }
    }
  }

  return best;
}

} // namespace tinytracer::geometry

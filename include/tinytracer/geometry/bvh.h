#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "tinytracer/renderer/scene_uploader.h"

namespace tinytracer::geometry {

class BVH {
public:
  static constexpr int BVH_MAX_DEPTH = 32;
  static constexpr int BVH_LEAF_TRIANGLES = 4;
  static constexpr int SAH_NUM_BINS = 12;

  std::vector<tinytracer::renderer::GPUNode> nodes_;
  std::vector<tinytracer::renderer::GPUTriangle> triangles_;

  BVH() = default;
  BVH(std::vector<tinytracer::renderer::GPUTriangle> &triangles);

private:
  struct Bin {
    glm::vec4 min = glm::vec4((std::numeric_limits<float>::max)());
    glm::vec4 max = glm::vec4(-(std::numeric_limits<float>::max)());
    int count = 0;
  };
  struct SplitResult {
    int axis;
    float pos;
    float cost;
  };

  void growNodeToInclude(tinytracer::renderer::GPUNode &node,
                         const glm::vec3 &point);
  void growNodeToInclude(tinytracer::renderer::GPUNode &node,
                         const tinytracer::renderer::GPUTriangle &triangle);
  void split(const uint32_t nodeIndex, const size_t depth = 0);

  float surfaceArea(const glm::vec3 &min, const glm::vec3 &max) const;
  float surfaceArea(const tinytracer::renderer::GPUNode &node) const;

  void growBinToInclude(Bin &bin, const tinytracer::renderer::GPUTriangle &tri);
  SplitResult findBestSplit(const tinytracer::renderer::GPUNode &node);
};

} // namespace tinytracer::geometry
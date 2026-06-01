#pragma once

#include <span>

#include "tinytracer/world/scene.h"

using GLuint = unsigned int;

namespace tinytracer::renderer {

struct GPUMaterial {
  glm::vec3 color;
  float smoothness;
  glm::vec4 emission;
  float transparency;
  float ior;
  float pad0 = 0;
  float pad1 = 0;
};

struct GPUTriangle {
  glm::vec3 a;
  uint32_t materialIdx;

  glm::vec3 b;
  uint32_t transformIdx;

  glm::vec3 c;
  uint32_t pad0 = 0;
};

struct GPUNode {
  glm::vec4 min;
  glm::vec4 max;
  uint32_t left;
  uint32_t right;
  uint32_t triangleCount; // 0 = interior, >0 = leaf
  uint32_t pad;
};

struct GPUSphere {
  glm::vec3 position;
  float radius;

  glm::vec3 color;
  float smoothness;
  glm::vec4 emission;
  float transparency;
  float ior;
  float pad0;
  float pad1;
};

struct GPUTransform {
  glm::mat4 modelMatrix;
  glm::mat4 invModelMatrix;
  glm::mat4 normalMatrix;
};

struct GPUTLASEntry {
  glm::vec4 worldMin; // transformed from object-space bounds
  glm::vec4 worldMax;
  uint32_t blasOffset; // index of mesh root BVH node in the packed BLAS array
  uint32_t triOffset;
  uint32_t triCount;
  uint32_t transformIdx;
};

struct MeshData {
  std::vector<GPUNode> blasNodes;
  std::vector<GPUTriangle> triangles;
  GPUTLASEntry tlasEntry;
};

struct PackedSceneGeometry {
  std::vector<GPUNode> blasNodes;        // all BLASes concatenated
  std::vector<GPUTriangle> triangles;    // all triangles concatenated
  std::vector<GPUTLASEntry> tlasEntries; // one per mesh, with offsets filled in
};

enum class RebuildFlags : uint8_t {
  None = 0,
  Spheres = 1 << 0,
  Materials = 1 << 1,
  Geometry = 1 << 2,
  SceneData = 1 << 3,
  Transforms = 1 << 4,
  All = 0xFF
};

class SceneUploader {
public:
  SceneUploader() = default;
  ~SceneUploader();

  SceneUploader(const SceneUploader &) = delete;
  SceneUploader &operator=(const SceneUploader &) = delete;

  void upload(const tinytracer::world::Scene &scene, RebuildFlags flags);

  void bindAll() const;

private:
  GLuint sphereSSBO_ = 0;
  GLuint matSSBO_ = 0;
  GLuint triSSBO_ = 0;
  GLuint blasSSBO_ = 0;
  GLuint dataSSBO_ = 0;
  GLuint tlasSSBO_ = 0;
  GLuint transformSSBO_ = 0;

  // binding slots
  static constexpr GLuint SLOT_SPHERES_ = 0;
  static constexpr GLuint SLOT_MATERIALS_ = 1;
  static constexpr GLuint SLOT_TRIANGLES_ = 2;
  static constexpr GLuint SLOT_BLAS_ = 3;
  static constexpr GLuint SLOT_DATA_ = 4;
  static constexpr GLuint SLOT_TLAS_ = 5;
  static constexpr GLuint SLOT_TRANSFORMS_ = 6;

  std::vector<GPUTLASEntry> cachedTLASEntries_;

  void uploadBuffer(GLuint &ssbo, GLuint slot, const void *data, size_t size);

  template <typename T_>
  void uploadBuffer(GLuint &ssbo, GLuint slot, std::span<T_> data) {
    uploadBuffer(ssbo, slot, data.data(), data.size_bytes());
  }

  template <typename T_>
  void uploadBuffer(GLuint &ssbo, GLuint slot, const std::vector<T_> &data) {
    uploadBuffer(ssbo, slot, std::span<const T_>(data.data(), data.size()));
  }
};

inline RebuildFlags operator|(RebuildFlags a, RebuildFlags b) {
  return static_cast<RebuildFlags>(static_cast<uint8_t>(a) |
                                   static_cast<uint8_t>(b));
}

inline RebuildFlags operator&(RebuildFlags a, RebuildFlags b) {
  return static_cast<RebuildFlags>(static_cast<uint8_t>(a) &
                                   static_cast<uint8_t>(b));
}

inline bool hasFlag(RebuildFlags a, RebuildFlags b) {
  return (a & b) != RebuildFlags::None;
}

} // namespace tinytracer::renderer

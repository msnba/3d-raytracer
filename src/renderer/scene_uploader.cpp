#include <glad/glad.h>

#include "tinytracer/geometry/bvh.h"
#include "tinytracer/renderer/scene_uploader.h"
#include "tinytracer/utils/log.h"

namespace tinytracer::renderer {

SceneUploader::~SceneUploader() {
  if (sphereSSBO_)
    glDeleteBuffers(1, &sphereSSBO_);
  if (matSSBO_)
    glDeleteBuffers(1, &matSSBO_);
  if (triSSBO_)
    glDeleteBuffers(1, &triSSBO_);
  if (blasSSBO_)
    glDeleteBuffers(1, &blasSSBO_);
  if (dataSSBO_)
    glDeleteBuffers(1, &dataSSBO_);
  if (tlasSSBO_)
    glDeleteBuffers(1, &tlasSSBO_);
  if (transformSSBO_)
    glDeleteBuffers(1, &transformSSBO_);
}

void SceneUploader::upload(const tinytracer::world::Scene &scene,
                           RebuildFlags flags) {

  using namespace tinytracer::world;

  tinytracer::utils::info("rebuilt scene");
  if (hasFlag(flags, RebuildFlags::Spheres)) {
    std::vector<Sphere *> spheres = scene.getObjectsOfType<Sphere>();
    std::vector<GPUSphere> gpuSpheres;
    gpuSpheres.reserve(spheres.size());
    for (Sphere *s : spheres)
      gpuSpheres.push_back(convertToGPUObject(*s));

    uploadBuffer(sphereSSBO_, SLOT_SPHERES_, gpuSpheres.data(),
                 gpuSpheres.size() * sizeof(GPUSphere));
  }

  if (hasFlag(flags, RebuildFlags::Materials)) {
    // std::vector<Object::Material *> &materials = pScene->materials_;
    std::vector<GPUMaterial> gpuMaterials;
    gpuMaterials.reserve(scene.materials_.size());
    for (Object::Material *m : scene.materials_)
      gpuMaterials.push_back({m->color, m->smoothness, m->emission,
                              m->transparency, m->ior, 0, 0});

    uploadBuffer(matSSBO_, SLOT_MATERIALS_, gpuMaterials.data(),
                 gpuMaterials.size() * sizeof(GPUMaterial));
  }

  if (hasFlag(flags, RebuildFlags::Geometry)) {
    std::vector<Mesh *> meshes = scene.getObjectsOfType<Mesh>();
    PackedSceneGeometry packed = packMeshes(meshes);

    cachedTLASEntries_ = packed.tlasEntries;

    uploadBuffer(triSSBO_, SLOT_TRIANGLES_, packed.triangles);
    uploadBuffer(blasSSBO_, SLOT_BLAS_, packed.blasNodes);
    uploadBuffer(tlasSSBO_, SLOT_TLAS_, packed.tlasEntries);

    std::vector<GPUTransform> transforms = buildGPUTransforms(meshes);
    uploadBuffer(transformSSBO_, SLOT_TRANSFORMS_, transforms);
  }

  if (hasFlag(flags, RebuildFlags::Transforms) &&
      !hasFlag(flags, RebuildFlags::Geometry)) {
    // meshes
    std::vector<Mesh *> meshes = scene.getObjectsOfType<Mesh>();

    rebuildTLAS(cachedTLASEntries_, meshes);
    uploadBuffer(tlasSSBO_, SLOT_TLAS_, cachedTLASEntries_);

    std::vector<GPUTransform> transforms = buildGPUTransforms(meshes);
    uploadBuffer(transformSSBO_, SLOT_TRANSFORMS_, transforms);

    // spheres (position reupload)
    std::vector<Sphere *> spheres = scene.getObjectsOfType<Sphere>();
    std::vector<GPUSphere> gpuSpheres;
    gpuSpheres.reserve(spheres.size());
    for (Sphere *s : spheres)
      gpuSpheres.push_back(convertToGPUObject(*s));
    uploadBuffer(sphereSSBO_, SLOT_SPHERES_, gpuSpheres.data(),
                 gpuSpheres.size() * sizeof(GPUSphere));
  }

  if (hasFlag(flags, RebuildFlags::SceneData)) {
    struct GPUSceneData {
      uint32_t maxBounce;
      uint32_t numRaysPerPixel;
      uint32_t SSAA;
    } sceneData{scene.settings().maxBounce, scene.settings().numRaysPerPixel,
                scene.settings().isSSAAEnabled};

    uploadBuffer(dataSSBO_, SLOT_DATA_, &sceneData, sizeof(GPUSceneData));
  }
}

void SceneUploader::uploadBuffer(GLuint &ssbo, GLuint slot, const void *data,
                                 size_t size) {
  if (!ssbo) {
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(size), data,
                 GL_DYNAMIC_DRAW);
  } else {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

    GLint currentSize = 0;
    glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE,
                           &currentSize);

    // if the size is the exact same, reallocation isn't necessary
    if (static_cast<size_t>(currentSize) != size)
      glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(size),
                   data, GL_DYNAMIC_DRAW);
    else
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                      static_cast<GLsizeiptr>(size), data);
  }

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, ssbo);
}

void SceneUploader::bindAll() const {
  if (sphereSSBO_)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SLOT_SPHERES_, sphereSSBO_);
  if (matSSBO_)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SLOT_MATERIALS_, matSSBO_);
  if (triSSBO_)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SLOT_TRIANGLES_, triSSBO_);
  if (blasSSBO_)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SLOT_BLAS_, blasSSBO_);
  if (dataSSBO_)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SLOT_DATA_, dataSSBO_);
  if (tlasSSBO_)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SLOT_TLAS_, tlasSSBO_);
  if (transformSSBO_)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SLOT_TRANSFORMS_,
                     transformSSBO_);
}

GPUSphere convertToGPUObject(const tinytracer::world::Sphere &sphere) {
  return GPUSphere{sphere.transform_.position,
                   sphere.radius_,
                   sphere.material_.color,
                   sphere.material_.smoothness,
                   sphere.material_.emission,
                   sphere.material_.transparency,
                   sphere.material_.ior,
                   0,
                   0};
}

std::vector<GPUSphere>
convertToGPUObject(const std::vector<tinytracer::world::Sphere *> &spheres) {
  std::vector<GPUSphere> outSpheres;
  outSpheres.reserve(spheres.size());

  for (const auto *sphere : spheres)
    outSpheres.push_back(convertToGPUObject(*sphere));

  return outSpheres;
}

MeshGPUData buildMeshGPUData(const tinytracer::world::Mesh &mesh,
                             uint32_t transformIdx) {
  MeshGPUData out;

  // Build object-space triangles — no model matrix applied.
  for (size_t i = 0; i + 2 < mesh.indices_.size(); i += 3) {
    GPUTriangle tri{};
    tri.a = mesh.vertices_[static_cast<size_t>(mesh.indices_[i].vertex_index)];
    tri.b =
        mesh.vertices_[static_cast<size_t>(mesh.indices_[i + 1].vertex_index)];
    tri.c =
        mesh.vertices_[static_cast<size_t>(mesh.indices_[i + 2].vertex_index)];
    tri.materialIdx = mesh.materialIdx_;
    tri.transformIdx = transformIdx;
    out.triangles.push_back(tri);
  }

  // Build BLAS in object space.
  tinytracer::geometry::BVH blas(out.triangles);
  out.blasNodes = blas.nodes_;
  out.triangles =
      blas.triangles_; // BVH reorders triangles, take the reordered copy

  // Populate TLAS entry — offsets are filled in by packMeshes().
  out.tlasEntry.blasOffset = 0; // filled later
  out.tlasEntry.triOffset = 0;  // filled later
  out.tlasEntry.triCount = static_cast<uint32_t>(out.triangles.size());
  out.tlasEntry.transformIdx = transformIdx;

  // World-space AABB — transform the 8 object-space corners.
  glm::mat4 model = mesh.getMatrix();
  glm::vec3 corners[8] = {
      mesh.minBounds_,
      {mesh.maxBounds_.x, mesh.minBounds_.y, mesh.minBounds_.z},
      {mesh.minBounds_.x, mesh.maxBounds_.y, mesh.minBounds_.z},
      {mesh.minBounds_.x, mesh.minBounds_.y, mesh.maxBounds_.z},
      {mesh.minBounds_.x, mesh.maxBounds_.y, mesh.maxBounds_.z},
      {mesh.maxBounds_.x, mesh.minBounds_.y, mesh.maxBounds_.z},
      {mesh.maxBounds_.x, mesh.maxBounds_.y, mesh.minBounds_.z},
      mesh.maxBounds_};

  glm::vec3 worldMin(std::numeric_limits<float>::max());
  glm::vec3 worldMax(-std::numeric_limits<float>::max());
  for (const glm::vec3 &c : corners) {
    glm::vec3 wc = glm::vec3(model * glm::vec4(c, 1.0f));
    worldMin = glm::min(worldMin, wc);
    worldMax = glm::max(worldMax, wc);
  }
  out.tlasEntry.worldMin = glm::vec4(worldMin, 0.0f);
  out.tlasEntry.worldMax = glm::vec4(worldMax, 0.0f);

  return out;
}

PackedSceneGeometry
packMeshes(const std::vector<tinytracer::world::Mesh *> &meshes) {
  PackedSceneGeometry packed;
  uint32_t blasOffset = 0;
  uint32_t triOffset = 0;

  for (uint32_t i = 0; i < static_cast<uint32_t>(meshes.size()); i++) {
    MeshGPUData data = buildMeshGPUData(*meshes[i], i);

    data.tlasEntry.blasOffset = blasOffset;
    data.tlasEntry.triOffset = triOffset;

    blasOffset += static_cast<uint32_t>(data.blasNodes.size());
    triOffset += static_cast<uint32_t>(data.triangles.size());

    packed.blasNodes.insert(packed.blasNodes.end(), data.blasNodes.begin(),
                            data.blasNodes.end());
    packed.triangles.insert(packed.triangles.end(), data.triangles.begin(),
                            data.triangles.end());
    packed.tlasEntries.push_back(data.tlasEntry);
  }

  return packed;
}

void rebuildTLAS(std::vector<GPUTLASEntry> &entries,
                 const std::vector<tinytracer::world::Mesh *> &meshes) {
  // Only recomputes world-space AABBs and transformIdx.
  // BLAS offsets and tri offsets are unchanged — don't touch them.
  for (uint32_t i = 0; i < static_cast<uint32_t>(meshes.size()); i++) {
    const auto &mesh = *meshes[i];
    GPUTLASEntry &entry = entries[i];

    entry.transformIdx = i;

    glm::mat4 model = mesh.getMatrix();
    glm::vec3 corners[8] = {
        mesh.minBounds_,
        {mesh.maxBounds_.x, mesh.minBounds_.y, mesh.minBounds_.z},
        {mesh.minBounds_.x, mesh.maxBounds_.y, mesh.minBounds_.z},
        {mesh.minBounds_.x, mesh.minBounds_.y, mesh.maxBounds_.z},
        {mesh.minBounds_.x, mesh.maxBounds_.y, mesh.maxBounds_.z},
        {mesh.maxBounds_.x, mesh.minBounds_.y, mesh.maxBounds_.z},
        {mesh.maxBounds_.x, mesh.maxBounds_.y, mesh.minBounds_.z},
        mesh.maxBounds_};

    glm::vec3 worldMin(std::numeric_limits<float>::max());
    glm::vec3 worldMax(-std::numeric_limits<float>::max());
    for (const glm::vec3 &c : corners) {
      glm::vec3 wc = glm::vec3(model * glm::vec4(c, 1.0f));
      worldMin = glm::min(worldMin, wc);
      worldMax = glm::max(worldMax, wc);
    }
    entry.worldMin = glm::vec4(worldMin, 0.0f);
    entry.worldMax = glm::vec4(worldMax, 0.0f);
  }
}

GPUTransform buildGPUTransform(const tinytracer::world::Mesh &mesh) {
  glm::mat4 model = mesh.getMatrix();
  glm::mat4 inv = glm::inverse(model);
  return GPUTransform{model, inv, glm::transpose(inv)};
}
std::vector<GPUTransform>
buildGPUTransforms(const std::vector<tinytracer::world::Mesh *> &meshes) {
  std::vector<GPUTransform> out;
  out.reserve(meshes.size());
  for (const auto *mesh : meshes)
    out.push_back(buildGPUTransform(*mesh));
  return out;
}

} // namespace tinytracer::renderer
#include "object.h"
#include "log.h"
#include "bvh.h"

#include <iostream>

void Mesh::loadMeshFromPath(const std::string &path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::string warn, err;

    bool ok = tinyobj::LoadObj(
        &attrib,
        &shapes,
        nullptr,
        &warn,
        &err,
        path.c_str(),
        nullptr,
        true);

    if (!warn.empty())
        Log::warning(warn);
    if (!err.empty())
        Log::error(err);
    if (!ok)
    {
        Log::error("Failed to load OBJ: " + path);
        return;
    }

    for (const tinyobj::shape_t &shape : shapes)
    {
        for (size_t i = 0; i + 2 < shape.mesh.indices.size(); i += 3)
        {
            for (size_t j = 0; j < 3; j++)
            {
                tinyobj::index_t idx = shape.mesh.indices[i + j];
                unsigned int vi = static_cast<unsigned int>(idx.vertex_index);
                glm::vec3 v = {
                    attrib.vertices[vi * 3u],
                    attrib.vertices[vi * 3u + 1u],
                    attrib.vertices[vi * 3u + 2u]};
                idx.vertex_index = static_cast<int>(vertices_.size());
                vertices_.push_back(v);
                minBounds_ = glm::min(minBounds_, v);
                maxBounds_ = glm::max(maxBounds_, v);
                indices_.push_back(idx);
            }
        }
    }
}

GPUSphere convertToGPUObject(const Sphere &sphere)
{
    return GPUSphere{sphere.transform_.position, sphere.radius_, sphere.material_.color, sphere.material_.smoothness, sphere.material_.emission, sphere.material_.transparency, sphere.material_.ior, 0, 0};
}

std::vector<GPUSphere> convertToGPUObject(const std::vector<Sphere *> &spheres)
{
    std::vector<GPUSphere> outSpheres;
    outSpheres.reserve(spheres.size());

    for (const Sphere *sphere : spheres)
        outSpheres.push_back(convertToGPUObject(*sphere));

    return outSpheres;
}

MeshGPUData buildMeshGPUData(const Mesh &mesh, uint32_t transformIdx)
{
    MeshGPUData out;

    // Build object-space triangles — no model matrix applied.
    for (size_t i = 0; i + 2 < mesh.indices_.size(); i += 3)
    {
        GPUTriangle tri{};
        tri.a = mesh.vertices_[static_cast<size_t>(mesh.indices_[i].vertex_index)];
        tri.b = mesh.vertices_[static_cast<size_t>(mesh.indices_[i + 1].vertex_index)];
        tri.c = mesh.vertices_[static_cast<size_t>(mesh.indices_[i + 2].vertex_index)];
        tri.materialIdx = mesh.materialIdx_;
        tri.transformIdx = transformIdx;
        out.triangles.push_back(tri);
    }

    // Build BLAS in object space.
    BVH blas(out.triangles);
    out.blasNodes = blas.nodes_;
    out.triangles = blas.triangles_; // BVH reorders triangles, take the reordered copy

    // Populate TLAS entry — offsets are filled in by packMeshes().
    out.tlasEntry.blasOffset = 0; // filled later
    out.tlasEntry.triOffset = 0;  // filled later
    out.tlasEntry.triCount = static_cast<uint32_t>(out.triangles.size());
    out.tlasEntry.transformIdx = transformIdx;

    // World-space AABB — transform the 8 object-space corners.
    glm::mat4 model = getMatrix(mesh.transform_);
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
    for (const glm::vec3 &c : corners)
    {
        glm::vec3 wc = glm::vec3(model * glm::vec4(c, 1.0f));
        worldMin = glm::min(worldMin, wc);
        worldMax = glm::max(worldMax, wc);
    }
    out.tlasEntry.worldMin = glm::vec4(worldMin, 0.0f);
    out.tlasEntry.worldMax = glm::vec4(worldMax, 0.0f);

    return out;
}

PackedSceneGeometry packMeshes(const std::vector<Mesh *> &meshes)
{
    PackedSceneGeometry packed;
    uint32_t blasOffset = 0;
    uint32_t triOffset = 0;

    for (uint32_t i = 0; i < static_cast<uint32_t>(meshes.size()); i++)
    {
        MeshGPUData data = buildMeshGPUData(*meshes[i], i);

        data.tlasEntry.blasOffset = blasOffset;
        data.tlasEntry.triOffset = triOffset;

        blasOffset += static_cast<uint32_t>(data.blasNodes.size());
        triOffset += static_cast<uint32_t>(data.triangles.size());

        packed.blasNodes.insert(packed.blasNodes.end(), data.blasNodes.begin(), data.blasNodes.end());
        packed.triangles.insert(packed.triangles.end(), data.triangles.begin(), data.triangles.end());
        packed.tlasEntries.push_back(data.tlasEntry);
    }

    return packed;
}

void rebuildTLAS(std::vector<GPUTLASEntry> &entries, const std::vector<Mesh *> &meshes)
{
    // Only recomputes world-space AABBs and transformIdx.
    // BLAS offsets and tri offsets are unchanged — don't touch them.
    for (uint32_t i = 0; i < static_cast<uint32_t>(meshes.size()); i++)
    {
        const Mesh &mesh = *meshes[i];
        GPUTLASEntry &entry = entries[i];

        entry.transformIdx = i;

        glm::mat4 model = getMatrix(mesh.transform_);
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
        for (const glm::vec3 &c : corners)
        {
            glm::vec3 wc = glm::vec3(model * glm::vec4(c, 1.0f));
            worldMin = glm::min(worldMin, wc);
            worldMax = glm::max(worldMax, wc);
        }
        entry.worldMin = glm::vec4(worldMin, 0.0f);
        entry.worldMax = glm::vec4(worldMax, 0.0f);
    }
}

GPUTransform buildGPUTransform(const Mesh &mesh)
{
    glm::mat4 model = getMatrix(mesh.transform_);
    glm::mat4 inv = glm::inverse(model);
    return GPUTransform{model, inv, glm::transpose(inv)};
}
std::vector<GPUTransform> buildGPUTransforms(const std::vector<Mesh *> &meshes)
{
    std::vector<GPUTransform> out;
    out.reserve(meshes.size());
    for (const Mesh *mesh : meshes)
        out.push_back(buildGPUTransform(*mesh));
    return out;
}
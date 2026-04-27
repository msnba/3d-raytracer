#include "object.h"
#include "log.h"

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

std::pair<GPUMesh, std::vector<GPUTriangle>> convertToGPUObject(const Mesh &mesh)
{
    std::vector<GPUTriangle> outTriangles;
    glm::mat4 model = getMatrix(mesh.transform_);

    for (size_t i = 0; i + 2 < mesh.indices_.size(); i += 3)
    {
        GPUTriangle tri{};

        tri.a = model * glm::vec4(mesh.vertices_[static_cast<size_t>(mesh.indices_[i].vertex_index)], 1.0f);
        tri.b = model * glm::vec4(mesh.vertices_[static_cast<size_t>(mesh.indices_[i + 1].vertex_index)], 1.0f);
        tri.c = model * glm::vec4(mesh.vertices_[static_cast<size_t>(mesh.indices_[i + 2].vertex_index)], 1.0f);

        tri.materialIdx = mesh.materialIdx_;
        tri.pad0 = 0;
        tri.pad1 = 0;

        outTriangles.push_back(tri);
    }

    glm::vec3 corners[8] = {// just hard programmed in the corners
                            mesh.minBounds_,
                            {mesh.maxBounds_.x, mesh.minBounds_.y, mesh.minBounds_.z},
                            {mesh.minBounds_.x, mesh.maxBounds_.y, mesh.minBounds_.z},
                            {mesh.minBounds_.x, mesh.minBounds_.y, mesh.maxBounds_.z},
                            {mesh.minBounds_.x, mesh.maxBounds_.y, mesh.maxBounds_.z},
                            {mesh.maxBounds_.x, mesh.minBounds_.y, mesh.maxBounds_.z},
                            {mesh.maxBounds_.x, mesh.maxBounds_.y, mesh.minBounds_.z},
                            mesh.maxBounds_};

    glm::vec3 worldMin(std::numeric_limits<float>::max());
    glm::vec3 worldMax(std::numeric_limits<float>::lowest());

    for (int i = 0; i < 8; i++)
    {
        glm::vec3 worldCorner = glm::vec3(model * glm::vec4(corners[i], 1.0f));
        worldMin = glm::min(worldMin, worldCorner);
        worldMax = glm::max(worldMax, worldCorner);
    }

    GPUMesh gpuMesh{
        glm::uvec4(0,
                   static_cast<uint32_t>(mesh.indices_.size() / 3),
                   mesh.materialIdx_, 0),
        glm::vec4(worldMin, 0),
        glm::vec4(worldMax, 0)};

    return std::pair(gpuMesh, outTriangles);
}

std::pair<std::vector<GPUMesh>, std::vector<GPUTriangle>> convertToGPUObject(const std::vector<Mesh *> &meshes)
{
    std::vector<GPUMesh> outMeshes;
    std::vector<GPUTriangle> outTriangles;
    size_t triOffset = 0;

    for (const Mesh *mesh : meshes)
    {
        auto [gpuMesh, tris] = convertToGPUObject(*mesh);
        gpuMesh.data.x = static_cast<uint32_t>(triOffset);
        triOffset += tris.size();
        outMeshes.push_back(gpuMesh);
        outTriangles.insert(outTriangles.end(), tris.begin(), tris.end());
    }

    return {outMeshes, outTriangles};
}

std::pair<GPUMesh, std::vector<GPUTriangle>> convertToGPUObject(const Rectangle &rectangle)
{
    return convertToGPUObject(static_cast<const Mesh &>(rectangle));
}

std::pair<std::vector<GPUMesh>, std::vector<GPUTriangle>> convertToGPUObject(const std::vector<Rectangle *> &rectangles)
{
    std::vector<Mesh *> meshes;
    meshes.reserve(rectangles.size());
    for (Rectangle *rectangle : rectangles)
        meshes.push_back(static_cast<Mesh *>(rectangle));
    return convertToGPUObject(meshes);
}
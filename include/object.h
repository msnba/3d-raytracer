#ifndef OBJECT_H
#define OBJECT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

struct GPUMaterial
{
    glm::vec3 color;
    float smoothness;
    glm::vec4 emission;
};

struct GPUTriangle
{
    glm::vec4 v0, v1, v2; // vec4 used for padding
    glm::vec4 n0, n1, n2;
};

struct GPUMesh
{
    uint32_t firstTriangle;
    uint32_t triangleCount;
    uint32_t materialIdx;
};
struct GPUSphere
{
    glm::vec3 position;
    float radius;

    // explicit GPUMaterial declaration or else it breaks
    glm::vec3 color;
    float smoothness;
    glm::vec4 emission;
};

struct Mesh
{
    std::vector<tinyobj::index_t> indices;
    const tinyobj::attrib_t *attrib;
    uint32_t materialIdx;
};

struct Scene
{
    std::vector<Mesh> meshes;
    std::vector<GPUMaterial> materials;
    std::vector<GPUSphere> spheres;
};

static Mesh loadMesh(const std::string &path, const GPUMaterial &mat, std::vector<GPUMaterial> &materialPool) // mesh loader method
{
    uint32_t materialIndex = static_cast<uint32_t>(materialPool.size());
    materialPool.push_back(mat);

    tinyobj::attrib_t *attrib = new tinyobj::attrib_t(); // mem leak, fix later
    std::vector<tinyobj::shape_t> shapes;
    std::string warn, err;

    bool ok = tinyobj::LoadObj(
        attrib,
        &shapes,
        nullptr,
        &warn,
        &err,
        path.c_str(),
        nullptr,
        true);

    if (!warn.empty())
        std::cout << "WARN: " << warn << std::endl;
    if (!err.empty())
        std::cerr << "ERROR: " << err << std::endl;
    if (!ok)
    {
        std::cerr << "Failed to load OBJ: " << path << std::endl;
        return Mesh();
    }

    std::vector<tinyobj::index_t> indices;

    for (const tinyobj::shape_t &shape : shapes)
    {
        for (const tinyobj::index_t &idx : shape.mesh.indices)
        {
            indices.push_back(idx);
        }
    }

    return Mesh{std::move(indices), attrib, materialIndex};
}

static void convertToGPUMeshes(const Scene &scene, std::vector<GPUTriangle> &outTriangles, std::vector<GPUMesh> &outMeshes)
{
    outTriangles.clear();
    outMeshes.clear();

    size_t triOffset = 0;

    for (const Mesh &mesh : scene.meshes)
    {
        const tinyobj::attrib_t &attrib = *mesh.attrib;
        size_t triangleCount = static_cast<uint32_t>(mesh.indices.size() / 3);

        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            const tinyobj::index_t &i0 = mesh.indices[i + 0];
            const tinyobj::index_t &i1 = mesh.indices[i + 1];
            const tinyobj::index_t &i2 = mesh.indices[i + 2];

            auto pos = [&](const tinyobj::index_t &idx)
            {
                return glm::vec3(
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]);
            };

            auto nrm = [&](const tinyobj::index_t &idx)
            {
                if (idx.normal_index < 0)
                    return glm::vec3(0, 1, 0);
                return glm::vec3(
                    attrib.normals[3 * idx.normal_index + 0],
                    attrib.normals[3 * idx.normal_index + 1],
                    attrib.normals[3 * idx.normal_index + 2]);
            };

            GPUTriangle tri;
            tri.v0 = glm::vec4(pos(i0), 1.0f);
            tri.v1 = glm::vec4(pos(i1), 1.0f);
            tri.v2 = glm::vec4(pos(i2), 1.0f);

            tri.n0 = glm::vec4(nrm(i0), 0.0f);
            tri.n1 = glm::vec4(nrm(i1), 0.0f);
            tri.n2 = glm::vec4(nrm(i2), 0.0f);
            outTriangles.push_back(tri);
        }

        outMeshes.push_back(GPUMesh{
            .firstTriangle = static_cast<uint32_t>(triOffset),
            .triangleCount = static_cast<uint32_t>(triangleCount),
            .materialIdx = mesh.materialIdx});

        triOffset += triangleCount;
    }
}

#endif
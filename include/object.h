#ifndef OBJECT_H
#define OBJECT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <iostream>

#include "tiny_obj_loader.h"

// -- Structs --

struct Transform
{
    glm::vec3 position = glm::vec3(0);
    glm::vec3 rotation = glm::vec3(0);
    glm::vec3 scale = glm::vec3(1);

    glm::mat4 getMatrix() const
    {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), position);

        glm::mat4 Rx = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1, 0, 0));
        glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0, 1, 0));
        glm::mat4 Rz = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0, 0, 1));

        glm::mat4 R = Rx * Ry * Rz;

        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

        return T * R * S;
    };
};
struct GPUMaterial
{
    glm::vec3 color;
    float smoothness;
    glm::vec4 emission;
};

struct GPUTriangle
{
    glm::vec3 a;
    uint32_t materialIdx;

    glm::vec3 b;
    uint32_t pad0;

    glm::vec3 c;
    uint32_t pad1;
};

struct GPUMesh
{
    glm::uvec4 data; // firstTriangle, triangleCount, materialIdx, pad
    glm::vec4 minBounds;
    glm::vec4 maxBounds;
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

struct Rectangle
{
    Transform transform;
    GPUMaterial material;
};

struct Mesh
{
    std::vector<tinyobj::index_t> indices;
    const tinyobj::attrib_t *attrib;
    Transform transform;
    uint32_t materialIdx;
    glm::vec3 minBounds;
    glm::vec3 maxBounds;
};

struct Scene
{
    std::vector<Mesh> meshes;
    std::vector<GPUMaterial> materials;
    std::vector<GPUSphere> spheres;
};

// -- Mesh Handling --

static Mesh loadMesh(const std::string &path, const GPUMaterial &mat, const Transform &transform, std::vector<GPUMaterial> &materialPool) // mesh loader method
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

    glm::vec3 minB(std::numeric_limits<float>::max());
    glm::vec3 maxB(std::numeric_limits<float>::lowest());
    std::vector<tinyobj::index_t> indices;

    for (const tinyobj::shape_t &shape : shapes)
    {
        for (const tinyobj::index_t &idx : shape.mesh.indices)
        {
            indices.push_back(idx);

            float vx = attrib->vertices[3 * idx.vertex_index + 0];
            float vy = attrib->vertices[3 * idx.vertex_index + 1];
            float vz = attrib->vertices[3 * idx.vertex_index + 2];

            minB = glm::min(minB, glm::vec3(vx, vy, vz));
            maxB = glm::max(maxB, glm::vec3(vx, vy, vz));
        }
    }

    return Mesh{std::move(indices), attrib, transform, materialIndex, minB, maxB};
}

inline glm::vec3 pos(const tinyobj::index_t &idx, const tinyobj::attrib_t &attrib)
{
    return glm::vec3(
        attrib.vertices[3 * idx.vertex_index + 0],
        attrib.vertices[3 * idx.vertex_index + 1],
        attrib.vertices[3 * idx.vertex_index + 2]);
}

inline glm::vec3 nrm(const tinyobj::index_t &idx, const tinyobj::attrib_t &attrib)
{
    if (idx.normal_index < 0)
        return glm::vec3(0, 1, 0);
    return glm::vec3(
        attrib.normals[3 * idx.normal_index + 0],
        attrib.normals[3 * idx.normal_index + 1],
        attrib.normals[3 * idx.normal_index + 2]);
}

static void convertToGPUMeshes(const Scene &scene, std::vector<GPUTriangle> &outTriangles, std::vector<GPUMesh> &outMeshes)
{
    outTriangles.clear();
    outMeshes.clear();

    size_t triOffset = 0;

    for (const Mesh &mesh : scene.meshes)
    {
        const tinyobj::attrib_t &attrib = *mesh.attrib;
        glm::mat4 model = mesh.transform.getMatrix(); // Compute once here
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            tinyobj::index_t ind[3] = {};
            for (int j = 0; j < 3; j++)
            {
                ind[j] = mesh.indices[i + j];
            }

            GPUTriangle tri;

            tri.a = model * glm::vec4(pos(ind[0], attrib), 1.0f);
            tri.b = model * glm::vec4(pos(ind[1], attrib), 1.0f);
            tri.c = model * glm::vec4(pos(ind[2], attrib), 1.0f);

            tri.materialIdx = mesh.materialIdx;
            tri.pad0 = 0;
            tri.pad1 = 0;

            outTriangles.push_back(tri);
        }

        glm::vec3 corners[8] = {// just hard programmed in the corners
                                mesh.minBounds,
                                {mesh.maxBounds.x, mesh.minBounds.y, mesh.minBounds.z},
                                {mesh.minBounds.x, mesh.maxBounds.y, mesh.minBounds.z},
                                {mesh.minBounds.x, mesh.minBounds.y, mesh.maxBounds.z},
                                {mesh.minBounds.x, mesh.maxBounds.y, mesh.maxBounds.z},
                                {mesh.maxBounds.x, mesh.minBounds.y, mesh.maxBounds.z},
                                {mesh.maxBounds.x, mesh.maxBounds.y, mesh.minBounds.z},
                                mesh.maxBounds};

        glm::vec3 worldMin(std::numeric_limits<float>::max());
        glm::vec3 worldMax(std::numeric_limits<float>::lowest());

        for (int i = 0; i < 8; i++)
        {
            glm::vec3 worldCorner = glm::vec3(model * glm::vec4(corners[i], 1.0f));
            worldMin = glm::min(worldMin, worldCorner);
            worldMax = glm::max(worldMax, worldCorner);
        }

        outMeshes.push_back(GPUMesh{
            glm::uvec4(static_cast<uint32_t>(triOffset),
                       static_cast<uint32_t>(mesh.indices.size() / 3),
                       mesh.materialIdx, 0),
            glm::vec4(worldMin, 0),
            glm::vec4(worldMax, 0)});

        triOffset += (mesh.indices.size()) / 3;
    }
}

// -- Primitive Handling --
static Mesh loadRect(Rectangle rect, Scene &scene)
{
    // lazy way of doing this until i fix it
    return loadMesh("assets/models/cube.obj", rect.material, rect.transform, scene.materials);
}

#endif
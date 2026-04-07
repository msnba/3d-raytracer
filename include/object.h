#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <stdint.h>

#include "tiny_obj_loader.h"

// -- Structs --

struct Transform
{
    glm::vec3 position = glm::vec3(0);
    glm::vec3 rotation = glm::vec3(0);
    glm::vec3 scale = glm::vec3(1);
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
    std::vector<glm::vec3> vertices;
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

inline glm::vec3 pos(const tinyobj::index_t &idx, const tinyobj::attrib_t &attrib)
{
    unsigned int vertex_index = 3u * static_cast<unsigned int>(idx.vertex_index);
    return glm::vec3(
        attrib.vertices[vertex_index + 0],
        attrib.vertices[vertex_index + 1],
        attrib.vertices[vertex_index + 2]);
}

inline glm::vec3 nrm(const tinyobj::index_t &idx, const tinyobj::attrib_t &attrib)
{
    if (idx.normal_index < 0)
        return glm::vec3(0, 1, 0);

    unsigned int normal_index = 3u * static_cast<unsigned int>(idx.normal_index);
    return glm::vec3(
        attrib.normals[normal_index + 0],
        attrib.normals[normal_index + 1],
        attrib.normals[normal_index + 2]);
}

inline glm::mat4 getMatrix(const Transform &t)
{
    glm::mat4 T = glm::translate(glm::mat4(1.0f), t.position);

    glm::mat4 Rx = glm::rotate(glm::mat4(1.0f), t.rotation.x, glm::vec3(1, 0, 0));
    glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), t.rotation.y, glm::vec3(0, 1, 0));
    glm::mat4 Rz = glm::rotate(glm::mat4(1.0f), t.rotation.z, glm::vec3(0, 0, 1));

    glm::mat4 R = Rx * Ry * Rz;

    glm::mat4 S = glm::scale(glm::mat4(1.0f), t.scale);

    return T * R * S;
}

Mesh loadMesh(const std::string &path, const GPUMaterial &mat, const Transform &transform, std::vector<GPUMaterial> &materialPool);

void convertToGPUMeshes(const Scene &scene, std::vector<GPUTriangle> &outTriangles, std::vector<GPUMesh> &outMeshes);

Mesh loadRect(struct Rectangle rect, Scene &scene);
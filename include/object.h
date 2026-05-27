#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <stdint.h>
#include <limits>

#include "tiny_obj_loader.h"

class Object
{
public:
    struct Transform
    {
        glm::vec3 position = glm::vec3(0);
        glm::vec3 rotation = glm::vec3(0);
        glm::vec3 scale = glm::vec3(1);
    } transform_;

    struct Material
    {
        glm::vec3 color;
        float smoothness;
        glm::vec4 emission;
        float transparency;
        float ior;
    } material_;

    Object() = default;
    Object(Transform transform, Material material) : transform_(transform), material_(material) {}
    virtual ~Object() = default; // makes dynamic casting work, just trust me bro

    virtual std::string typeName() const = 0;

    uint32_t materialIdx_ = 0;
    uint32_t transformIdx_ = 0;
};

class Sphere : public Object
{
public:
    Sphere() = default;
    Sphere(glm::vec3 position, float radius, Material material) : Object({position, {0, 0, 0}, glm::vec3(radius)}, material), radius_(radius) {}

    std::string typeName() const override { return "sphere"; };

    float radius_;
};

class Mesh : public Object
{
public:
    Mesh() = default;
    Mesh(Transform transform, Material material, std::vector<tinyobj::index_t> indices, std::vector<glm::vec3> vertices) : Object(transform, material), indices_(indices), vertices_(vertices) {}
    Mesh(const std::string &path, Transform transform, Material material) : Object(transform, material), path_(path)
    {
        loadMeshFromPath(path);
    }

    std::string typeName() const override { return "mesh"; };

    std::string path_;
    std::vector<tinyobj::index_t> indices_;
    std::vector<glm::vec3> vertices_;
    glm::vec3 minBounds_ = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 maxBounds_ = glm::vec3(std::numeric_limits<float>::lowest());

protected:
    void loadMeshFromPath(const std::string &path);
};

class Cube : public Mesh
{
public:
    Cube() = default;
    Cube(glm::vec3 position, glm::vec3 rotation, float width, float height, float length, Material material) : Mesh({position, rotation, glm::vec3(width, height, length)}, material, {}, {})
    {
        // cube generation

        vertices_ = {
            {-0.5f, -0.5f, -0.5f},
            {0.5f, -0.5f, -0.5f},
            {0.5f, 0.5f, -0.5f},
            {-0.5f, 0.5f, -0.5f},
            {-0.5f, -0.5f, 0.5f},
            {0.5f, -0.5f, 0.5f},
            {0.5f, 0.5f, 0.5f},
            {-0.5f, 0.5f, 0.5f},
        };

        // 12 triangles, wound CCW when viewed from outside, formatter makes it like this
        static const int faceIndices[36] = {
            0,
            2,
            1,
            0,
            3,
            2, // -Z
            4,
            5,
            6,
            4,
            6,
            7, // +Z
            0,
            1,
            5,
            0,
            5,
            4, // -Y
            3,
            6,
            2,
            3,
            7,
            6, // +Y
            0,
            4,
            7,
            0,
            7,
            3, // -X
            1,
            2,
            6,
            1,
            6,
            5, // +X
        };

        indices_.clear();
        for (int vi : faceIndices)
        {
            tinyobj::index_t idx{};
            idx.vertex_index = vi;
            idx.normal_index = -1;
            idx.texcoord_index = -1;
            indices_.push_back(idx);
        }

        minBounds_ = glm::vec3(-0.5f);
        maxBounds_ = glm::vec3(0.5f);
    }

    std::string typeName() const override { return "cube"; };
};

struct GPUMaterial
{
    glm::vec3 color;
    float smoothness;
    glm::vec4 emission;
    float transparency;
    float ior;
    float pad0 = 0;
    float pad1 = 0;
};

struct GPUTriangle
{
    glm::vec3 a;
    uint32_t materialIdx;

    glm::vec3 b;
    uint32_t transformIdx;

    glm::vec3 c;
    uint32_t pad0 = 0;
};

struct GPUNode
{
    glm::vec4 min;
    glm::vec4 max;
    uint32_t left;
    uint32_t right;
    uint32_t triangleCount; // 0 = interior, >0 = leaf
    uint32_t pad;
};

struct GPUSphere
{
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

struct GPUTransform
{
    glm::mat4 modelMatrix;
    glm::mat4 invModelMatrix;
    glm::mat4 normalMatrix;
};

GPUSphere convertToGPUObject(const Sphere &sphere);
std::vector<GPUSphere> convertToGPUObject(const std::vector<Sphere *> &spheres);

struct GPUTLASEntry
{
    glm::vec4 worldMin; // transformed from object-space bounds
    glm::vec4 worldMax;
    uint32_t blasOffset; // index of mesh root BVH node in the packed BLAS array
    uint32_t triOffset;
    uint32_t triCount;
    uint32_t transformIdx;
};

struct MeshGPUData
{
    std::vector<GPUNode> blasNodes;
    std::vector<GPUTriangle> triangles;
    GPUTLASEntry tlasEntry;
};

MeshGPUData buildMeshGPUData(const Mesh &mesh, uint32_t transformIdx);

struct PackedSceneGeometry
{
    std::vector<GPUNode> blasNodes;        // all BLASes concatenated
    std::vector<GPUTriangle> triangles;    // all triangles concatenated
    std::vector<GPUTLASEntry> tlasEntries; // one per mesh, with offsets filled in
};
PackedSceneGeometry packMeshes(const std::vector<Mesh *> &meshes);

void rebuildTLAS(std::vector<GPUTLASEntry> &entries, const std::vector<Mesh *> &meshes);

GPUTransform buildGPUTransform(const Mesh &mesh);
std::vector<GPUTransform> buildGPUTransforms(const std::vector<Mesh *> &meshes);

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

inline glm::mat4 getMatrix(const Object::Transform &t)
{
    glm::mat4 T = glm::translate(glm::mat4(1.0f), t.position);

    glm::mat4 Rx = glm::rotate(glm::mat4(1.0f), t.rotation.x, glm::vec3(1, 0, 0));
    glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), t.rotation.y, glm::vec3(0, 1, 0));
    glm::mat4 Rz = glm::rotate(glm::mat4(1.0f), t.rotation.z, glm::vec3(0, 0, 1));

    glm::mat4 R = Rx * Ry * Rz;

    glm::mat4 S = glm::scale(glm::mat4(1.0f), t.scale);

    return T * R * S;
}
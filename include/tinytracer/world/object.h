#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <stdint.h>
#include <string>
#include <vector>

#include "tiny_obj_loader.h"

namespace tinytracer::world {

class Object {
public:
  struct Transform {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 rotation = glm::vec3(0);
    glm::vec3 scale = glm::vec3(1);
  } transform_;

  struct Material {
    glm::vec3 color;
    float smoothness;
    glm::vec4 emission;
    float transparency;
    float ior;
  } material_;

  Object() = default;
  Object(Transform transform, Material material, std::string name)
      : transform_(transform), material_(material), name_(name) {}
  virtual ~Object() = default; // makes dynamic casting work

  virtual std::string typeName() const = 0;

  inline glm::mat4 getMatrix() const {
    glm::mat4 T = glm::translate(glm::mat4(1.0f), transform_.position);

    glm::mat4 Rx =
        glm::rotate(glm::mat4(1.0f), transform_.rotation.x, glm::vec3(1, 0, 0));
    glm::mat4 Ry =
        glm::rotate(glm::mat4(1.0f), transform_.rotation.y, glm::vec3(0, 1, 0));
    glm::mat4 Rz =
        glm::rotate(glm::mat4(1.0f), transform_.rotation.z, glm::vec3(0, 0, 1));

    glm::mat4 R = Rx * Ry * Rz;

    glm::mat4 S = glm::scale(glm::mat4(1.0f), transform_.scale);

    return T * R * S;
  }

  uint32_t materialIdx_ = 0;
  uint32_t transformIdx_ = 0;

  std::string name_;
};

class Sphere : public Object {
public:
  Sphere() = default;
  Sphere(glm::vec3 position, float radius, Material material, std::string name)
      : Object({position, {0, 0, 0}, glm::vec3(radius)}, material, name) {}

  std::string typeName() const override { return "sphere"; };

  float radius() const { return transform_.scale.x; };
};

class Mesh : public Object {
public:
  Mesh() = default;
  Mesh(Transform transform, Material material, std::string name,
       std::vector<tinyobj::index_t> indices, std::vector<glm::vec3> vertices)
      : Object(transform, material, name), indices_(indices),
        vertices_(vertices) {}
  Mesh(const std::string &path, Transform transform, Material material,
       std::string name)
      : Object(transform, material, name), path_(path) {
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

class Cube : public Mesh {
public:
  Cube() = default;
  Cube(glm::vec3 position, glm::vec3 rotation, float width, float height,
       float length, Material material, std::string name)
      : Mesh({position, rotation, glm::vec3(width, height, length)}, material,
             name, {}, {}) {
    // cube generation

    vertices_ = {
        {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f},
        {-0.5f, 0.5f, -0.5f},  {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f},
        {0.5f, 0.5f, 0.5f},    {-0.5f, 0.5f, 0.5f},
    };

    // 12 triangles, wound CCW when viewed from outside, formatter makes it like
    // this
    static const int faceIndices[36] = {
        0, 2, 1, 0, 3,
        2, // -Z
        4, 5, 6, 4, 6,
        7, // +Z
        0, 1, 5, 0, 5,
        4, // -Y
        3, 6, 2, 3, 7,
        6, // +Y
        0, 4, 7, 0, 7,
        3, // -X
        1, 2, 6, 1, 6,
        5, // +X
    };

    indices_.clear();
    for (int vi : faceIndices) {
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

inline glm::vec3 pos(const tinyobj::index_t &idx,
                     const tinyobj::attrib_t &attrib) {
  unsigned int vertex_index = 3u * static_cast<unsigned int>(idx.vertex_index);
  return glm::vec3(attrib.vertices[vertex_index + 0],
                   attrib.vertices[vertex_index + 1],
                   attrib.vertices[vertex_index + 2]);
}

inline glm::vec3 nrm(const tinyobj::index_t &idx,
                     const tinyobj::attrib_t &attrib) {
  if (idx.normal_index < 0)
    return glm::vec3(0, 1, 0);

  unsigned int normal_index = 3u * static_cast<unsigned int>(idx.normal_index);
  return glm::vec3(attrib.normals[normal_index + 0],
                   attrib.normals[normal_index + 1],
                   attrib.normals[normal_index + 2]);
}

} // namespace tinytracer::world
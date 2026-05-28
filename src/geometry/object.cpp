#include <iostream>

#include "tinytracer/utils/log.h"
#include "tinytracer/world/object.h"

namespace tinytracer::world {

void Mesh::loadMeshFromPath(const std::string &path) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::string warn, err;

  bool ok = tinyobj::LoadObj(&attrib, &shapes, nullptr, &warn, &err,
                             path.c_str(), nullptr, true);

  if (!warn.empty())
    tinytracer::utils::warning(warn);
  if (!err.empty())
    tinytracer::utils::error(err);
  if (!ok) {
    tinytracer::utils::error("Failed to load OBJ: " + path);
    return;
  }

  for (const tinyobj::shape_t &shape : shapes) {
    for (size_t i = 0; i + 2 < shape.mesh.indices.size(); i += 3) {
      for (size_t j = 0; j < 3; j++) {
        tinyobj::index_t idx = shape.mesh.indices[i + j];
        unsigned int vi = static_cast<unsigned int>(idx.vertex_index);
        glm::vec3 v = {attrib.vertices[vi * 3u], attrib.vertices[vi * 3u + 1u],
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

}
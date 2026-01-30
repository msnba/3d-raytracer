#ifndef OBJECT_H
#define OBJECT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

struct Object
{
    std::vector<glm::vec3> vertices;
    glm::vec3 color = glm::vec3(1.0f);

    unsigned int VBO = 0, VAO = 0;
    size_t vertexCount = 0;

    Object() = default;
    Object(const std::vector<glm::vec3> &verts, const glm::vec3 col)
    {
        vertices = verts;
        color = col;
    }

    static Object loadObj(const std::string path, const glm::vec3 color) // easy object loader method
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
            std::cout << "WARN: " << warn << std::endl;
        if (!err.empty())
            std::cerr << "ERROR: " << err << std::endl;
        if (!ok)
        {
            std::cerr << "Failed to load OBJ: " << path << std::endl;
            return Object();
        }

        std::vector<glm::vec3> vertices;
        for (const auto &shape : shapes)
        {
            for (const auto &idx : shape.mesh.indices)
            {
                vertices.push_back({attrib.vertices[3 * idx.vertex_index + 0],
                                    attrib.vertices[3 * idx.vertex_index + 1],
                                    attrib.vertices[3 * idx.vertex_index + 2]});
            }
        }

        return Object(vertices, color);
    }
};

#endif
#include "object.h"

Mesh loadMesh(const std::string &path, const GPUMaterial &mat, const Transform &transform, std::vector<GPUMaterial> &materialPool)
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

            unsigned int vertex_index = 3u * static_cast<unsigned int>(idx.vertex_index);
            float vx = attrib->vertices[vertex_index + 0];
            float vy = attrib->vertices[vertex_index + 1];
            float vz = attrib->vertices[vertex_index + 2];

            minB = glm::min(minB, glm::vec3(vx, vy, vz));
            maxB = glm::max(maxB, glm::vec3(vx, vy, vz));
        }
    }

    return Mesh{std::move(indices), attrib, transform, materialIndex, minB, maxB};
}

void convertToGPUMeshes(const Scene &scene, std::vector<GPUTriangle> &outTriangles, std::vector<GPUMesh> &outMeshes)
{
    outTriangles.clear();
    outMeshes.clear();

    size_t triOffset = 0;

    for (const Mesh &mesh : scene.meshes)
    {
        const tinyobj::attrib_t &attrib = *mesh.attrib;
        glm::mat4 model = getMatrix(mesh.transform); // Compute once here

        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            tinyobj::index_t ind[3] = {};
            for (int j = 0; j < 3; j++)
            {
                ind[j] = mesh.indices[i + static_cast<size_t>(j)];
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

Mesh loadRect(Rectangle rect, Scene &scene)
{
    // lazy way of doing this until i fix it
    return loadMesh("assets/models/cube.obj", rect.material, rect.transform, scene.materials);
}
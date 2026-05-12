#include <glad/glad.h>

#include "scene_uploader.h"
#include "bvh.h"
#include "log.h"

SceneUploader::~SceneUploader()
{
    if (sphereSSBO_)
        glDeleteBuffers(1, &sphereSSBO_);
    if (matSSBO_)
        glDeleteBuffers(1, &matSSBO_);
    if (triSSBO_)
        glDeleteBuffers(1, &triSSBO_);
    if (bvhSSBO_)
        glDeleteBuffers(1, &bvhSSBO_);
    if (dataSSBO_)
        glDeleteBuffers(1, &dataSSBO_);
}

void SceneUploader::upload(const Scene &scene, RebuildFlags flags)
{
    Log::info("rebuilt scene");
    if (hasFlag(flags, RebuildFlags::Spheres))
    {
        std::vector<Sphere *> spheres = scene.getObjectsOfType<Sphere>();
        std::vector<GPUSphere> gpuSpheres;
        gpuSpheres.reserve(spheres.size());
        for (Sphere *s : spheres)
            gpuSpheres.push_back(convertToGPUObject(*s));

        uploadBuffer(sphereSSBO_, SLOT_SPHERES_, gpuSpheres.data(), gpuSpheres.size() * sizeof(GPUSphere));
    }

    if (hasFlag(flags, RebuildFlags::Materials))
    {
        // std::vector<Object::Material *> &materials = pScene->materials_;
        std::vector<GPUMaterial> gpuMaterials;
        gpuMaterials.reserve(scene.materials_.size());
        for (Object::Material *m : scene.materials_)
            gpuMaterials.push_back({m->color, m->smoothness, m->emission, m->transparency, m->ior, 0, 0});

        uploadBuffer(matSSBO_, SLOT_MATERIALS_, gpuMaterials.data(), gpuMaterials.size() * sizeof(GPUMaterial));
    }

    if (hasFlag(flags, RebuildFlags::Geometry))
    {
        std::vector<Mesh *> meshes = scene.getObjectsOfType<Mesh>();
        auto [gpuMeshes, gpuTriangles] = convertToGPUObject(meshes);

        BVH bvh(gpuTriangles);

        uploadBuffer(triSSBO_, SLOT_TRIANGLES_, bvh.triangles_);
        uploadBuffer(bvhSSBO_, SLOT_BVH_, bvh.nodes_);
    }

    if (hasFlag(flags, RebuildFlags::SceneData))
    {
        struct GPUSceneData
        {
            uint32_t maxBounce;
            uint32_t numRaysPerPixel;
            uint32_t SSAA;
        } sceneData{scene.settings().maxBounce, scene.settings().numRaysPerPixel, scene.settings().isSSAAEnabled};

        uploadBuffer(dataSSBO_, SLOT_DATA_, &sceneData, sizeof(GPUSceneData));
    }
}

void SceneUploader::uploadBuffer(GLuint &ssbo, GLuint slot, const void *data, size_t size)
{
    if (!ssbo)
    {
        glGenBuffers(1, &ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(size), data, GL_DYNAMIC_DRAW);
    }
    else
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

        GLint currentSize = 0;
        glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &currentSize);

        // if the size is the exact same, reallocation isn't necessary
        if (static_cast<size_t>(currentSize) != size)
            glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(size), data, GL_DYNAMIC_DRAW);
        else
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, static_cast<GLsizeiptr>(size), data);
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, ssbo);
}

void SceneUploader::bindAll() const
{
    if (sphereSSBO_)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SLOT_SPHERES_, sphereSSBO_);
    if (matSSBO_)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SLOT_MATERIALS_, matSSBO_);
    if (triSSBO_)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SLOT_TRIANGLES_, triSSBO_);
    if (bvhSSBO_)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SLOT_BVH_, bvhSSBO_);
    if (dataSSBO_)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SLOT_DATA_, dataSSBO_);
}
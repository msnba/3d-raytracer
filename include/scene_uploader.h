#pragma once

#include "scene.h"

typedef unsigned int GLuint;

enum class RebuildFlags : uint8_t
{
    None = 0,
    Spheres = 1 << 0,
    Materials = 1 << 1,
    Geometry = 1 << 2,
    SceneData = 1 << 3,
    Transforms = 1 << 4,
    All = 0xFF
};

class SceneUploader
{
public:
    SceneUploader() = default;
    ~SceneUploader();

    SceneUploader(const SceneUploader &) = delete;
    SceneUploader &operator=(const SceneUploader &) = delete;

    void upload(const Scene &scene, RebuildFlags flags);

    void bindAll() const;

private:
    GLuint sphereSSBO_ = 0;
    GLuint matSSBO_ = 0;
    GLuint triSSBO_ = 0;
    GLuint bvhSSBO_ = 0;
    GLuint dataSSBO_ = 0;

    // binding slots
    static constexpr GLuint SLOT_SPHERES_ = 0;
    static constexpr GLuint SLOT_MATERIALS_ = 1;
    static constexpr GLuint SLOT_TRIANGLES_ = 2;
    static constexpr GLuint SLOT_BVH_ = 3;
    static constexpr GLuint SLOT_DATA_ = 4;

    void uploadBuffer(GLuint &ssbo, GLuint slot, const void *data, size_t size);
};

inline RebuildFlags operator|(RebuildFlags a, RebuildFlags b)
{
    return static_cast<RebuildFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline RebuildFlags operator&(RebuildFlags a, RebuildFlags b)
{
    return static_cast<RebuildFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline bool hasFlag(RebuildFlags a, RebuildFlags b)
{
    return (a & b) != RebuildFlags::None;
}

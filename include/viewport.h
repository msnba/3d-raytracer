#pragma once

#include <memory>
#include <atomic>

#include "window.h"
#include "camera.h"
#include "object.h"
#include "shader.h"
#include "gui.h"
#include "scene.h"

class Viewport
{
public:
    Viewport(std::unique_ptr<Window> window, std::unique_ptr<Camera> camera, std::unique_ptr<GUI> gui, std::weak_ptr<Scene> scene);
    ~Viewport();

    Viewport(const Viewport &) = delete;

    void update();
    void processGui();
    void processKeyInput();

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

    void rebuildScene(RebuildFlags flags);

    bool shouldClose() const;
    std::string getFPS();

private:
    void saveScreenshot();
    void toggleFullscreen();
    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
    static void cursorPosCallback(GLFWwindow *window, double xposd, double yposd);
    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);

    std::unique_ptr<Window> window_;
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<GUI> gui_;
    std::weak_ptr<Scene> scene_;
    GLFWwindow *rawWindow_ = nullptr;

    Shader passthrough_;
    Shader raytrace_;

    unsigned int quadVBO_ = 0, quadVAO_ = 0;
    unsigned int accumTexture_ = 0;

    unsigned int sphereSSBO_ = 0;
    unsigned int matSSBO_ = 0;
    unsigned int triSSBO_ = 0;
    unsigned int bvhSSBO_ = 0;
    unsigned int dataSSBO_ = 0;

    uint32_t maxBounce_ = 5;
    uint32_t numRaysPerPixel_ = 1;
    uint32_t isSSAAEnabled_ = 0;

    uint32_t accumFrameIndex_ = 0;
    float deltaTime_ = 0.0f;
    float lastFrame_ = 0.0f;
    int fpsFrameCount_ = 0;
    float lastFPS_ = 0.0f;
    float fpsTimer_ = 0.0f;
    const float fpsInterval_ = 0.5f;
    std::string fpsString_;

    float mouseLastX_ = 0.0f;
    float mouseLastY_ = 0.0f;

    bool isAccumulationEnabled_ = true;
    bool isScreenshot_ = false;
    bool isFullscreen_ = false;
    bool isPanning_ = false;
    bool isMoving_ = false;

    std::atomic<bool> screenshotInProgress_ = false;
    bool fullscreenPressed_ = false;
};

inline Viewport::RebuildFlags operator|(Viewport::RebuildFlags a, Viewport::RebuildFlags b)
{
    return static_cast<Viewport::RebuildFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline Viewport::RebuildFlags operator&(Viewport::RebuildFlags a, Viewport::RebuildFlags b)
{
    return static_cast<Viewport::RebuildFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline bool hasFlag(Viewport::RebuildFlags a, Viewport::RebuildFlags b)
{
    return (a & b) != Viewport::RebuildFlags::None;
}
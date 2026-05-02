#pragma once

#include <memory>
#include <atomic>

#include "window.h"
#include "camera.h"
#include "object.h"
#include "shader.h"
#include "gui.h"
#include "scene.h"
#include "scene_uploader.h"

class Viewport
{
public:
    Viewport(std::unique_ptr<Window> window, std::unique_ptr<Camera> camera, std::unique_ptr<GUI> gui, std::weak_ptr<Scene> scene);
    ~Viewport();

    Viewport(const Viewport &) = delete;
    Viewport &operator=(const Viewport &) = delete;

    void update();
    bool shouldClose() const;

private:
    void processGui();
    void processKeyInput();
    void rebuildAccumTexture();
    void saveScreenshot();
    void toggleFullscreen();

    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
    static void cursorPosCallback(GLFWwindow *window, double xposd, double yposd);
    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);

    std::string getFPS();

    std::unique_ptr<Window> window_;
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<GUI> gui_;
    std::weak_ptr<Scene> scene_;
    GLFWwindow *rawWindow_ = nullptr;

    Shader passthrough_;
    Shader raytrace_;

    SceneUploader sceneUploader_;

    unsigned int quadVBO_ = 0, quadVAO_ = 0;
    unsigned int accumTexture_ = 0;

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
    bool fullscreenPressed_ = false;
    bool isPanning_ = false;
    bool isMoving_ = false;

    std::atomic<bool> screenshotInProgress_ = false;
};
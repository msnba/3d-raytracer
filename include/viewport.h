#pragma once

#include <memory>

#include "window.h"
#include "camera.h"
#include "object.h"
#include "shader.h"
#include "gui.h"

class Viewport
{
    friend class GUI;

public:
    Viewport(std::unique_ptr<Window> window, std::unique_ptr<Camera> camera, std::unique_ptr<GUI> gui, std::weak_ptr<Scene> scene);
    ~Viewport();

    Viewport(const Viewport &) = delete;

    void update();
    void processGui();
    void processKeyInput();

    void rebuildScene();

    bool shouldClose() const;
    std::string getFPS();

private:
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
};
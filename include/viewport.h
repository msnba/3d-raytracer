#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <memory>

#include "window.h"
#include "camera.h"
#include "object.h"
#include "shader.h"

class Viewport
{
public:
    Viewport(std::unique_ptr<Window> window, std::unique_ptr<Camera> camera, std::weak_ptr<const Scene> scene_);
    ~Viewport();

    void update();
    void processGui();
    void processKeyInput();

    void rebuildScene();

    bool shouldClose() const;

private:
    static void cursorPosCallback(GLFWwindow *window, double xposd, double yposd);

    std::unique_ptr<Window> m_window;
    std::unique_ptr<Camera> m_camera;
    std::weak_ptr<const Scene> m_scene;
    GLFWwindow *m_rawWindow = nullptr;

    Shader m_passthrough;
    Shader m_raytrace;

    unsigned int m_quadVBO = 0, m_quadVAO = 0;
    unsigned int m_accumTexture = 0;

    unsigned int m_sphereSSBO = 0;
    unsigned int m_matSSBO = 0;
    unsigned int m_triSSBO = 0;
    unsigned int m_bvhSSBO = 0;
    unsigned int m_dataSSBO = 0;

    uint32_t m_accumFrameIndex = 0;
    float m_deltaTime = 0.0f;
    float m_lastFrame = 0.0f;

    float m_mouseLastX = 0.0f;
    float m_mouseLastY = 0.0f;
};

#endif
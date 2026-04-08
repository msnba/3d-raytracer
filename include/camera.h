#pragma once

#include <glm/glm.hpp>
#include <stdint.h>

struct GLFWwindow;

struct Camera
{
    friend class Viewport;

public:
    Camera() = default;
    Camera(float fov = 60.0f, float speed = 5.0f, float yaw = -90.0f, float pitch = 0.0f, glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f));

    bool handleKeyInput(GLFWwindow *window, float deltaTime);
    bool handleMouseInput(uint32_t &accumeFrameIndex, float mouseX, float mouseY, float &mouseLastX, float &mouseLastY);
    bool handleScrollInput(double xoffset, double yoffset);

private:
    float fov_, speed_, yaw_, pitch_;
    glm::vec3 cameraPos_, cameraFront_;
    void normalize();
};
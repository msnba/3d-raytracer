#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"

Camera::Camera(float fov, float speed, float yaw, float pitch, glm::vec3 cameraPos) : fov_(fov), speed_(speed), cameraPos_(cameraPos), yaw_(yaw), pitch_(pitch)
{
    cameraFront_ = glm::vec3(0.0f, 0.0f, 1.0f);
    cameraUp_ = glm::vec3(0.0f, 1.0f, 0.0f);

    normalize();
}

void Camera::handleKeyInput(GLFWwindow *window, float deltaTime)
{
    float velocity = speed_ * deltaTime;

    // movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos_ += velocity * cameraFront_;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos_ -= velocity * cameraFront_;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos_ -= glm::normalize(glm::cross(cameraFront_, cameraUp_)) * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos_ += glm::normalize(glm::cross(cameraFront_, cameraUp_)) * velocity;
}

void Camera::handleMouseInput(uint32_t &accumFrameIndex, float deltaTime, float mouseX, float mouseY, float &mouseLastX, float &mouseLastY)
{
    float xoffset = mouseX - mouseLastX;
    float yoffset = mouseLastY - mouseY; // y coords go from bottom to top
    mouseLastX = mouseX;
    mouseLastY = mouseY;

    const float sensitivity = 0.1f; // change this value to your liking
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw_ += xoffset;
    pitch_ += yoffset;

    // anti flip
    if (pitch_ > 89.0f)
        pitch_ = 89.0f;
    if (pitch_ < -89.0f)
        pitch_ = -89.0f;

    normalize();

    if (xoffset != 0.0f || yoffset != 0.0f)
        accumFrameIndex = 0;
}

void Camera::handleScrollInput(float xoffset, float yoffset)
{
    float sensitivity = 2.5f;
    fov_ = std::min(std::max(fov_ - sensitivity * yoffset, 0.1f), 150.0f);
}

void Camera::normalize()
{
    glm::vec3 front;
    front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front.y = sin(glm::radians(pitch_));
    front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    cameraFront_ = glm::normalize(front);
}

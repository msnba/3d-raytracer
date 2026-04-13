#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

#include "camera.h"

Camera::Camera(float fov, float speed, float yaw, float pitch, glm::vec3 cameraPos) : fov_(fov), speed_(speed), yaw_(yaw), pitch_(pitch), cameraPos_(cameraPos), cameraFront_(glm::vec3(0.0f, 0.0f, 1.0f))
{
    normalize();
}

bool Camera::handleKeyInput(GLFWwindow *window, float deltaTime)
{
    bool isMoving = false;
    float velocity = speed_ * deltaTime;

    // movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        cameraPos_ += velocity * glm::normalize(cameraFront_ * glm::vec3(1, 0, 1));
        isMoving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        cameraPos_ -= velocity * glm::normalize(cameraFront_ * glm::vec3(1, 0, 1));
        isMoving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        cameraPos_ -= velocity * glm::normalize(glm::cross(cameraFront_, glm::vec3(0, 1, 0)));
        isMoving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        cameraPos_ += velocity * glm::normalize(glm::cross(cameraFront_, glm::vec3(0, 1, 0)));
        isMoving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
    {
        cameraPos_ -= velocity * glm::vec3(0, 1, 0);
        isMoving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        cameraPos_ += velocity * glm::vec3(0, 1, 0);
        isMoving = true;
    }

    return isMoving;
}

bool Camera::handleMouseInput(uint32_t &accumFrameIndex, float mouseX, float mouseY, float &mouseLastX, float &mouseLastY)
{
    float xoffset = mouseX - mouseLastX;
    float yoffset = mouseLastY - mouseY; // y coords go from bottom to top

    if (xoffset == 0 && yoffset == 0)
        return false;

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

    return true;
}

bool Camera::handleScrollInput(double xoffset, double yoffset)
{
    static constexpr float sensitivity = 2.5f;

    if ((fov_ <= 10.0f && yoffset > 0) || (fov_ >= 150.0f && yoffset < 0))
        return false;

    fov_ = std::min(std::max(fov_ - sensitivity * static_cast<float>(yoffset), 10.f), 150.f);

    (void)xoffset;
    return true;
}

void Camera::normalize()
{
    glm::vec3 front;
    front.x = cosf(glm::radians(yaw_)) * cosf(glm::radians(pitch_));
    front.y = sinf(glm::radians(pitch_));
    front.z = sinf(glm::radians(yaw_)) * cosf(glm::radians(pitch_));
    cameraFront_ = glm::normalize(front);
}

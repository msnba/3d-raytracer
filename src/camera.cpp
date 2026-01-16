#include "camera.h"

Camera::Camera(float fov, float speed)
{
    this->fov = fov;
    this->speed = speed;

    cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
}

void Camera::getInput(GLFWwindow *window, float deltaTime)
{
    float velocity = speed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += velocity * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= velocity * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
}

glm::mat4 Camera::getProjection(int SCR_WIDTH, int SCR_HEIGHT)
{
    return glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
}

glm::mat4 Camera::getView()
{
    return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

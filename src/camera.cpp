#include "camera.h"

Camera::Camera(float fov, float speed)
{
    this->fov = fov;
    this->speed = speed;

    cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    yaw = -90.0f; // a yaw of 0.0 results in a direction vector pointing to the right
    pitch = 0.0f;
}

Camera::Camera(float fov, float speed, float yaw, float pitch, glm::vec3 cameraPos)
{
    this->fov = fov;
    this->speed = speed;

    this->cameraPos = cameraPos;
    this->cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
    this->cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    this->yaw = yaw;
    this->pitch = pitch;

    Camera::normalize();
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
    // if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    //     cameraPos += velocity * cameraUp;
    // if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
    //     cameraPos -= velocity * cameraUp;
}

glm::mat4 Camera::getProjection(int SCR_WIDTH, int SCR_HEIGHT)
{
    return glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
}

glm::mat4 Camera::getView()
{
    return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

void Camera::normalize()
{
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera
{
public:
    float fov, speed, yaw, pitch;
    glm::vec3 cameraPos, cameraFront, cameraUp;

    Camera(float fov = 60.0f, float speed = 5.0f);

    void getInput(GLFWwindow *window, float deltaTime);

    glm::mat4 getProjection(int SCR_WIDTH, int SCR_HEIGHT);
    glm::mat4 getView();

    void normalize();
};

#endif
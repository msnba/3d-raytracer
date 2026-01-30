#include <glad/glad.h>
#include <GLFW/glfw3.h> // ! Must be included after GLAD (due to method overriding).
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "camera.h"
#include "shader.h"
#include "window.h"
#include "object.h"

void mouseInput(GLFWwindow *window, double xpos, double ypos);
void getInput(GLFWwindow *window);

const unsigned int SCR_WIDTH = 1440;
const unsigned int SCR_HEIGHT = 1080;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

Camera camera(75.0f, 6.0f); // fov, speed
Window window(SCR_WIDTH, SCR_HEIGHT, "Window");

int main()
{
    // glfwSetCursorPosCallback(window.window, mouseInput);
    // glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    Shader raytracer("assets/raytracer.vert", "assets/raytracer.frag", ShaderType::PATH);

    // -- Quad Buffer --
    float quad[] = {// using a quad so fragment shader runs over every pixel on the screen
                    -1.f, -1.f,
                    1.f, -1.f,
                    -1.f, 1.f,
                    -1.f, 1.f,
                    1.f, -1.f,
                    1.f, 1.f};

    // super simple vertex buffer for a quad over the screen
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    while (!glfwWindowShouldClose(window.window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        getInput(window.window);

        // start draw
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(raytracer.ID);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window.window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(raytracer.ID);

    return 0;
}

float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;

void mouseInput(GLFWwindow *window, double xpos, double ypos)
{
    xpos = static_cast<float>(xpos);
    ypos = static_cast<float>(ypos);

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // y coords go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f; // change this value to your liking
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    camera.yaw += xoffset;
    camera.pitch += yoffset;

    // anti flip
    if (camera.pitch > 89.0f)
        if (camera.pitch > 89.0f)
            camera.pitch = 89.0f;
    if (camera.pitch < -89.0f)
        camera.pitch = -89.0f;

    camera.normalize();
}
void getInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    camera.getInput(window, deltaTime);
}
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
void resetAccumulation();

const unsigned int SCR_WIDTH = 1440;
const unsigned int SCR_HEIGHT = 1080;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

Camera camera(90.0f, 6.0f); // fov, speed
bool cameraDirty = true;    // dictates when the frame clears so there isn't smudging

Window window(SCR_WIDTH, SCR_HEIGHT, "Window");
uint32_t frameIndex = 0;

GLuint *gAccumFBO = nullptr;

void resetAccumulation()
{
    frameIndex = 0;

    if (!gAccumFBO)
        return;

    for (int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, gAccumFBO[i]);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int main()
{
    // -- Settings --
    glfwSetCursorPosCallback(window.window, mouseInput);
    glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glDisable(GL_BLEND);

    // -- Pixel Shader --
    Shader raytracer("assets/raytracer.vert", "assets/raytracer.frag", ShaderType::PATH);

    // ! THIS WILL BE REPLACED BY A COMPUTE SHADER LATER
    float quad[] = {// using a quad so fragment shader runs over every pixel on the screen
                    -1.f, -1.f,
                    1.f, -1.f,
                    -1.f, 1.f,
                    -1.f, 1.f,
                    1.f, -1.f,
                    1.f, 1.f};

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // -- Object Instantiation --
    Scene scene;

    scene.spheres.push_back({{2.f, 1.f, -9.f}, 1.0f, {1.f, 1.0f, 1.f}, 1.f, {0.f, 0.f, 0.f, 0.f}});
    scene.spheres.push_back({{5.0f, 0.5f, -8.f}, 1.0f, {1.f, 1.f, 0.f}, 1.f, {0.f, 0.f, 0.f, 0.f}});
    scene.spheres.push_back({{2.f, -15.0f, -10.f}, 15.0f, {1.f, 0.f, 1.f}, 0.f, {0.f, 0.f, 0.f, 0.f}});
    scene.spheres.push_back({{-15.f, 15.0f, 0.f}, 10.0f, {0.f, 0.f, 0.f}, 0.f, {1.f, 1.f, 1.f, 5.f}});
    scene.spheres.push_back({{2.f, -5.0f, -30.f}, 15.0f, {0.f, 1.f, 0.f}, 0.f, {0.f, 0.f, 0.f, 0.f}});

    scene.meshes.push_back(loadMesh("assets/models/tetrahedron.obj", GPUMaterial{{1.f, 1.f, 1.f}, 1.f, {0.f, 0.f, 0.f, 0.f}}, scene.materials));

    // -- SSBO's --

    GLuint sphereSSBO;
    glGenBuffers(1, &sphereSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        scene.spheres.size() * sizeof(GPUSphere),
        scene.spheres.data(),
        GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sphereSSBO);

    GLuint matSSBO;
    glGenBuffers(1, &matSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, matSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        scene.materials.size() * sizeof(GPUMaterial),
        scene.materials.data(),
        GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, matSSBO);

    std::vector<GPUTriangle> triangles;
    std::vector<GPUMesh> gpuMeshes;
    convertToGPUMeshes(scene, triangles, gpuMeshes);

    GLuint triSSBO;
    glGenBuffers(1, &triSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        triangles.size() * sizeof(GPUTriangle),
        triangles.data(),
        GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, triSSBO);

    GLuint meshSSBO;
    glGenBuffers(1, &meshSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        gpuMeshes.size() * sizeof(GPUMesh),
        gpuMeshes.data(),
        GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, meshSSBO);

    // -- Frame Accumulation --

    GLuint accumTex[2];
    GLuint accumFBO[2]; // swapping frame buffers to pass past frame into fragment shader for averaging
    glGenTextures(2, accumTex);
    glGenFramebuffers(2, accumFBO);

    for (int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, accumTex[i]);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA16F,
            SCR_WIDTH,
            SCR_HEIGHT,
            0,
            GL_RGBA,
            GL_FLOAT,
            nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, accumFBO[i]);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            accumTex[i],
            0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    gAccumFBO = accumFBO;

    int ping = 0;

    while (!glfwWindowShouldClose(window.window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        getInput(window.window);

        if (cameraDirty)
        {
            resetAccumulation();
            cameraDirty = false;
        }

        // start draw
        int pong = 1 - ping; // effectively flips frame buffers
        glBindFramebuffer(GL_FRAMEBUFFER, accumFBO[ping]);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(raytracer.ID);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, accumTex[pong]);
        glUniform1i(glGetUniformLocation(raytracer.ID, "previousFrame"), 0);

        glUniform1ui(
            glGetUniformLocation(raytracer.ID, "frameIndex"),
            frameIndex);

        glUniform1ui(
            glGetUniformLocation(raytracer.ID, "sphereCount"),
            static_cast<int>(scene.spheres.size()));

        raytracer.setVec3("cameraPos", camera.cameraPos);
        raytracer.setVec3("cameraFront", camera.cameraFront);
        raytracer.setVec3("cameraUp", camera.cameraUp);
        raytracer.setFloat("fov", camera.fov);
        raytracer.setVec2("resolution", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
        glUniform1ui(glGetUniformLocation(raytracer.ID, "maxBounce"), 30);
        glUniform1ui(glGetUniformLocation(raytracer.ID, "numRaysPerPixel"), 25);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, accumFBO[ping]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBlitFramebuffer( // copies previous frem buffer to another
            0, 0, SCR_WIDTH, SCR_HEIGHT,
            0, 0, SCR_WIDTH, SCR_HEIGHT,
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST);

        ping = pong;
        frameIndex++; // * Comment out to disable accumulation

        glfwSwapBuffers(window.window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(raytracer.ID);

    return 0;
}

float lastX = static_cast<float>(SCR_WIDTH) / 2.0;
float lastY = static_cast<float>(SCR_HEIGHT) / 2.0;

void mouseInput(GLFWwindow *window, double xposd, double yposd)
{
    float xpos = static_cast<float>(xposd);
    float ypos = static_cast<float>(yposd);

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

    if (xoffset != 0.0f || yoffset != 0.0f)
        cameraDirty = true;
}
void getInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    glm::vec3 oldPos = camera.cameraPos;
    float oldYaw = camera.yaw;
    float oldPitch = camera.pitch;

    camera.getInput(window, deltaTime);

    if (camera.cameraPos != oldPos ||
        camera.yaw != oldYaw ||
        camera.pitch != oldPitch)
    {
        cameraDirty = true;
    }
}
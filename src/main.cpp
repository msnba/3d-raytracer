#include <glad/glad.h>
#include <GLFW/glfw3.h> // ! Must be included after GLAD (due to method overriding).
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "camera.h"
#include "shader.h"
#include "window.h" //includes imgui imports
#include "object.h"
#include "bvh.h"

void mouseInput(GLFWwindow *window, double xposd, double yposd);
void getInput(GLFWwindow *window);

const unsigned int SCR_WIDTH = 1440;
const unsigned int SCR_HEIGHT = 1080;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float fps = 0.0f;

// Camera camera(90.0f, 6.0f); // fov, speed
Camera camera(90.0f, 6.0f, 0.0f, -10.0f, glm::vec3(-5, 4, 0));
bool cameraDirty = true; // dictates when the frame clears so there isn't smudging

Window window(SCR_WIDTH, SCR_HEIGHT, "Window");
uint32_t frameIndex = 0;

GLuint *gAccumFBO = nullptr;

int main()
{
    // -- Settings --
    // glfwSetCursorPosCallback(window.window, mouseInput);
    glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glDisable(GL_BLEND);

    // -- Shader --
    // Shader raytracer("assets/raytracer.vert", "assets/raytracer.frag", ShaderType::PATH);
    Shader pass("assets/pass.vert", "assets/pass.frag", ShaderType::PATH);
    Shader raytracer("assets/raytracer.comp");

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

    // scene.spheres.push_back({{2.f, 1.f, -9.f}, 1.0f, {0.f, 0.f, 1.f}, 1.f, {1.f, 0.f, 0.f, 0.f}});
    // scene.spheres.push_back({{5.0f, 0.5f, -8.f}, 1.0f, {1.f, 0.f, 0.f}, 1.f, {0.f, 0.f, 0.f, 0.f}});
    // scene.spheres.push_back({{2.f, -15.0f, -10.f}, 15.0f, {1.f, 0.f, 1.f}, 0.f, {0.f, 0.f, 0.f, 0.f}});
    scene.spheres.push_back({{-15.f, 15.0f, 0.f}, 7.5f, {0.f, 0.f, 0.f}, 0.f, {1.f, 1.f, 1.f, 5.f}});

    scene.meshes.push_back(loadMesh("assets/models/dragon8k.obj", GPUMaterial{{1.f, 1.f, 1.f}, 0.f, {0.f, 0.f, 0.f, 0.f}}, Transform{{10.f, 2.f, 0.f}, {}, glm::vec3(5)}, scene.materials));

    scene.meshes.push_back(loadRect({{{0, 0, -12.5f}, {0, 0, 0}, {20, .5f, 20}}, {{1, 1, 1}, 0.f, {0, 0, 0, 0}}}, scene));
    scene.spheres.push_back({{3, 1.5, 1.25f}, 1.0f, {1.f, 0.f, 0.f}, 1.f, {0.f, 0.f, 0.f, 0.f}});
    scene.spheres.push_back({{3, 1.5, -1.25f}, 1.0f, {0.f, 1.f, 0.f}, 1.f, {0.f, 0.f, 0.f, 0.f}});
    scene.spheres.push_back({{8, 1.5, 0.f}, 1.0f, {0.f, 0.f, 1.f}, 1.f, {0.f, 0.f, 0.f, 0.f}});

    // -- SSBO's --
    uint32_t meshCount = scene.meshes.size();
    uint32_t sphereCount = scene.spheres.size();

    GLuint sphereSSBO;
    glGenBuffers(1, &sphereSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sphereCount * sizeof(GPUSphere),
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

    BVH bvh(triangles); // reorders the triangles

    GLuint triSSBO;
    glGenBuffers(1, &triSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        triangles.size() * sizeof(GPUTriangle),
        triangles.data(),
        GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, triSSBO);

    GLuint bvhSSBO;
    glGenBuffers(1, &bvhSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        bvh.nodes.size() * sizeof(BVH::GPUNode),
        bvh.nodes.data(),
        GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bvhSSBO);

    struct GPUSceneData
    {
        uint32_t maxBounce;
        uint32_t numRaysPerPixel;
    } sceneData{5, 1}; // maxBounce, numRaysPerPixel

    GLuint dataSSBO;
    glGenBuffers(1, &dataSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, dataSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPUSceneData),
        &sceneData,
        GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, dataSSBO);

    // -- Frame Accumulation --

    GLuint accumTex;
    glGenTextures(1, &accumTex);
    glBindTexture(GL_TEXTURE_2D, accumTex);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        SCR_WIDTH,
        SCR_HEIGHT,
        0,
        GL_RGBA,
        GL_FLOAT,
        nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindImageTexture(
        0,
        accumTex,
        0,
        GL_FALSE,
        0,
        GL_READ_WRITE,
        GL_RGBA32F);

    while (!glfwWindowShouldClose(window.window))
    {
        glfwPollEvents();

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        fps = 1.0f / deltaTime;

        getInput(window.window);

        if (cameraDirty)
        {
            frameIndex = 0;
            cameraDirty = false;
        }

        // imgui stuff
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(SCR_WIDTH / 20.0f, SCR_HEIGHT / 30.0f), ImGuiCond_Always);
        ImGui::Begin("Stats Panel", nullptr,
                     ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("FPS: %.0f", fps);
        // ImGui::Text("%u", frameIndex);
        // ImGui::Text("%f0 %f1 %f2", camera.cameraPos[0], camera.cameraPos[1], camera.cameraPos[2]);
        // ImGui::Text("%.0f", camera.pitch);
        // ImGui::Text("%.0f", camera.yaw);
        // ImGui::Text("DT: %.3f", deltaTime);
        ImGui::End();

        // start compute
        glUseProgram(raytracer.ID);

        glUniform1ui(
            glGetUniformLocation(raytracer.ID, "frameIndex"),
            frameIndex);

        raytracer.setVec3("cameraPos", camera.cameraPos);
        raytracer.setVec3("cameraFront", camera.cameraFront);
        raytracer.setVec3("cameraUp", camera.cameraUp);
        raytracer.setFloat("fov", camera.fov);
        raytracer.setVec2("resolution", glm::vec2(SCR_WIDTH, SCR_HEIGHT));

        glUniform1ui(
            glGetUniformLocation(raytracer.ID, "sphereCount"),
            sphereCount);

        glDispatchCompute(
            (SCR_WIDTH + 7) / 8,
            (SCR_HEIGHT + 7) / 8,
            1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // used for shared frames

        // start draw
        ImGui::Render();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(pass.ID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, accumTex);
        glUniform1i(glGetUniformLocation(pass.ID, "accumTex"), 0);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        frameIndex++; // * Comment out to disable accumulation

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window.window);
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
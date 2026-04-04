#include <glad/glad.h>
#include <GLFW/glfw3.h> // ! Must be included after GLAD (due to method overriding).
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <stdint.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "viewport.h"
#include "bvh.h"

Viewport::Viewport(std::unique_ptr<Window> window, std::unique_ptr<Camera> camera, std::weak_ptr<const Scene> scene) : m_window(std::move(window)), m_camera(std::move(camera)), m_scene(scene)
{
    if (!m_window)
    {
        glfwTerminate();
        throw std::runtime_error("Viewport created without a valid window.");
    }

    m_rawWindow = m_window->window;

    glfwSetWindowUserPointer(m_rawWindow, this);

    glfwSetCursorPosCallback(m_rawWindow, Viewport::cursorPosCallback);
    glfwSetInputMode(m_rawWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glDisable(GL_BLEND);

    m_passthrough = Shader("assets/pass.vert", "assets/pass.frag", ShaderType::PATH);
    m_raytrace = Shader("assets/raytracer.comp");

    m_mouseLastX = static_cast<float>(m_window->SCR_WIDTH) / 2.0;
    m_mouseLastY = static_cast<float>(m_window->SCR_HEIGHT) / 2.0;

    float quad[] = {
        -1.f, -1.f,
        1.f, -1.f,
        -1.f, 1.f,
        -1.f, 1.f,
        1.f, -1.f,
        1.f, 1.f};

    glGenVertexArrays(1, &m_quadVAO);
    glBindVertexArray(m_quadVAO);

    glGenBuffers(1, &m_quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    rebuildScene();
}

Viewport::~Viewport()
{
    glDeleteVertexArrays(1, &m_quadVAO);
    glDeleteBuffers(1, &m_quadVBO);
    glDeleteBuffers(1, &m_sphereSSBO); // Don't forget these!
    glDeleteBuffers(1, &m_matSSBO);
    glDeleteTextures(1, &m_accumTexture);
    glDeleteProgram(m_raytrace.ID);
}

void Viewport::update()
{
    float currentFrame = (float)glfwGetTime();
    m_deltaTime = currentFrame - m_lastFrame;
    m_lastFrame = currentFrame;

    processKeyInput();

    processGui();

    glUseProgram(m_raytrace.ID);

    glUniform1ui(
        glGetUniformLocation(m_raytrace.ID, "frameIndex"),
        m_accumFrameIndex);

    m_raytrace.setVec3("cameraPos", m_camera->cameraPos_);
    m_raytrace.setVec3("cameraFront", m_camera->cameraFront_);
    m_raytrace.setVec3("cameraUp", m_camera->cameraUp_);
    m_raytrace.setFloat("fov", m_camera->fov_);

    glDispatchCompute(
        (m_window->SCR_WIDTH + 15) / 16,
        (m_window->SCR_HEIGHT + 15) / 16,
        1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // needed for shared frames

    ImGui::Render();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_window->SCR_WIDTH, m_window->SCR_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_passthrough.ID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_accumTexture);
    glUniform1i(glGetUniformLocation(m_passthrough.ID, "accumTex"), 0);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    m_accumFrameIndex++; // * Comment out to disable accumulation

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_rawWindow);

    glfwPollEvents();
}

void Viewport::processGui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(m_window->SCR_WIDTH, m_window->SCR_HEIGHT / 30.0f), ImGuiCond_Always);
    ImGui::Begin("Stats Panel", nullptr,
                 ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("FPS: %.0f", 1.0f / m_deltaTime);
    ImGui::End();
}

void Viewport::processKeyInput()
{
    if (glfwGetKey(m_rawWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_rawWindow, true);

    glm::vec3 oldPos = m_camera->cameraPos_;
    float oldYaw = m_camera->yaw_;
    float oldPitch = m_camera->pitch_;

    m_camera->handleKeyInput(m_rawWindow, m_deltaTime);

    if (m_camera->cameraPos_ != oldPos ||
        m_camera->yaw_ != oldYaw ||
        m_camera->pitch_ != oldPitch)
        m_accumFrameIndex = 0;
}

void Viewport::rebuildScene()
{
    std::shared_ptr<const Scene> pScene = m_scene.lock();

    if (!pScene)
    {
        std::cerr << "Locking scene failed during rebuild!\n";
        return;
    }

    if (m_sphereSSBO)
        glDeleteBuffers(1, &m_sphereSSBO);
    glGenBuffers(1, &m_sphereSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sphereSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        pScene->spheres.size() * sizeof(GPUSphere),
        pScene->spheres.data(),
        GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_sphereSSBO);

    if (m_matSSBO)
        glDeleteBuffers(1, &m_matSSBO);
    glGenBuffers(1, &m_matSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_matSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        pScene->materials.size() * sizeof(GPUMaterial),
        pScene->materials.data(),
        GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_matSSBO);

    std::vector<GPUTriangle> triangles;
    std::vector<GPUMesh> gpuMeshes;
    convertToGPUMeshes(*pScene, triangles, gpuMeshes);

    BVH bvh(triangles);

    if (m_triSSBO)
        glDeleteBuffers(1, &m_triSSBO);
    glGenBuffers(1, &m_triSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_triSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        bvh.triangles.size() * sizeof(GPUTriangle),
        bvh.triangles.data(),
        GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_triSSBO);

    if (m_bvhSSBO)
        glDeleteBuffers(1, &m_bvhSSBO);
    glGenBuffers(1, &m_bvhSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_bvhSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        bvh.nodes.size() * sizeof(BVH::GPUNode),
        bvh.nodes.data(),
        GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_bvhSSBO);

    struct GPUSceneData
    {
        uint32_t maxBounce;
        uint32_t numRaysPerPixel;
    } sceneData{5, 1}; // maxBounce, numRaysPerPixel

    if (m_dataSSBO)
        glDeleteBuffers(1, &m_dataSSBO);
    glGenBuffers(1, &m_dataSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_dataSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(GPUSceneData),
        &sceneData,
        GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_dataSSBO);

    // -- Frame Accumulation --
    glGenTextures(1, &m_accumTexture);
    glBindTexture(GL_TEXTURE_2D, m_accumTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        m_window->SCR_WIDTH,
        m_window->SCR_HEIGHT,
        0,
        GL_RGBA,
        GL_FLOAT,
        nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindImageTexture(
        0,
        m_accumTexture,
        0,
        GL_FALSE,
        0,
        GL_READ_WRITE,
        GL_RGBA32F);
}

bool Viewport::shouldClose() const
{
    return glfwWindowShouldClose(m_rawWindow);
}

void Viewport::cursorPosCallback(GLFWwindow *window, double xposd, double yposd)
{
    Viewport *instance = static_cast<Viewport *>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->m_camera->handleMouseInput(
            instance->m_accumFrameIndex,
            instance->m_deltaTime,
            static_cast<float>(xposd),
            static_cast<float>(yposd),
            instance->m_mouseLastX,
            instance->m_mouseLastY);
    }
}

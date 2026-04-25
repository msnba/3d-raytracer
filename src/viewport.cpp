#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <portable-file-dialogs.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <iostream>
#include <iomanip>
#include <vector>
#include <future>
#include <stdexcept>
#include <stdint.h>
#include <functional>

#include "viewport.h"
#include "bvh.h"
#include "gui.h"
#include "shader_sources.h"
#include "log.h"

Viewport::Viewport(std::unique_ptr<Window> window, std::unique_ptr<Camera> camera, std::unique_ptr<GUI> gui, std::weak_ptr<Scene> scene) : window_(std::move(window)), camera_(std::move(camera)), gui_(std::move(gui)), scene_(scene), rawWindow_(window_->window), passthrough_(PASS_VERT, PASS_FRAG), raytrace_(RAYTRACER_COMP)
{
    if (!window_)
    {
        glfwTerminate();
        throw std::runtime_error("Viewport created without a valid window.");
    }

    glfwSetFramebufferSizeCallback(rawWindow_, framebufferSizeCallback);
    glfwSetWindowUserPointer(rawWindow_, this);
    glfwSetScrollCallback(rawWindow_, Viewport::scrollCallback);
    glDisable(GL_BLEND);

    mouseLastX_ = static_cast<float>(window_->SCR_WIDTH) / 2.0f;
    mouseLastY_ = static_cast<float>(window_->SCR_HEIGHT) / 2.0f;

    float quad[] = {
        -1.f, -1.f,
        1.f, -1.f,
        -1.f, 1.f,
        -1.f, 1.f,
        1.f, -1.f,
        1.f, 1.f};

    glGenVertexArrays(1, &quadVAO_);
    glBindVertexArray(quadVAO_);

    glGenBuffers(1, &quadVBO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);

    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    gui_->setCallbacks({.onMaxBounceChanged = [this](uint32_t maxBounce)
                        { maxBounce_ = maxBounce; rebuildScene(RebuildFlags::SceneData); },
                        .onRaysPerPixelChanged = [this](uint32_t numRaysPerPixel)
                        { numRaysPerPixel_ = numRaysPerPixel; rebuildScene({RebuildFlags::SceneData}); },
                        .onSSAAChanged = [this](bool isSSAAEnabled)
                        { isSSAAEnabled_ = isSSAAEnabled ? 1 : 0; rebuildScene(RebuildFlags::SceneData); },
                        .onScreenshot = [this]()
                        { isScreenshot_ = true; },
                        .onToggleFullscreen = [this]()
                        { fullscreenPressed_ = true; },
                        .onAccumulationChanged = [this](bool isAccumulationEnabled)
                        { isAccumulationEnabled_ = isAccumulationEnabled; }

    });

    rebuildScene(RebuildFlags::All);
}

Viewport::~Viewport()
{
    glDeleteVertexArrays(1, &quadVAO_);
    glDeleteBuffers(1, &quadVBO_);
    glDeleteBuffers(1, &sphereSSBO_);
    glDeleteBuffers(1, &matSSBO_);
    glDeleteTextures(1, &accumTexture_);
}

void Viewport::update()
{
    float currentFrame = (float)glfwGetTime();
    deltaTime_ = currentFrame - lastFrame_;
    lastFrame_ = currentFrame;

    processKeyInput();
    processGui();

    raytrace_.use();
    raytrace_.set("frameIndex", accumFrameIndex_);
    raytrace_.set("cameraPos", camera_->cameraPos_);
    raytrace_.set("cameraFront", camera_->cameraFront_);
    raytrace_.set("fov", camera_->fov_);

    glDispatchCompute(
        (window_->SCR_WIDTH + 15) / 16,
        (window_->SCR_HEIGHT + 15) / 16,
        1);

    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, static_cast<GLsizei>(window_->SCR_WIDTH), static_cast<GLsizei>(window_->SCR_HEIGHT));

    passthrough_.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, accumTexture_);
    passthrough_.set("accumTex", 0);
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    if (isScreenshot_)
        saveScreenshot();

    if (fullscreenPressed_ && !isFullscreen_)
    {
        isFullscreen_ = true;
        toggleFullscreen();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (isAccumulationEnabled_ && !isPanning_ && !isMoving_)
        accumFrameIndex_++;
    else
        accumFrameIndex_ = 0;
    glfwSwapBuffers(rawWindow_);
    glfwPollEvents();
}

void Viewport::processGui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    gui_->render({.accumFrameIndex = accumFrameIndex_, .fpsString = getFPS(), .fov = camera_->fov_});
}

void Viewport::rebuildScene(RebuildFlags flags)
{
    Log::info("rebuilt scene");

    accumFrameIndex_ = 0;

    std::shared_ptr<Scene> pScene = scene_.lock();

    if (!pScene)
        throw std::runtime_error("Locking scene failed during rebuild.");

    if (hasFlag(flags, RebuildFlags::Spheres))
    {
        if (sphereSSBO_)
            glDeleteBuffers(1, &sphereSSBO_);
        glGenBuffers(1, &sphereSSBO_);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSSBO_);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<long int>(pScene->spheres.size() * sizeof(GPUSphere)),
            pScene->spheres.data(),
            GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sphereSSBO_);
    }

    if (hasFlag(flags, RebuildFlags::Spheres))
    {
        if (matSSBO_)
            glDeleteBuffers(1, &matSSBO_);
        glGenBuffers(1, &matSSBO_);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, matSSBO_);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<long int>(pScene->materials.size() * sizeof(GPUMaterial)),
            pScene->materials.data(),
            GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, matSSBO_);
    }

    if (hasFlag(flags, RebuildFlags::Geometry))
    {
        std::vector<GPUTriangle> triangles;
        std::vector<GPUMesh> gpuMeshes;
        convertToGPUMeshes(*pScene, triangles, gpuMeshes);
        BVH bvh(triangles);
        if (triSSBO_)
            glDeleteBuffers(1, &triSSBO_);
        glGenBuffers(1, &triSSBO_);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, triSSBO_);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<long int>(bvh.triangles_.size() * sizeof(GPUTriangle)),
            bvh.triangles_.data(),
            GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, triSSBO_);

        if (bvhSSBO_)
            glDeleteBuffers(1, &bvhSSBO_);
        glGenBuffers(1, &bvhSSBO_);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhSSBO_);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<long int>(bvh.nodes_.size() * sizeof(BVH::GPUNode)),
            bvh.nodes_.data(),
            GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bvhSSBO_);
    }

    if (hasFlag(flags, RebuildFlags::SceneData))
    {
        struct GPUSceneData
        {
            uint32_t maxBounce;
            uint32_t numRaysPerPixel;
            uint32_t SSAA;
        } sceneData{maxBounce_, numRaysPerPixel_, isSSAAEnabled_};

        if (dataSSBO_)
            glDeleteBuffers(1, &dataSSBO_);
        glGenBuffers(1, &dataSSBO_);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, dataSSBO_);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            sizeof(GPUSceneData),
            &sceneData,
            GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, dataSSBO_);
    }

    glGenTextures(1, &accumTexture_);
    glBindTexture(GL_TEXTURE_2D, accumTexture_);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        static_cast<GLsizei>(window_->SCR_WIDTH),
        static_cast<GLsizei>(window_->SCR_HEIGHT),
        0,
        GL_RGBA,
        GL_FLOAT,
        nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindImageTexture(
        0,
        accumTexture_,
        0,
        GL_FALSE,
        0,
        GL_READ_WRITE,
        GL_RGBA32F);
}

bool Viewport::shouldClose() const
{
    return glfwWindowShouldClose(rawWindow_);
}

std::string Viewport::getFPS()
{
    fpsTimer_ += deltaTime_;
    fpsFrameCount_++;

    if (fpsTimer_ >= fpsInterval_)
    {
        fpsString_ = "FPS: " + std::to_string(static_cast<int>(static_cast<float>(fpsFrameCount_) / fpsTimer_));

        fpsTimer_ = 0.0f;
        fpsFrameCount_ = 0;
    }

    return fpsString_;
}

void Viewport::saveScreenshot()
{
    if (screenshotInProgress_)
        return;

    isScreenshot_ = false;
    screenshotInProgress_ = true;

    int w = static_cast<int>(window_->SCR_WIDTH);
    int h = static_cast<int>(window_->SCR_HEIGHT);

    auto pixels = std::make_shared<std::vector<uint8_t>>(static_cast<size_t>(w * h * 3));
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels->data());

    std::thread([this, pixels, w, h]()
                {
                    std::string destination = pfd::save_file("Save Screenshot", "screenshot.png",
                                                             {"PNG Files", "*.png"})
                                                  .result();
                    if (!destination.empty())
                    {
                        if (destination.size() < 4 || destination.substr(destination.size() - 4) != ".png")
                            destination += ".png";

                        stbi_flip_vertically_on_write(1);
                        stbi_write_png(destination.c_str(), w, h, 3, pixels->data(), w * 3);
                        std::cout << "Screenshot saved in " << destination << "\n";
                    }

                    screenshotInProgress_ = false; })
        .detach();
}

void Viewport::toggleFullscreen()
{
    if (!isFullscreen_)
        return;

    window_->toggleFullscreen();
    accumFrameIndex_ = 0;
}

void Viewport::processKeyInput()
{
    static bool F1Pressed = false;
    // Keybinds
    if (glfwGetKey(rawWindow_, GLFW_KEY_Q) == GLFW_PRESS && (glfwGetKey(rawWindow_, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(rawWindow_, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) // Ctrl + Q
        glfwSetWindowShouldClose(rawWindow_, true);

    if (glfwGetKey(rawWindow_, GLFW_KEY_F1) == GLFW_PRESS && !F1Pressed)
    {
        Log::info("f1 pressed");
        F1Pressed = true;
        gui_->toggleRender();
    }

    if (glfwGetKey(rawWindow_, GLFW_KEY_F1) == GLFW_RELEASE)
        F1Pressed = false;

    if (glfwGetKey(rawWindow_, GLFW_KEY_F11) == GLFW_PRESS) // F11
        fullscreenPressed_ = true;

    if (glfwGetKey(rawWindow_, GLFW_KEY_F11) == GLFW_RELEASE)
        fullscreenPressed_ = false;

    if (glfwGetKey(rawWindow_, GLFW_KEY_F12) == GLFW_PRESS && !isScreenshot_ && !screenshotInProgress_)
        isScreenshot_ = true;

    // Panning & Moving Logic
    if (glfwGetMouseButton(rawWindow_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && !isPanning_)
    {
        // prevents camera snapping to cursor
        double cursorX, cursorY;
        glfwGetCursorPos(rawWindow_, &cursorX, &cursorY);
        mouseLastX_ = static_cast<float>(cursorX);
        mouseLastY_ = static_cast<float>(cursorY);

        glfwSetCursorPosCallback(rawWindow_, Viewport::cursorPosCallback);
        glfwSetInputMode(rawWindow_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        isPanning_ = true;
    }

    if (glfwGetMouseButton(rawWindow_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE && isPanning_)
    {
        glfwSetCursorPosCallback(rawWindow_, ImGui_ImplGlfw_CursorPosCallback);
        glfwSetInputMode(rawWindow_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        isPanning_ = false;
    }

    glm::vec3 oldPos = camera_->cameraPos_;

    camera_->handleKeyInput(rawWindow_, deltaTime_);

    isMoving_ = camera_->cameraPos_ != oldPos && !isMoving_;
}

void Viewport::framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    Viewport *instance = static_cast<Viewport *>(glfwGetWindowUserPointer(window));
    if (instance)
    {
        instance->accumFrameIndex_ = 0;

        instance->window_->SCR_WIDTH = static_cast<unsigned int>(width);
        instance->window_->SCR_HEIGHT = static_cast<unsigned int>(height);
        glViewport(0, 0, width, height);

        glBindTexture(GL_TEXTURE_2D, instance->accumTexture_);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA32F,
            width,
            height,
            0,
            GL_RGBA,
            GL_FLOAT,
            nullptr);

        glBindImageTexture(
            0,
            instance->accumTexture_,
            0,
            GL_FALSE,
            0,
            GL_READ_WRITE,
            GL_RGBA32F);
    }
}

void Viewport::cursorPosCallback(GLFWwindow *window, double xposd, double yposd)
{
    Viewport *instance = static_cast<Viewport *>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->camera_->handleMouseInput(
            instance->accumFrameIndex_,
            static_cast<float>(xposd),
            static_cast<float>(yposd),
            instance->mouseLastX_,
            instance->mouseLastY_);
    }
}

void Viewport::scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    Viewport *instance = static_cast<Viewport *>(glfwGetWindowUserPointer(window));

    if (instance && instance->camera_->handleScrollInput(xoffset, yoffset))
        instance->accumFrameIndex_ = 0;
}

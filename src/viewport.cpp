#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <future>
#include <stdexcept>
#include <stdint.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <portable-file-dialogs.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "viewport.h"
#include "bvh.h"
#include "gui.h"
#include "shader_sources.h"

Viewport::Viewport(std::unique_ptr<Window> window, std::unique_ptr<Camera> camera, std::unique_ptr<GUI> gui, std::weak_ptr<Scene> scene) : window_(std::move(window)), camera_(std::move(camera)), gui_(std::move(gui)), scene_(scene), rawWindow_(window_->window), passthrough_(PASS_VERT, PASS_FRAG), raytrace_(RAYTRACER_COMP)
{
    if (!window_)
    {
        glfwTerminate();
        throw std::runtime_error("Viewport created without a valid window.");
    }

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

    rebuildScene();
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

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (!isPanning_ && !isMoving_)
        accumFrameIndex_++; // * Comment out to disable accumulation
    else if (accumFrameIndex_ != 0)
        accumFrameIndex_ = 0;
    glfwSwapBuffers(rawWindow_);
    glfwPollEvents();
}

void Viewport::processGui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    gui_->render({.accumFrameIndex_ = accumFrameIndex_, .deltaTime_ = deltaTime_, .fpsString_ = getFPS(), .fov_ = camera_->fov_});
}

void Viewport::rebuildScene()
{
    std::shared_ptr<Scene> pScene = scene_.lock();

    if (!pScene)
    {
        std::cerr << "Locking scene failed during rebuild!\n";
        return;
    }

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
        static_cast<long int>(bvh.triangles.size() * sizeof(GPUTriangle)),
        bvh.triangles.data(),
        GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, triSSBO_);

    if (bvhSSBO_)
        glDeleteBuffers(1, &bvhSSBO_);
    glGenBuffers(1, &bvhSSBO_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhSSBO_);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        static_cast<long int>(bvh.nodes.size() * sizeof(BVH::GPUNode)),
        bvh.nodes.data(),
        GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bvhSSBO_);

    struct GPUSceneData
    {
        uint32_t maxBounce;
        uint32_t numRaysPerPixel;
    } sceneData{5, 1};

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

    // -- Frame Accumulation --
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
        // fpsString_ = std::to_string(gui_->get("test", 3.0f));

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

void Viewport::processKeyInput()
{
    // Keybinds
    if (glfwGetKey(rawWindow_, GLFW_KEY_Q) == GLFW_PRESS && (glfwGetKey(rawWindow_, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(rawWindow_, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) // Ctrl + Q
        glfwSetWindowShouldClose(rawWindow_, true);

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

    if (instance)
    {
        instance->camera_->handleScrollInput(xoffset, yoffset);
        instance->accumFrameIndex_ = 0;
    }
}

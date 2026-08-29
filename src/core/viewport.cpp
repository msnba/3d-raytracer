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
#include <stdexcept>
#include <stdint.h>
#include <vector>

#include "raw_sources.h"
#include "tinytracer/core/file_picker.h"
#include "tinytracer/core/viewport.h"
#include "tinytracer/geometry/bvh.h"
#include "tinytracer/utils/log.h"
#include "tinytracer/utils/settings.h"

namespace tinytracer::core {

Viewport::Viewport(std::unique_ptr<tinytracer::core::Window> window,
                   std::unique_ptr<tinytracer::world::Camera> camera,
                   std::unique_ptr<GUI> gui,
                   std::unique_ptr<tinytracer::world::Scene> scene)
    : window_(std::move(window)), camera_(std::move(camera)),
      gui_(std::move(gui)), scene_(std::move(scene)),
      rawWindow_(window_->window), passthrough_(PASS_VERT, PASS_FRAG),
      raytrace_(RAYTRACER_COMP) {
  if (!window_) {
    glfwTerminate();
    throw std::runtime_error("Viewport created without a valid window.");
  }

  glfwSetFramebufferSizeCallback(rawWindow_, framebufferSizeCallback);
  glfwSetWindowUserPointer(rawWindow_, this);
  glfwSetScrollCallback(rawWindow_, Viewport::scrollCallback);
  glDisable(GL_BLEND);

  mouseLastX_ = static_cast<float>(window_->SCR_WIDTH) / 2.0f;
  mouseLastY_ = static_cast<float>(window_->SCR_HEIGHT) / 2.0f;

  float quad[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f,
                  -1.f, 1.f,  1.f, -1.f, 1.f,  1.f};

  glGenVertexArrays(1, &quadVAO_);
  glBindVertexArray(quadVAO_);
  glGenBuffers(1, &quadVBO_);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  ViewportCallbacks callbacks{};

  callbacks.set("maxBounceChanged", [this](uint32_t maxBounce) {
    auto s = scene_->settings();
    s.maxBounce = maxBounce;
    scene_->setSettings(s);

    sceneUploader_.upload(*scene_,
                          tinytracer::renderer::RebuildFlags::SceneData);
    accumFrameIndex_ = 0;
  });

  callbacks.set("raysPerPixelChanged", [this](uint32_t numRaysPerPixel) {
    auto s = scene_->settings();
    s.numRaysPerPixel = numRaysPerPixel;
    scene_->setSettings(s);

    sceneUploader_.upload(*scene_,
                          tinytracer::renderer::RebuildFlags::SceneData);
    accumFrameIndex_ = 0;
  });

  callbacks.set("SSAAChanged", [this](bool isSSAAEnabled) {
    auto s = scene_->settings();
    s.isSSAAEnabled = isSSAAEnabled ? 1u : 0u;
    scene_->setSettings(s);

    sceneUploader_.upload(*scene_,
                          tinytracer::renderer::RebuildFlags::SceneData);
    accumFrameIndex_ = 0;
  });
  callbacks.set("screenshot", [this]() { isScreenshot_ = true; });

  callbacks.set("toggleFullscreen", [this]() { fullscreenPressed_ = true; });

  callbacks.set("accumulationChanged", [this](bool isAccumulationEnabled) {
    isAccumulationEnabled_ = isAccumulationEnabled;
  });

  callbacks.set("settingsLoaded", [this]() {
    isAccumulationEnabled_ = tinytracer::utils::Settings::get().getValue(
        "accumulationDefaultEnabled", true);
  });

  callbacks.set("sceneLoaded", [this](std::string destination) {
    if (scene_->loadFromFile(destination, true))
      pendingRebuild_ =
          pendingRebuild_ | tinytracer::renderer::RebuildFlags::All;
  });

  callbacks.set("sceneSaved", [this](std::optional<std::string> destination) {
    if (destination)
      scene_->saveToFile(*destination);
    else
      scene_->saveToFile();
  });

  callbacks.set(
      "sceneDataChanged", [this](tinytracer::world::Scene::Settings s) {
        scene_->setSettings(s);
        pendingRebuild_ =
            pendingRebuild_ | tinytracer::renderer::RebuildFlags::SceneData;
      });

  // setup for future gizmo
  callbacks.set("objectTransformChanged",
                [this](int index, tinytracer::world::Object::Transform t) {
                  scene_->objects()[static_cast<size_t>(index)]->transform_ = t;
                  pendingRebuild_ =
                      pendingRebuild_ |
                      tinytracer::renderer::RebuildFlags::Transforms;
                });

  gui_->attachWindow(rawWindow_);
  gui_->setCallbacks(callbacks);

  sceneUploader_.upload(*scene_, tinytracer::renderer::RebuildFlags::All);
  rebuildAccumTexture();

  isAccumulationEnabled_ = tinytracer::utils::Settings::get().getValue(
      "accumulationDefaultEnabled", true);
}

Viewport::~Viewport() {
  glDeleteVertexArrays(1, &quadVAO_);
  glDeleteBuffers(1, &quadVBO_);
  glDeleteTextures(1, &accumTexture_);
}

void Viewport::update() {
  float currentFrame = (float)glfwGetTime();
  deltaTime_ = currentFrame - lastFrame_;
  lastFrame_ = currentFrame;

  if (pendingRebuild_ != tinytracer::renderer::RebuildFlags::None) {
    sceneUploader_.upload(*scene_, pendingRebuild_);
    pendingRebuild_ = tinytracer::renderer::RebuildFlags::None;
    accumFrameIndex_ = 0;
  }

  processKeyInput();
  processGui();

  raytrace_.use();
  raytrace_.set("frameIndex", accumFrameIndex_);
  raytrace_.set("cameraPos", camera_->cameraPos_);
  raytrace_.set("cameraFront", camera_->cameraFront_);
  raytrace_.set("fov", camera_->fov_);

  glDispatchCompute((window_->SCR_WIDTH + 15) / 16,
                    (window_->SCR_HEIGHT + 15) / 16, 1);

  glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, static_cast<GLsizei>(window_->SCR_WIDTH),
             static_cast<GLsizei>(window_->SCR_HEIGHT));

  passthrough_.use();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, accumTexture_);
  passthrough_.set("accumTex", 0);
  glBindVertexArray(quadVAO_);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  if (isScreenshot_)
    saveScreenshot();

  if (fullscreenPressed_ && !isFullscreen_) {
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

void Viewport::processGui() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ViewportData data{};

  data.accumFrameIndex = accumFrameIndex_;
  data.rawFps = 1.0f / deltaTime_;
  data.smoothFps = smoothFPS();
  data.fov = camera_->fov_;
  data.scene = (scene_ && scene_->objectCount() > 0) ? scene_.get() : nullptr;

  gui_->render(data);
}

void Viewport::rebuildAccumTexture() {
  if (accumTexture_)
    glDeleteTextures(1, &accumTexture_);

  glGenTextures(1, &accumTexture_);
  glBindTexture(GL_TEXTURE_2D, accumTexture_);
  glTexImage2D(
      GL_TEXTURE_2D, 0, GL_RGBA32F, static_cast<GLsizei>(window_->SCR_WIDTH),
      static_cast<GLsizei>(window_->SCR_HEIGHT), 0, GL_RGBA, GL_FLOAT, nullptr);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glBindImageTexture(0, accumTexture_, 0, GL_FALSE, 0, GL_READ_WRITE,
                     GL_RGBA32F);
}

bool Viewport::shouldClose() const { return glfwWindowShouldClose(rawWindow_); }

float Viewport::smoothFPS() {
  static float fpsTimer_ = 0.0f;
  static int fpsFrameCount_ = 0;
  static float smoothFps = 0.0f;

  fpsTimer_ += deltaTime_;
  fpsFrameCount_++;

  if (fpsTimer_ >= fpsInterval_) {
    smoothFps = static_cast<float>(fpsFrameCount_) / fpsTimer_;

    fpsTimer_ = 0.0f;
    fpsFrameCount_ = 0;
  }

  return smoothFps;
}

void Viewport::saveScreenshot() {
  if (screenshotInProgress_)
    return;

  isScreenshot_ = false;
  screenshotInProgress_ = true;

  int w = static_cast<int>(window_->SCR_WIDTH);
  int h = static_cast<int>(window_->SCR_HEIGHT);

  auto pixels =
      std::make_shared<std::vector<uint8_t>>(static_cast<size_t>(w * h * 3));
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels->data());

  FilePicker::get().query({"png", "screenshot", "Save Screenshot"}, true,
                          [this, pixels, w, h](const std::string &destination) {
                            if (!destination.empty()) {
                              stbi_flip_vertically_on_write(1);
                              stbi_write_png(destination.c_str(), w, h, 3,
                                             pixels->data(), w * 3);
                            }
                            screenshotInProgress_ = false;
                          });
}

void Viewport::toggleFullscreen() {
  if (!isFullscreen_)
    return;

  window_->toggleFullscreen();
  accumFrameIndex_ = 0;
}

void Viewport::processKeyInput() {
  static bool F1Pressed = false;
  // Keybinds
  if (glfwGetKey(rawWindow_, GLFW_KEY_Q) == GLFW_PRESS &&
      (glfwGetKey(rawWindow_, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
       glfwGetKey(rawWindow_, GLFW_KEY_RIGHT_CONTROL) ==
           GLFW_PRESS)) // Ctrl + Q
    glfwSetWindowShouldClose(rawWindow_, true);

  if (glfwGetKey(rawWindow_, GLFW_KEY_F1) == GLFW_PRESS && !F1Pressed) {
    F1Pressed = true;
    gui_->toggleRender();
  }

  if (glfwGetKey(rawWindow_, GLFW_KEY_F1) == GLFW_RELEASE)
    F1Pressed = false;

  if (glfwGetKey(rawWindow_, GLFW_KEY_F11) == GLFW_PRESS) // F11
    fullscreenPressed_ = true;

  if (glfwGetKey(rawWindow_, GLFW_KEY_F11) == GLFW_RELEASE)
    fullscreenPressed_ = false;

  if (glfwGetKey(rawWindow_, GLFW_KEY_F12) == GLFW_PRESS && !isScreenshot_ &&
      !screenshotInProgress_)
    isScreenshot_ = true;

  // Panning & Moving Logic
  if (glfwGetMouseButton(rawWindow_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS &&
      !isPanning_) {
    // prevents camera snapping to cursor
    double cursorX, cursorY;
    glfwGetCursorPos(rawWindow_, &cursorX, &cursorY);
    mouseLastX_ = static_cast<float>(cursorX);
    mouseLastY_ = static_cast<float>(cursorY);

    glfwSetCursorPosCallback(rawWindow_, Viewport::cursorPosCallback);
    glfwSetInputMode(rawWindow_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    isPanning_ = true;
  }

  if (glfwGetMouseButton(rawWindow_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE &&
      isPanning_) {
    glfwSetCursorPosCallback(rawWindow_, ImGui_ImplGlfw_CursorPosCallback);
    glfwSetInputMode(rawWindow_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    isPanning_ = false;
  }

  glm::vec3 oldPos = camera_->cameraPos_;

  camera_->handleKeyInput(rawWindow_, deltaTime_);

  isMoving_ = camera_->cameraPos_ != oldPos;
}

void Viewport::framebufferSizeCallback(GLFWwindow *window, int width,
                                       int height) {
  Viewport *instance =
      static_cast<Viewport *>(glfwGetWindowUserPointer(window));
  if (!instance)
    return;

  instance->accumFrameIndex_ = 0;
  instance->window_->SCR_WIDTH = static_cast<unsigned int>(width);
  instance->window_->SCR_HEIGHT = static_cast<unsigned int>(height);

  glViewport(0, 0, width, height);
  instance->rebuildAccumTexture();
}

void Viewport::cursorPosCallback(GLFWwindow *window, double xposd,
                                 double yposd) {
  Viewport *instance =
      static_cast<Viewport *>(glfwGetWindowUserPointer(window));

  if (!instance)
    return;

  instance->camera_->handleMouseInput(
      instance->accumFrameIndex_, static_cast<float>(xposd),
      static_cast<float>(yposd), instance->mouseLastX_, instance->mouseLastY_);
}

void Viewport::scrollCallback(GLFWwindow *window, double xoffset,
                              double yoffset) {
  Viewport *instance =
      static_cast<Viewport *>(glfwGetWindowUserPointer(window));

  if (instance && instance->camera_->handleScrollInput(xoffset, yoffset))
    instance->accumFrameIndex_ = 0;
}

} // namespace tinytracer::core
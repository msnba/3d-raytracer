#pragma once

#include <any>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "tinytracer/core/window.h"
#include "tinytracer/world/object.h"
#include "tinytracer/world/scene.h"

using SettingValue = std::variant<int, float, bool, std::string>;

namespace tinytracer::core {

struct ViewportData {
  uint32_t accumFrameIndex = 0;
  float rawFps = 0.0f;
  float smoothFps = 0.0f;
  float fov = 90.0f;
  const tinytracer::world::Scene *scene = nullptr;
};

struct ViewportCallbacks {

  template <typename Fn> void set(const std::string &name, Fn &&fn) {
    callbacks[name] = std::function(std::forward<Fn>(fn));
  }

  template <typename Fn> std::function<Fn> *get(const std::string &name) {
    if (auto it = callbacks.find(name); it != callbacks.end())
      return std::any_cast<std::function<Fn>>(&it->second);
    return nullptr;
  }

private:
  std::unordered_map<std::string, std::any> callbacks;
};

class GUI {
public:
  GUI();
  GUI(GLFWwindow *rawWindow);

  ~GUI();
  GUI(const GUI &) = delete;
  GUI &operator=(const GUI &) = delete;
  GUI(GUI &&other);
  GUI &operator=(GUI &&other);

  void render(const ViewportData &data);
  bool toggleRender();

  void setCallbacks(const ViewportCallbacks &callbacks);
  void attachWindow(GLFWwindow *rawWindow);

private:
  void renderMenuBar(float menuBarHeight);
  void destroySelf();
  void rebuildFontAtlas(float scale);

  GLFWwindow *rawWindow_ = nullptr;
  ViewportCallbacks callbacks_{};
  ImFont *currentFont_ = nullptr;

  float guiScale_ = 1.0f;
  bool renderGui_ = true;
  bool showTopBar_ = true;

  int selectedObjectIndex_ = -1;
};

} // namespace tinytracer::core
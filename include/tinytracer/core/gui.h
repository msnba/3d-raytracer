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

using SettingValue = std::variant<int, float, bool, std::string>;

namespace tinytracer::core {

//  TODO: refactor viewport data
struct ViewportData {
  uint32_t accumFrameIndex = 0;
  const float fpsInterval = 0.5f;
  std::string fpsString;
  float fov = 90.0f;
};

struct ViewportCallbacks {

  template <typename Fn> void set(const std::string &name, Fn &&fn) {
    callbacks[name] = std::function(std::forward<Fn>(fn));
  }

  template <typename Fn> std::function<Fn> *get(const std::string &name) {
    auto it = callbacks.find(name);
    if (it == callbacks.end())
      return nullptr;
    return std::any_cast<std::function<Fn>>(&it->second);
  }

private:
  std::unordered_map<std::string, std::any> callbacks;
};

class GUI {
public:
  GUI() = default;
  GUI(GLFWwindow *rawWindow);

  ~GUI();
  GUI(const GUI &) = delete;
  GUI &operator=(const GUI &) = delete;
  GUI(GUI &&other);
  GUI &operator=(GUI &&other);

  void render(const ViewportData &data);
  bool toggleRender();

  void setCallbacks(const ViewportCallbacks &callbacks);

private:
  void destroySelf();

  GLFWwindow *rawWindow_ = nullptr;
  ViewportCallbacks callbacks_{};
  ImFont *currentFont_ = nullptr;

  float guiScale_ = 1.0f;
  bool renderGui_ = true;
  bool showTopBar_ = true;
};

} // namespace tinytracer::core
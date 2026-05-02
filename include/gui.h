#pragma once

#include <unordered_map>
#include <string>
#include <variant>
#include <stdexcept>
#include <memory>
#include <functional>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "window.h"

using SettingValue = std::variant<int, float, bool, std::string>;

// i know this isn't the best way to pass data
//  TODO: refactor viewport data
struct ViewportData
{
    uint32_t accumFrameIndex = 0;
    const float fpsInterval = 0.5f;
    std::string fpsString;
    float fov = 90.0f;
};

struct ViewportCallbacks
{
    std::function<void(uint32_t)> onMaxBounceChanged = nullptr;
    std::function<void(uint32_t)> onRaysPerPixelChanged = nullptr;
    std::function<void(bool)> onSSAAChanged = nullptr;
    std::function<void()> onScreenshot = nullptr;
    std::function<void()> onToggleFullscreen = nullptr;
    std::function<void(bool)> onAccumulationChanged = nullptr;
    std::function<void()> onSettingsLoaded = nullptr;
};

class GUI
{
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
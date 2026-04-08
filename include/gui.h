#pragma once

#include <GLFW/glfw3.h>
#include <unordered_map>
#include <string>
#include <variant>
#include <stdexcept>
#include <memory>

#include "window.h"

using SettingValue = std::variant<int, float, bool, std::string>;

// i know this isn't the best way to pass data
//  TODO: refactor vieport data
struct ViewportData
{
    uint32_t accumFrameIndex_ = 0;
    float deltaTime_ = 0.0f;
    float lastFrame_ = 0.0f;
    int fpsFrameCount_ = 0;
    float lastFPS_ = 0.0f;
    float fpsTimer_ = 0.0f;
    const float fpsInterval_ = 0.5f;
    std::string fpsString_;
    bool *pIsScreenshot = nullptr;
    bool isPanning_ = false;
    bool isMoving_ = false;
    float fov_ = 90.0f;
};

class GUI
{
public:
    GUI() = default;
    GUI(GLFWwindow *rawWindow, const std::string &filepath);

    ~GUI();
    GUI(const GUI &) = delete;
    GUI &operator=(const GUI &) = delete;
    GUI(GUI &&other) noexcept;
    GUI &operator=(GUI &&other) noexcept;

    void render(const ViewportData &data);
    bool loadSettings(const std::string &filepath);
    bool saveSettings(const std::string &filepath);

    template <typename T_>
    void set(const std::string &key, T_ value)
    {
        settingsMap_[key] = SettingValue(value);
    }

    template <typename T_>
    T_ get(const std::string &key, T_ fallback) const
    {
        auto it = settingsMap_.find(key);
        if (it == settingsMap_.end())
            return fallback;

        if (auto *val = std::get_if<T_>(&it->second))
            return *val;

        throw std::runtime_error("Key \"" + key + "\" exists, but wrong type \"" + typeid(T_).name() + "\" requested.");
    }

private:
    static SettingValue parseValue(const std::string &value);
    void destroySelf();

    std::unordered_map<std::string, SettingValue> settingsMap_;
    GLFWwindow *rawWindow_ = nullptr;

    float guiScale_ = 1.0f;
    bool showTopBar_ = true;
    bool showStats_ = true;
};
#pragma once

#include <GLFW/glfw3.h>
#include <unordered_map>
#include <string>
#include <variant>
#include <stdexcept>
#include <memory>

#include "window.h"

using SettingValue = std::variant<int, float, bool, std::string>;

class GUI
{
public:
    GUI() = default;
    GUI(GLFWwindow *rawWindow, const std::string &filepath);

    void render(const std::string &fpsString);
    bool loadSettings(const std::string &filepath);
    bool saveSettings(const std::string &filepath);

    template <typename T_>
    void setSetting(const std::string &key, T_ value)
    {
        settingsMap[key] = SettingValue(value);
    }

    template <typename T_>
    T_ get(const std::string &key, T_ fallback) const
    {
        auto it = settingsMap.find(key);
        if (it == settingsMap.end())
            return fallback;

        if (auto *val = std::get_if<T_>(&it->second))
            return *val;

        throw std::runtime_error("Key \"" + key + "\" exists, but wrong type \"" + typeid(T_).name() + "\" requested.");
    }

private:
    std::unordered_map<std::string, SettingValue> settingsMap;
    GLFWwindow *rawWindow_ = nullptr;

    static SettingValue parseValue(const std::string &value);
};
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <fstream>
#include <sstream>
#include <charconv>

#include "window.h"
#include "gui.h"

GUI::GUI(GLFWwindow *rawWindow, const std::string &filepath) : rawWindow_(rawWindow)
{
    if (!loadSettings(filepath))
        throw std::runtime_error("Settings at file \"" + filepath + "\" cannot be loaded.");
}

void GUI::render(const std::string &fpsString)
{
    int width, height;
    glfwGetWindowSize(rawWindow_, &width, &height);

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height) / 30.0f), ImGuiCond_Always);
    ImGui::Begin("Stats Panel", nullptr,
                 ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text(fpsString.c_str());
    ImGui::End();
}

bool GUI::loadSettings(const std::string &filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
        return false;

    std::string line;
    while (getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ss(line);
        std::string key, value;
        if (!std::getline(ss, key, '=') || !std::getline(ss, value))
            continue;

        settingsMap[key] = parseValue(value);
    }

    return true;
}

bool GUI::saveSettings(const std::string &filepath)
{
    (void)filepath;
    return false;
}

SettingValue GUI::parseValue(const std::string &value)
{
    if (value == "true" || value == "false")
        return value == "true";

    // type checking without having to use try/catch
    int i;
    auto [ptr_i, ec_i] = std::from_chars(value.data(), value.data() + value.size(), i);
    if (ec_i == std::errc{} && ptr_i == value.data() + value.size())
        return i;

    float f;
    auto [ptr_f, ec_f] = std::from_chars(value.data(), value.data() + value.size(), f);
    if (ec_i == std::errc{} && ptr_f == value.data() + value.size())
        return f;

    return value;
}
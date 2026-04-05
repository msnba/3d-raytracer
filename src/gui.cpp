#include <GLFW/glfw3.h>
#include <imgui.h>
#include <fstream>
#include <sstream>

#include "window.h"
#include "gui.h"

GUI::GUI(GLFWwindow *rawWindow, const std::string &file) : rawWindow_(rawWindow)
{
    // not loading from a settings file yet
    // loadSettings(file);
    (void)file;
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
    (void)filepath;
    return false;
}

bool GUI::saveSettings(const std::string &filepath)
{
    (void)filepath;
    return false;
}

SettingValue GUI::parseValue(const std::string &value)
{
    (void)value;
    return nullptr;
}
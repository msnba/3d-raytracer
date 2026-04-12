#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <fstream>
#include <sstream>
#include <charconv>

#include "window.h"
#include "gui.h"

GUI::GUI(GLFWwindow *rawWindow, const std::string &filepath) : rawWindow_(rawWindow)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr; // Disables ini saving beside the executable
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    int w, h;
    glfwGetFramebufferSize(rawWindow_, &w, &h);
    guiScale_ = static_cast<float>(h) / 1080.0f;

    ImGui::GetStyle().ScaleAllSizes(guiScale_);
    // ImGui::GetIO().FontGlobalScale = guiScale_;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(rawWindow, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    if (!loadSettings(filepath))
        throw std::runtime_error("Settings at file \"" + filepath + "\" cannot be loaded.");
}

GUI::~GUI()
{
    destroySelf();
}

GUI::GUI(GUI &&other) noexcept : settingsMap_(std::move(other.settingsMap_)), rawWindow_(other.rawWindow_), guiScale_(other.guiScale_)
{
    other.rawWindow_ = nullptr;
}

GUI &GUI::operator=(GUI &&other) noexcept
{
    if (this != &other)
    {
        destroySelf();

        settingsMap_ = std::move(other.settingsMap_);
        rawWindow_ = other.rawWindow_;
        guiScale_ = other.guiScale_;
        other.rawWindow_ = nullptr;
    }

    return *this;
}

void GUI::render(const ViewportData &data)
{
    static bool showDebug = true;
    static bool showProperties = true;
    static bool isAccumulationEnabled = true;
    static bool isSSAAEnabled = true;
    static int maxBounce = 5;
    static int raysPerPixel = 1;

    // Menu Bar
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, {});
    ImGui::PushStyleColor(ImGuiCol_WindowBg, {0, 0, 0, .25f});
    ImGui::PushStyleColor(ImGuiCol_BorderShadow, {});
    ImGui::PushStyleColor(ImGuiCol_Border, {});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
    if (ImGui::BeginMainMenuBar() && showTopBar_)
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Quit", "Ctrl+Q"))
                glfwSetWindowShouldClose(rawWindow_, true);

            if (ImGui::MenuItem("Screenshot", "F12") && callbacks_.onScreenshot)
                callbacks_.onScreenshot();

            if (ImGui::MenuItem("Toggle Fullscreen", "F11") && callbacks_.onToggleFullscreen)
                callbacks_.onToggleFullscreen();

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Show Debug Panel", nullptr, &showDebug);
            ImGui::MenuItem("Show Properties Panel", nullptr, &showProperties);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    if (showDebug)
    {
        float menuBarHeight = ImGui::GetFrameHeight(); // avoids overlapping top bar
        ImGui::SetNextWindowPos({0, menuBarHeight}, ImGuiCond_Appearing);
        ImGui::SetNextWindowSize({180.0f * guiScale_, 100.0f * guiScale_}, ImGuiCond_Appearing);

        ImGui::Begin("Debug Panel", nullptr);

        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, {0, 0, 0, 0});
        ImGui::BeginChild("##scroll", {0, 0}, false,
                          ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        std::stringstream stream;
        stream << data.fpsString << "\nSamples: " << data.accumFrameIndex << "\nFOV: " << data.fov;
        ImGui::Text("%s", stream.str().c_str());

        ImGui::EndChild();

        ImGui::PopStyleColor(1);

        ImGui::End();
    }

    if (showProperties)
    {
        float menuBarHeight = ImGui::GetFrameHeight();
        ImGui::SetNextWindowPos({0, menuBarHeight + 100.0f * guiScale_}, ImGuiCond_Appearing);

        ImGui::SetNextWindowSize({200.0f * guiScale_, 200.0f * guiScale_}, ImGuiCond_Appearing);

        ImGui::Begin("Properties Panel", nullptr);

        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, {0, 0, 0, 0});
        ImGui::BeginChild("##scroll", {0, 0}, false,
                          ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SeparatorText("Global Settings");

        if (ImGui::Checkbox("Accumulation Enabled", &isAccumulationEnabled) && callbacks_.onAccumulationChanged)
            callbacks_.onAccumulationChanged(isAccumulationEnabled);

        if (ImGui::Checkbox("SSAA Enabled", &isSSAAEnabled) && callbacks_.onSSAAChanged)
            callbacks_.onSSAAChanged(isSSAAEnabled);

        if (callbacks_.onMaxBounceChanged)
        {
            ImGui::Text("Max Ray Bounces");
            if (ImGui::InputInt("", &maxBounce))
            {
                maxBounce = std::max(maxBounce, 0);
                callbacks_.onMaxBounceChanged(static_cast<uint32_t>(maxBounce));
            }
        }

        if (callbacks_.onRaysPerPixelChanged)
        {
            ImGui::Text("Rays Per Pixel");
            if (ImGui::InputInt("", &raysPerPixel))
            {
                raysPerPixel = std::max(raysPerPixel, 0);
                callbacks_.onRaysPerPixelChanged(static_cast<uint32_t>(raysPerPixel));
            }
        }

        ImGui::EndChild();

        ImGui::PopStyleColor(1);

        ImGui::End();
    }
}

void GUI::setCallbacks(const ViewportCallbacks &callbacks)
{
    callbacks_ = callbacks;
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

        settingsMap_[key] = parseValue(value);
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
    if (ec_f == std::errc{} && ptr_f == value.data() + value.size())
        return f;

    return value;
}

void GUI::destroySelf()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
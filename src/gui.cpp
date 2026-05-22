#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <fstream>
#include <sstream>
#include <charconv>

#include "window.h"
#include "gui.h"
#include "log.h"
#include "settings.h"
#include "file_picker.h"

GUI::GUI(GLFWwindow *rawWindow) : rawWindow_(rawWindow)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();

    ImGui_ImplGlfw_InitForOpenGL(rawWindow, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    io.IniFilename = nullptr; // Disables ini saving beside the executable
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    int w, h;
    glfwGetFramebufferSize(rawWindow_, &w, &h);
    guiScale_ = static_cast<float>(h) / 1080.0f * Settings::get().getValue("guiScale", 1.0f);

    ImFontConfig cfg;
    cfg.SizePixels = 13.0f * guiScale_; // default imgui font size
    io.Fonts->AddFontDefault(&cfg);
    currentFont_ = nullptr;

    ImGui::GetStyle().ScaleAllSizes(guiScale_);
    io.FontGlobalScale = 1.0f;

    ImGui::StyleColorsDark();
}

GUI::~GUI()
{
    destroySelf();
}

GUI::GUI(GUI &&other) : rawWindow_(other.rawWindow_), guiScale_(other.guiScale_)
{
    other.rawWindow_ = nullptr;
}

GUI &GUI::operator=(GUI &&other)
{
    if (this != &other)
    {
        destroySelf();

        rawWindow_ = other.rawWindow_;
        guiScale_ = other.guiScale_;
        other.rawWindow_ = nullptr;
    }

    return *this;
}

void GUI::render(const ViewportData &data)
{
    if (!renderGui_)
        return;

    Settings &s = Settings::get();

    bool showDebug = s.getValue("showDebug", true);
    bool showSettings = s.getValue("showSettings", true);
    bool showLog = s.getValue("showLog", true);
    bool isAccumulationEnabled = s.getValue("accumulationDefaultEnabled", true);
    bool isSSAAEnabled = s.getValue("SSAADefaultEnabled", false);
    bool isVSyncEnabled = s.getValue("VSyncDefaultEnabled", true);
    static int maxBounce = 5;
    static int raysPerPixel = 1;

    // Menu Bar
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, {});
    ImGui::PushStyleColor(ImGuiCol_WindowBg, {0, 0, 0, .25f});
    ImGui::PushStyleColor(ImGuiCol_BorderShadow, {});
    ImGui::PushStyleColor(ImGuiCol_Border, {});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
    ImGui::PushFont(currentFont_);

    if (ImGui::BeginMainMenuBar() && showTopBar_)
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Quit", "Ctrl+Q"))
                glfwSetWindowShouldClose(rawWindow_, true);

            if (auto *fn = callbacks_.get<void()>("screenshot"); fn && ImGui::MenuItem("Screenshot", "F12"))
                (*fn)();

            ImGui::Separator();

            if (ImGui::MenuItem("Load Scene"))
            {
                FilePicker::get().query({"json", "", "Load Scene"}, false, [this](const std::string &destination)
                                        {
                    if(auto* fn = callbacks_.get<void(std::string)>("sceneLoaded"))
                        (*fn)(destination); });
            }

            if (auto *fn = callbacks_.get<void(std::optional<std::string>)>("sceneSaved"); fn && ImGui::MenuItem("Save Scene"))
                (*fn)(std::nullopt);

            if (ImGui::MenuItem("Save Scene As"))
            {
                FilePicker::get().query({"json", "scene", "Save Scene"}, true, [this](const std::string &destination)
                                        {
                    if(auto* fn = callbacks_.get<void(std::optional<std::string>)>("sceneSaved"))
                        (*fn)(destination); });
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Load Settings"))
            {
                FilePicker::get().query({"cfg", "", "Load Settings"}, false, [this](const std::string &destination)
                                        {
                    Settings::get().loadFromFile(destination, true); 
                    if(auto* fn = callbacks_.get<void()>("settingsLoaded"))
                        (*fn)(); });
            }

            if (ImGui::MenuItem("Save Settings"))
                Settings::get().saveToFile();

            if (ImGui::MenuItem("Save Settings As"))
            {
                FilePicker::get().query({"cfg", "", "Save Settings"}, true, [this](const std::string &destination)
                                        { Settings::get().saveToFile(destination); });
            }

            // if (ImGui::MenuItem("Toggle Fullscreen", "F11") && callbacks_.onToggleFullscreen)
            //     callbacks_.onToggleFullscreen();

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Show Debug Panel", nullptr, &showDebug))
                s.setValue("showDebug", showDebug);
            if (ImGui::MenuItem("Show Settings Panel", nullptr, &showSettings))
                s.setValue("showSettings", showSettings);
            if (ImGui::MenuItem("Show Log Panel", nullptr, &showLog))
                s.setValue("showLog", showLog);
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

        ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);

        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, {0, 0, 0, 0});
        ImGui::BeginChild("##scrollDebug", {0, 0}, false,
                          ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        std::stringstream stream;
        stream << data.fpsString << "\nSamples: " << data.accumFrameIndex << "\nFOV: " << data.fov;
        ImGui::Text("%s", stream.str().c_str());

        ImGui::EndChild();

        ImGui::PopStyleColor(1);

        ImGui::End();
    }

    if (showSettings)
    {
        float menuBarHeight = ImGui::GetFrameHeight();
        ImGui::SetNextWindowPos({0, menuBarHeight + 100.0f * guiScale_}, ImGuiCond_Appearing);

        ImGui::SetNextWindowSize({200.0f * guiScale_, 230.0f * guiScale_}, ImGuiCond_Appearing);

        ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);

        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, {0, 0, 0, 0});
        ImGui::BeginChild("##scrollSettings", {0, 0}, false,
                          ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SeparatorText("Render Settings");

        if (auto *fn = callbacks_.get<void(bool)>("accumulationChanged"); fn && ImGui::Checkbox("Frame Accumulation Enabled", &isAccumulationEnabled))
        {
            s.setValue("accumulationDefaultEnabled", isAccumulationEnabled);
            (*fn)(isAccumulationEnabled);
        }

        if (auto *fn = callbacks_.get<void(bool)>("SSAAChanged"); fn && ImGui::Checkbox("SSAA Enabled", &isSSAAEnabled))
        {
            s.setValue("SSAADefaultEnabled", isSSAAEnabled);
            (*fn)(isSSAAEnabled);
        }

        if (ImGui::Checkbox("VSync Enabled", &isVSyncEnabled))
        {
            s.setValue("VSyncDefaultEnabled", isVSyncEnabled);
            glfwSwapInterval(isVSyncEnabled ? 1 : 0);
        }

        if (auto *fn = callbacks_.get<void(uint32_t)>("maxBounceChanged"))
        {
            ImGui::Text("Max Ray Bounces");
            if (ImGui::InputInt("##maxBounce", &maxBounce))
            {
                maxBounce = std::max(maxBounce, 0);
                (*fn)(static_cast<uint32_t>(maxBounce));
            }
        }

        if (auto *fn = callbacks_.get<void(uint32_t)>("raysPerPixelChanged"))
        {
            ImGui::Text("Rays Per Pixel");
            if (ImGui::InputInt("##raysPerPixel", &raysPerPixel))
            {
                raysPerPixel = std::max(raysPerPixel, 0);
                (*fn)(static_cast<uint32_t>(raysPerPixel));
            }
        }

        ImGui::EndChild();

        ImGui::PopStyleColor(1);

        ImGui::End();
    }

    if (showLog)
    {
        float menuBarHeight = ImGui::GetFrameHeight();
        ImGui::SetNextWindowPos({0, menuBarHeight + 340.0f * guiScale_}, ImGuiCond_Appearing);

        ImGui::SetNextWindowSize({420.0f * guiScale_, 230.0f * guiScale_}, ImGuiCond_Appearing);

        ImGui::Begin("Log", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);

        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, {0, 0, 0, 0});
        ImGui::BeginChild("##scrollLog", {0, 0}, false,
                          ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        Log::Logger::get().forEach([&](const Log::Entry &e)
                                   {
            ImVec4 col;
            const char* prefix = "";
            switch (e.level) {
                case Log::Level::Debug:   col = {0.5f, 0.5f, 0.5f, 1.f}; prefix = "[D]"; break;
                case Log::Level::Info:    col = {0.8f, 0.8f, 0.8f, 1.f}; prefix = "[I]"; break;
                case Log::Level::Warning: col = {1.0f, 0.8f, 0.2f, 1.f}; prefix = "[W]"; break;
                case Log::Level::Error:   col = {1.0f, 0.3f, 0.3f, 1.f}; prefix = "[E]"; break;
            } 

            ImGui::TextColored(col, "%s %5.1fs  %s", prefix, e.time, e.msg.c_str()); });

        ImGui::EndChild();

        ImGui::PopStyleColor(1);

        ImGui::End();
    }

    ImGui::PopFont();
}

bool GUI::toggleRender()
{
    renderGui_ = !renderGui_;
    return renderGui_;
}

void GUI::setCallbacks(const ViewportCallbacks &callbacks)
{
    callbacks_ = callbacks;
}

void GUI::destroySelf()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    Settings::get().saveToFile();
}
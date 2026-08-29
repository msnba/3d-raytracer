#include <GLFW/glfw3.h>
#include <charconv>
#include <fstream>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <sstream>

#include "tinytracer/core/file_picker.h"
#include "tinytracer/core/gui.h"
#include "tinytracer/core/window.h"
#include "tinytracer/utils/log.h"
#include "tinytracer/utils/settings.h"

namespace tinytracer::core {

GUI::GUI() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();

  io.IniFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();
}

GUI::GUI(GLFWwindow *rawWindow) : GUI() { attachWindow(rawWindow); }

GUI::~GUI() { destroySelf(); }

GUI::GUI(GUI &&other)
    : rawWindow_(other.rawWindow_), guiScale_(other.guiScale_) {
  other.rawWindow_ = nullptr;
}

GUI &GUI::operator=(GUI &&other) {
  if (this != &other) {
    destroySelf();

    rawWindow_ = other.rawWindow_;
    guiScale_ = other.guiScale_;
    other.rawWindow_ = nullptr;
  }

  return *this;
}

void GUI::renderMenuBar(float menuBarHeight) {
  (void)menuBarHeight;
  auto &userSettings = tinytracer::utils::Settings::get();

  ImGui::PushStyleColor(ImGuiCol_MenuBarBg, {});
  ImGui::PushStyleColor(ImGuiCol_WindowBg, {0, 0, 0, .25f});
  ImGui::PushStyleColor(ImGuiCol_BorderShadow, {});
  ImGui::PushStyleColor(ImGuiCol_Border, {});
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
  ImGui::PushFont(currentFont_);

  if (ImGui::BeginMainMenuBar() && showTopBar_) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Quit", "Ctrl+Q"))
        glfwSetWindowShouldClose(rawWindow_, true);

      if (auto *fn = callbacks_.get<void()>("screenshot");
          fn && ImGui::MenuItem("Screenshot", "F12"))
        (*fn)();

      ImGui::Separator();

      if (ImGui::MenuItem("Load Scene")) {
        FilePicker::get().query(
            {"json", "", "Load Scene"}, false,
            [this](const std::string &destination) {
              if (auto *fn = callbacks_.get<void(std::string)>("sceneLoaded"))
                (*fn)(destination);
            });
      }

      if (auto *fn =
              callbacks_.get<void(std::optional<std::string>)>("sceneSaved");
          fn && ImGui::MenuItem("Save Scene"))
        (*fn)(std::nullopt);

      if (ImGui::MenuItem("Save Scene As")) {
        FilePicker::get().query(
            {"json", "scene", "Save Scene"}, true,
            [this](const std::string &destination) {
              if (auto *fn = callbacks_.get<void(std::optional<std::string>)>(
                      "sceneSaved"))
                (*fn)(destination);
            });
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Load Settings")) {
        FilePicker::get().query(
            {"cfg", "", "Load Settings"}, false,
            [this](const std::string &destination) {
              tinytracer::utils::Settings::get().loadFromFile(destination,
                                                              true);
              if (auto *fn = callbacks_.get<void()>("settingsLoaded"))
                (*fn)();
            });
      }

      if (ImGui::MenuItem("Save Settings"))
        tinytracer::utils::Settings::get().saveToFile();

      if (ImGui::MenuItem("Save Settings As")) {
        FilePicker::get().query({"cfg", "", "Save Settings"}, true,
                                [this](const std::string &destination) {
                                  tinytracer::utils::Settings::get().saveToFile(
                                      destination);
                                });
      }

      // if (ImGui::MenuItem("Toggle Fullscreen", "F11") &&
      // callbacks_.onToggleFullscreen)
      //     callbacks_.onToggleFullscreen();

      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      auto showDebug = userSettings.getValue("showDebug", true);
      auto showSettings = userSettings.getValue("showSettings", true);
      auto showLog = userSettings.getValue("showLog", true);
      if (ImGui::MenuItem("Show Debug Panel", nullptr, &showDebug))
        userSettings.setValue("showDebug", showDebug);
      if (ImGui::MenuItem("Show Settings Panel", nullptr, &showSettings))
        userSettings.setValue("showSettings", showSettings);
      if (ImGui::MenuItem("Show Log Panel", nullptr, &showLog))
        userSettings.setValue("showLog", showLog);
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }

  ImGui::PopStyleVar();
  ImGui::PopStyleColor(4);
}

void GUI::render(const ViewportData &data) {
  if (!renderGui_ || !rawWindow_)
    return;

  auto &userSettings = tinytracer::utils::Settings::get();
  auto sceneSettings = data.scene->settings();
  bool sceneSettingsChanged = false;

  bool showDebug = userSettings.getValue("showDebug", false);
  bool showSettings = userSettings.getValue("showSettings", false);
  bool showLog = userSettings.getValue("showLog", false);
  bool showOutliner = userSettings.getValue("showOutliner", true);
  bool showProperties = userSettings.getValue("showProperties", true);
  bool isAccumulationEnabled =
      userSettings.getValue("accumulationDefaultEnabled", true);
  bool isSSAAEnabled = userSettings.getValue("SSAADefaultEnabled", false);
  bool isVSyncEnabled = userSettings.getValue("VSyncDefaultEnabled", true);

  float menuBarHeight = ImGui::GetFrameHeight();

  // Menu Bar
  ImGui::PushStyleColor(ImGuiCol_MenuBarBg, {});
  ImGui::PushStyleColor(ImGuiCol_WindowBg, {0, 0, 0, .25f});
  ImGui::PushStyleColor(ImGuiCol_BorderShadow, {});
  ImGui::PushStyleColor(ImGuiCol_Border, {});
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
  ImGui::PushFont(currentFont_);

  if (ImGui::BeginMainMenuBar() && showTopBar_) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Quit", "Ctrl+Q"))
        glfwSetWindowShouldClose(rawWindow_, true);

      if (auto *fn = callbacks_.get<void()>("screenshot");
          fn && ImGui::MenuItem("Screenshot", "F12"))
        (*fn)();

      ImGui::Separator();

      if (ImGui::MenuItem("Load Scene")) {
        FilePicker::get().query(
            {"json", "", "Load Scene"}, false,
            [this](const std::string &destination) {
              if (auto *fn = callbacks_.get<void(std::string)>("sceneLoaded"))
                (*fn)(destination);
            });
      }

      if (auto *fn =
              callbacks_.get<void(std::optional<std::string>)>("sceneSaved");
          fn && ImGui::MenuItem("Save Scene"))
        (*fn)(std::nullopt);

      if (ImGui::MenuItem("Save Scene As")) {
        FilePicker::get().query(
            {"json", "scene", "Save Scene"}, true,
            [this](const std::string &destination) {
              if (auto *fn = callbacks_.get<void(std::optional<std::string>)>(
                      "sceneSaved"))
                (*fn)(destination);
            });
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Load Settings")) {
        FilePicker::get().query(
            {"cfg", "", "Load Settings"}, false,
            [this](const std::string &destination) {
              tinytracer::utils::Settings::get().loadFromFile(destination,
                                                              true);
              if (auto *fn = callbacks_.get<void()>("settingsLoaded"))
                (*fn)();
            });
      }

      if (ImGui::MenuItem("Save Settings"))
        tinytracer::utils::Settings::get().saveToFile();

      if (ImGui::MenuItem("Save Settings As")) {
        FilePicker::get().query({"cfg", "", "Save Settings"}, true,
                                [this](const std::string &destination) {
                                  tinytracer::utils::Settings::get().saveToFile(
                                      destination);
                                });
      }

      // if (ImGui::MenuItem("Toggle Fullscreen", "F11") &&
      // callbacks_.onToggleFullscreen)
      //     callbacks_.onToggleFullscreen();

      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      if (ImGui::MenuItem("Show Debug Panel", nullptr, &showDebug))
        userSettings.setValue("showDebug", showDebug);
      if (ImGui::MenuItem("Show Settings Panel", nullptr, &showSettings))
        userSettings.setValue("showSettings", showSettings);
      if (ImGui::MenuItem("Show Log Panel", nullptr, &showLog))
        userSettings.setValue("showLog", showLog);
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }

  ImGui::PopStyleVar();
  ImGui::PopStyleColor(4);

  // Debug
  if (showDebug) {
    ImGui::SetNextWindowPos({0, menuBarHeight}, ImGuiCond_Appearing);
    ImGui::SetNextWindowSize({180.0f * guiScale_, 100.0f * guiScale_},
                             ImGuiCond_Appearing);

    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);

    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, {0, 0, 0, 0});
    ImGui::BeginChild("##scrollDebug", {0, 0}, false,
                      ImGuiWindowFlags_HorizontalScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);

    std::stringstream stream;
    stream << "FPS: " << static_cast<int>(data.smoothFps)
           << "\nSamples: " << data.accumFrameIndex << "\nFOV: " << data.fov;
    ImGui::Text("%s", stream.str().c_str());

    ImGui::EndChild();

    ImGui::PopStyleColor(1);

    ImGui::End();
  }

  // Settings
  if (showSettings) {
    ImGui::SetNextWindowPos({0, menuBarHeight + 100.0f * guiScale_},
                            ImGuiCond_Appearing);

    ImGui::SetNextWindowSize({200.0f * guiScale_, 230.0f * guiScale_},
                             ImGuiCond_Appearing);

    ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);

    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, {0, 0, 0, 0});
    ImGui::BeginChild("##scrollSettings", {0, 0}, false,
                      ImGuiWindowFlags_HorizontalScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::SeparatorText("Render Settings");

    if (auto *fn = callbacks_.get<void(bool)>("accumulationChanged");
        fn &&
        ImGui::Checkbox("Frame Accumulation Enabled", &isAccumulationEnabled)) {
      userSettings.setValue("accumulationDefaultEnabled",
                            isAccumulationEnabled);
      (*fn)(isAccumulationEnabled);
    }

    if (auto *fn = callbacks_.get<void(bool)>("SSAAChanged");
        fn && ImGui::Checkbox("SSAA Enabled", &isSSAAEnabled)) {
      userSettings.setValue("SSAADefaultEnabled", isSSAAEnabled);
      (*fn)(isSSAAEnabled);
    }

    if (ImGui::Checkbox("VSync Enabled", &isVSyncEnabled)) {
      userSettings.setValue("VSyncDefaultEnabled", isVSyncEnabled);
      glfwSwapInterval(isVSyncEnabled ? 1 : 0);
    }

    ImGui::Text("Max Ray Bounces");
    if (ImGui::InputInt("##maxBounce",
                        reinterpret_cast<int *>(&sceneSettings.maxBounce))) {
      sceneSettings.maxBounce = std::max(sceneSettings.maxBounce, 0u);
      sceneSettingsChanged = true;
    }

    ImGui::Text("Rays Per Pixel");
    if (ImGui::InputInt(
            "##raysPerPixel",
            reinterpret_cast<int *>(&sceneSettings.numRaysPerPixel))) {
      sceneSettings.numRaysPerPixel =
          std::max(sceneSettings.numRaysPerPixel, 0u);
      sceneSettingsChanged = true;
    }

    if (auto *fn = callbacks_.get<void(tinytracer::world::Scene::Settings)>(
            "sceneDataChanged");
        fn && sceneSettingsChanged)
      (*fn)(sceneSettings);

    ImGui::EndChild();

    ImGui::PopStyleColor(1);

    ImGui::End();
  }

  // Log
  if (showLog) {
    ImGui::SetNextWindowPos({0, menuBarHeight + 340.0f * guiScale_},
                            ImGuiCond_Appearing);

    ImGui::SetNextWindowSize({420.0f * guiScale_, 230.0f * guiScale_},
                             ImGuiCond_Appearing);

    ImGui::Begin("Log", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);

    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, {0, 0, 0, 0});
    ImGui::BeginChild("##scrollLog", {0, 0}, false,
                      ImGuiWindowFlags_HorizontalScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);

    tinytracer::utils::Logger::get().forEach(
        [&](const tinytracer::utils::Entry &e) {
          ImVec4 col;
          const char *prefix = "";
          switch (e.level) {
          case tinytracer::utils::Level::Debug:
            col = {0.5f, 0.5f, 0.5f, 1.f};
            prefix = "[D]";
            break;
          case tinytracer::utils::Level::Info:
            col = {0.8f, 0.8f, 0.8f, 1.f};
            prefix = "[I]";
            break;
          case tinytracer::utils::Level::Warning:
            col = {1.0f, 0.8f, 0.2f, 1.f};
            prefix = "[W]";
            break;
          case tinytracer::utils::Level::Error:
            col = {1.0f, 0.3f, 0.3f, 1.f};
            prefix = "[E]";
            break;
          }

          ImGui::TextColored(col, "%s %5.1fs  %s", prefix, e.time,
                             e.msg.c_str());
        });

    ImGui::EndChild();

    ImGui::PopStyleColor(1);

    ImGui::End();
  }

  // Outliner
  if (showOutliner) {
    ImGui::SetNextWindowPos({780.0f * guiScale_, menuBarHeight},
                            ImGuiCond_Appearing);

    ImGui::SetNextWindowSize({300.0f * guiScale_, 600.0f * guiScale_},
                             ImGuiCond_Appearing);

    ImGui::Begin("Outline", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);

    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, {0, 0, 0, 0});
    ImGui::BeginChild("##scrollOutliner", {0, 0}, false,
                      ImGuiWindowFlags_HorizontalScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);

    if (data.scene) {
      const auto &objects = data.scene->objects();
      for (int i = 0; i < static_cast<int>(objects.size()); i++) {
        const auto &obj = objects[static_cast<long unsigned int>(i)];

        bool isSelected = (selectedObjectIndex_ == i);
        if (ImGui::Selectable((" " + obj->name_).c_str(), isSelected)) {
          selectedObjectIndex_ = isSelected ? -1 : i; // toggle deselect

          if (auto *fn = callbacks_.get<void(int)>("objectSelected"))
            (*fn)(selectedObjectIndex_);
        }

        if (isSelected) {
          ImGui::SameLine();
          ImGui::TextDisabled(" ");
        }
      }
    } else {
      ImGui::TextDisabled("No scene loaded.");
    }

    ImGui::EndChild();

    ImGui::PopStyleColor(1);

    ImGui::End();
  }

  // Properties Panel
  if (showProperties) {
    ImGui::SetNextWindowPos({480.0f * guiScale_, menuBarHeight},
                            ImGuiCond_Appearing);

    ImGui::SetNextWindowSize({300.0f * guiScale_, 600.0f * guiScale_},
                             ImGuiCond_Appearing);

    ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);

    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, {0, 0, 0, 0});
    ImGui::BeginChild("##scrollProperties", {0, 0}, false,
                      ImGuiWindowFlags_HorizontalScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);

    if (data.scene) {
      if (selectedObjectIndex_ >= 0 &&
          static_cast<size_t>(selectedObjectIndex_) <
              data.scene->objectCount()) {
        const auto &object =
            data.scene->objects()[static_cast<size_t>(selectedObjectIndex_)];

        ImGui::SeparatorText(object->name_.c_str());

        ImGui::Text("Transform");
        auto t = object->transform_;
        bool sliderHeld = false;
        static bool prevSliderHeld = sliderHeld;
        bool valueChanged = false;

        // x
        if (ImGui::InputFloat("Object X", &t.position.x, 0.0f, 0.0f, "%.3f",
                              ImGuiInputTextFlags_EnterReturnsTrue))
          valueChanged = true;

        if (ImGui::SliderFloat("##sliderObjectX", &t.position.x, 0.0f, 50.0f,
                               "%.3f"))
          valueChanged = true;

        if (ImGui::IsItemActive())
          sliderHeld = true;

        // y
        if (ImGui::InputFloat("Object Y", &t.position.y, 0.0f, 0.0f, "%.3f",
                              ImGuiInputTextFlags_EnterReturnsTrue))
          valueChanged = true;

        if (ImGui::SliderFloat("##sliderObjectY", &t.position.y, 0.0f, 50.0f,
                               "%.3f"))
          valueChanged = true;

        if (ImGui::IsItemActive())
          sliderHeld = true;

        // z
        if (ImGui::InputFloat("Object Z", &t.position.z, 0.0f, 0.0f, "%.3f",
                              ImGuiInputTextFlags_EnterReturnsTrue))
          valueChanged = true;

        if (ImGui::SliderFloat("##sliderObjectZ", &t.position.z, 0.0f, 50.0f,
                               "%.3f"))
          valueChanged = true;

        if (ImGui::IsItemActive())
          sliderHeld = true;

        if (valueChanged)
          if (auto *fn =
                  callbacks_
                      .get<void(int, tinytracer::world::Object::Transform)>(
                          "objectTransformChanged"))
            (*fn)(selectedObjectIndex_, t);

        if (auto *fn = callbacks_.get<void(bool)>("accumulationChanged");
            fn && prevSliderHeld != sliderHeld)
          (*fn)(!sliderHeld);

        prevSliderHeld = sliderHeld;
      }
    } else {
      ImGui::TextDisabled("No scene loaded.");
    }

    ImGui::EndChild();

    ImGui::PopStyleColor(1);

    ImGui::End();
  }

  ImGui::PopFont();
}

bool GUI::toggleRender() {
  renderGui_ = !renderGui_;
  return renderGui_;
}

void GUI::setCallbacks(const ViewportCallbacks &callbacks) {
  callbacks_ = callbacks;
}

void GUI::attachWindow(GLFWwindow *rawWindow) {
  if (!rawWindow)
    return;

  if (rawWindow_) {
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
  }

  rawWindow_ = rawWindow;

  ImGui_ImplGlfw_InitForOpenGL(rawWindow_, true);
  ImGui_ImplOpenGL3_Init("#version 430");

  // monitor content scale, accounts for dpi
  float xscale = 1.0f, yscale = 1.0f;
  GLFWmonitor *monitor = glfwGetWindowMonitor(rawWindow_);
  if (!monitor)
    monitor = glfwGetPrimaryMonitor();
  if (monitor)
    glfwGetMonitorContentScale(monitor, &xscale, &yscale);
  guiScale_ =
      yscale * tinytracer::utils::Settings::get().getValue("guiScale", 1.0f);

  rebuildFontAtlas(guiScale_);

  ImGui::GetStyle() = ImGuiStyle();
  ImGui::StyleColorsDark();
  ImGui::GetStyle().ScaleAllSizes(guiScale_);
}

void GUI::destroySelf() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  tinytracer::utils::Settings::get().saveToFile();
}

void GUI::rebuildFontAtlas(float scale) {
  ImGuiIO &io = ImGui::GetIO();
  io.Fonts->Clear();

  ImFontConfig cfg;
  cfg.SizePixels = std::round(13.0f * scale);
  currentFont_ = io.Fonts->AddFontDefault(&cfg);
  io.Fonts->Build();

  io.FontGlobalScale = 1.0f;

  ImGui_ImplOpenGL3_DestroyFontsTexture();
  ImGui_ImplOpenGL3_CreateFontsTexture();
}

} // namespace tinytracer::core
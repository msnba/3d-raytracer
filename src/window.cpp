#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdexcept>

#include "window.h"

#define FULLSCREEN

Window::Window(unsigned int width, unsigned int height, const char *title)
{
    if (!glfwInit())
        return;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // TODO: Handle resizing properly.

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

#ifdef FULLSCREEN
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    window = glfwCreateWindow(mode->width, mode->height, title, monitor, NULL);

    SCR_WIDTH = static_cast<unsigned int>(mode->width);
    SCR_HEIGHT = static_cast<unsigned int>(mode->height);

    (void)width;
    (void)height;
#else
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    window = glfwCreateWindow(width, height, title, NULL, NULL);
#endif

    if (!window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize OpenGL Window\n");
    }

    glfwMakeContextCurrent(window); // ! Window must be contextualized before GLAD initialization.
    glfwSwapInterval(0);            // Turns off V-Sync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD\n");
    }

    // -- ImGUI --
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr; // Disables ini saving beside the executable
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
};

Window::~Window()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

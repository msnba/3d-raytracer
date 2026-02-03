#ifndef WINDOW_H
#define WINDOW_H
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
class Window
{
public:
    const unsigned int SCR_WIDTH, SCR_HEIGHT;
    GLFWwindow *window;
    Window(unsigned int width, unsigned int height, const char *title);
    ~Window();
};
#endif
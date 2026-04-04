#ifndef WINDOW_H
#define WINDOW_H

#include <GLFW/glfw3.h>
class Window
{
public:
    unsigned int SCR_WIDTH, SCR_HEIGHT;
    GLFWwindow *window;
    Window(unsigned int width, unsigned int height, const char *title);
    ~Window();
};
#endif
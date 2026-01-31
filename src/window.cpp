#include "window.h"

Window::Window(unsigned int width, unsigned int height, const char *title) : SCR_WIDTH(width), SCR_HEIGHT(height)
{
    if (!glfwInit())
        return;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // TODO: Handle resizing properly.

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(width, height, title, NULL, NULL);

    if (window == nullptr)
    {
        std::cerr << "Failed to initialize OpenGL Window\n";
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window); // ! Window must be contextualized before GLAD initialization.

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return;
    }
};

Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

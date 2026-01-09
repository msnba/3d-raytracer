#include <glad/glad.h>
#include <GLFW/glfw3.h> // ! Must be included after GLAD (due to method overriding).
#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void get_input(GLFWwindow *window);

int main()
{
    // -- Boilerplate --
    if (!glfwInit())
        return -1;

    GLFWwindow *window = glfwCreateWindow(800, 600, "Window", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to initialize Window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);                                    // ! Window must be contextualized before GLAD initialization.
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // Resize callback.

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // -- Render Loop --

    while (!glfwWindowShouldClose(window))
    {
        get_input(window);

        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}
void get_input(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
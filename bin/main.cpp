#include <glad/glad.h>
#include <GLFW/glfw3.h> // ! Must be included after GLAD (due to method overriding).
#include <iostream>

int main()
{
    if (!glfwInit())
        return -1;

    GLFWwindow *window = glfwCreateWindow(800, 600, "Window", NULL, NULL);
    glfwMakeContextCurrent(window); // ! Window must be contextualized before GLAD initialization.

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    while (!glfwWindowShouldClose(window)) // Simple refresh loop
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
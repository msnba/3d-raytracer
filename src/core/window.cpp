#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <stdexcept>

#include "tinytracer/core/window.h"

namespace tinytracer::core {

Window::Window(unsigned int width, unsigned int height, const char *title,
               const bool fullscreen) {
  if (!glfwInit())
    return;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

  if (fullscreen) {
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    window = glfwCreateWindow(mode->width, mode->height, title, monitor, NULL);

    SCR_WIDTH = static_cast<unsigned int>(mode->width);
    SCR_HEIGHT = static_cast<unsigned int>(mode->height);

    (void)width;
    (void)height;
  } else {
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height),
                              title, NULL, NULL);
  }

  if (!window) {
    glfwTerminate();
    throw std::runtime_error("Failed to initialize OpenGL Window\n");
  }

  glfwMakeContextCurrent(
      window); // ! Window must be contextualized before GLAD initialization.
  glfwSwapInterval(1); // V-Sync Enabled

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    glfwTerminate();
    throw std::runtime_error("Failed to initialize GLAD\n");
  }

  if (fullscreen)
    toggleFullscreen();
};

Window::~Window() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

Window::Window(Window &&other)
    : SCR_WIDTH(other.SCR_WIDTH), SCR_HEIGHT(other.SCR_HEIGHT),
      window(other.window), isFullscreen_(other.isFullscreen_),
      windowedX_(other.windowedX_), windowedY_(other.windowedY_),
      windowedWidth_(other.windowedWidth_),
      windowedHeight_(other.windowedHeight_) {
  other.window = nullptr;
}

Window &Window::operator=(Window &&other) {
  if (this != &other) {
    glfwDestroyWindow(window);

    window = other.window;
    SCR_WIDTH = other.SCR_WIDTH;
    SCR_HEIGHT = other.SCR_HEIGHT;
    isFullscreen_ = other.isFullscreen_;
    windowedX_ = other.windowedX_;
    windowedY_ = other.windowedY_;
    windowedWidth_ = other.windowedWidth_;
    windowedHeight_ = other.windowedHeight_;

    other.window = nullptr;
  }
  return *this;
}

std::pair<int, int> Window::getMaximizedSize() {
  glfwInit();
  int x, y, w, h;
  glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &x, &y, &w, &h);
  return {w, h};
}

void Window::toggleFullscreen() {
  if (!isFullscreen_) {
    glfwGetWindowPos(window, &windowedX_, &windowedY_);
    glfwGetFramebufferSize(window, &windowedWidth_, &windowedHeight_);

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height,
                         mode->refreshRate);

    SCR_WIDTH = static_cast<unsigned int>(mode->width);
    SCR_HEIGHT = static_cast<unsigned int>(mode->height);
  } else {
    glfwSetWindowMonitor(window, nullptr, windowedX_, windowedY_,
                         windowedWidth_, windowedHeight_, 0);

    SCR_WIDTH = static_cast<unsigned int>(windowedWidth_);
    SCR_HEIGHT = static_cast<unsigned int>(windowedHeight_);
  }

  isFullscreen_ = !isFullscreen_;
}

} // namespace tinytracer::core

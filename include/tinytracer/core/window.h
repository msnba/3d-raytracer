#pragma once

struct GLFWwindow;

namespace tinytracer::core {

class Window {
public:
  unsigned int SCR_WIDTH, SCR_HEIGHT;
  GLFWwindow *window;
  Window(unsigned int width, unsigned int height, const char *title,
         const bool fullscreen);
  ~Window();

  void toggleFullscreen();

private:
  bool isFullscreen_ = false;
  int windowedX_ = 0, windowedY_ = 0;
  int windowedWidth_ = 0, windowedHeight_ = 0;
};

} // namespace tinytracer::core
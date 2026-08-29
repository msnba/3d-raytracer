#define _USE_MATH_DEFINES
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <math.h>
#include <vector>

#include "raw_sources.h"
#include "tinytracer/core/gui.h"
#include "tinytracer/core/viewport.h"
#include "tinytracer/utils/settings.h"
#include "tinytracer/world/object.h"
#include "tinytracer/world/scene.h"

// MSVC fix
#ifndef M_PIf
#define M_PIf static_cast<float>(M_PI)
#endif

int main() {
  if (!tinytracer::utils::Settings::get().loadFromSource(DEFAULT_SETTINGS,
                                                         false))
    throw std::runtime_error("Default settings cannot be loaded.");

  auto [w, h] = tinytracer::core::Window::getMaximizedSize();

  tinytracer::core::Viewport viewport(
      std::make_unique<tinytracer::core::Window>(w, h, "Window", false),
      std::make_unique<tinytracer::world::Camera>(90.0f, 6.0f, 0.0f, -40.0f,
                                                  glm::vec3(-2, 7, 0)),
      std::make_unique<tinytracer::core::GUI>(),
      std::make_unique<tinytracer::world::Scene>());

  while (!viewport.shouldClose())
    viewport.update();

  return 0;
}
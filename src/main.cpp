#define _USE_MATH_DEFINES
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "viewport.h"
#include "object.h"
#include "gui.h"
#include "scene.h"

#define SCR_WIDTH 1440
#define SCR_HEIGHT 1080

// MSVC fix
#ifndef M_PIf
#define M_PIf static_cast<float>(M_PI)
#endif

int main()
{
    std::unique_ptr<Window> window = std::make_unique<Window>(SCR_WIDTH, SCR_HEIGHT, "Window", false);
    std::unique_ptr<Camera> camera = std::make_unique<Camera>(90.0f, 6.0f, 0.0f, -40.0f, glm::vec3(-2, 7, 0));
    std::unique_ptr<GUI> gui = std::make_unique<GUI>(window->window, "./assets/defaults.cfg");
    std::shared_ptr scene = std::make_shared<Scene>();

    Object::Material dragonMaterial{
        {1.f, 1.f, 1.f},      // color
        0.f,                  // smoothness
        {0.f, 0.f, 0.f, 0.f}, // emission color + strength
        .9f,                  // transparency
        1.5f                  // ior
    };

    Object::Transform dragonTransform{
        {5.5f, 1.5f, 0.f}, // position
        {},                // rotation
        glm::vec3(4)       // scale
    };

    scene->add(std::make_unique<Mesh>(Mesh("assets/models/dragon.obj", dragonTransform, dragonMaterial)));
    scene->add(std::make_unique<Rectangle>(Rectangle({0, 0, -12.5f}, {0, 0, 0}, 40.f, .5f, 40.f, {{1, 1, 1}, 0.f, {0, 0, 0, 0}, 0, 0})));

    // scene->meshes.push_back(loadRect({{{5.f, 4.f, -1.f}, {0, 0, 0}, {2, .25f, 2}}, {{0, 0, 0}, 0, {1, 1, 1, 1}, 0, 0, 0, 0}}, *scene));

    { // creates a circle of spheres in a color wheel
        float sides = 6;
        float radius = 3.f;
        float origin[2] = {5.5, 0.0};
        for (float i = 0; i < sides; i++)
        {
            float angle = 2.0f * M_PIf * i / sides + M_PIf / 2.0f;

            float r = 0.5f + 0.5f * sinf(angle);
            float g = 0.5f + 0.5f * sinf(angle + 2.0f * M_PIf / 3.0f);
            float b = 0.5f + 0.5f * sinf(angle + 4.0f * M_PIf / 3.0f);

            // x (up), y, z (right)
            scene->add(
                std::make_unique<Sphere>(Sphere(
                    {radius * sin(angle) + origin[0], 1.5, radius * cos(angle) + origin[1]}, // position
                    1.0f,                                                                    // radius
                    {
                        {0, 0, 0},      // color
                        0.f,            // smoothness
                        {r, g, b, 1.f}, // emission
                        0,              // transparency
                        0               // ior
                    })));
        }
    }

    Viewport viewport(std::move(window), std::move(camera), std::move(gui), scene);

    // -- Render Loop --
    while (!viewport.shouldClose())
    {
        viewport.update();
    }

    return 0;
}
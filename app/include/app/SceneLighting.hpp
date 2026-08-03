#ifndef APP_SCENE_LIGHTING_HPP
#define APP_SCENE_LIGHTING_HPP

#include <glm/glm.hpp>

namespace app {
/**
 * @brief Directional light data (used to represent dim moonlight coming through a window).
 */
struct DirectionalLight {
    glm::vec3 direction{-0.3f, -1.0f, -0.2f};
    glm::vec3 ambient{0.03f, 0.03f, 0.05f};
    glm::vec3 diffuse{0.15f, 0.15f, 0.2f};
    glm::vec3 specular{0.1f, 0.1f, 0.1f};
    bool enabled{true};
};

/**
 * @brief Point light data (used to represent the lamp hanging above the billiard table).
 */
struct PointLight {
    glm::vec3 position{0.0f, 2.2f, 0.0f};
    glm::vec3 ambient{0.05f, 0.05f, 0.03f};
    glm::vec3 diffuse{1.0f, 0.85f, 0.6f};
    glm::vec3 specular{1.0f, 0.9f, 0.7f};
    float constant{1.0f};
    float linear{0.09f};
    float quadratic{0.032f};
    bool enabled{true};
};

/**
 * @brief Aggregates all the light sources used to illuminate the billiard room scene.
 * Adjustable at runtime through @ref app::GUIController and the action/event chain in @ref app::MainController.
 */
struct SceneLighting {
    DirectionalLight directional;
    PointLight lamp;
};
}// namespace app

#endif//APP_SCENE_LIGHTING_HPP

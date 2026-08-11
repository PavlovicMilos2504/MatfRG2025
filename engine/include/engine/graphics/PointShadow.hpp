/**
 * @file PointShadow.hpp
 * @brief Defines the PointShadow class that implements omnidirectional shadow mapping for a point light.
 */

#ifndef POINT_SHADOW_HPP
#define POINT_SHADOW_HPP

#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>

namespace engine::resources {
class Shader;
}

namespace engine::graphics {
/**
 * @class PointShadow
 * @brief Implements omnidirectional (point light) shadow mapping using a depth cubemap rendered in a
 * single pass with a geometry shader (https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows).
 *
 * Usage:
 * @code
 * auto matrices = point_shadow->light_space_matrices(lamp.position);
 * depth_shader->use();
 * for (int i = 0; i < 6; ++i) {
 *     depth_shader->set_mat4("shadowMatrices[" + std::to_string(i) + "]", matrices[i]);
 * }
 * depth_shader->set_vec3("lightPos", lamp.position);
 * depth_shader->set_float("far_plane", point_shadow->far_plane());
 * point_shadow->begin_capture();
 * // ... draw the shadow-casting geometry with depth_shader (only the "model" uniform is needed)
 * point_shadow->end_capture(window_width, window_height);
 * // ... later, when shading the scene:
 * point_shadow->bind(lighting_shader, "shadowMap", texture_unit);
 * lighting_shader->set_float("farPlane", point_shadow->far_plane());
 * @endcode
 */
class PointShadow final {
public:
    /**
    * @brief Creates the depth cubemap and its framebuffer. Resolution is independent of the window
    * size, so this only needs to be called once (unlike @ref graphics::Bloom::resize).
    */
    void initialize(int resolution = 1024, float near_plane = 0.05f, float far_plane = 15.0f);

    /**
    * @brief Computes the 6 light-space (projection * view) matrices used by the depth pass's
    * geometry shader, one per cubemap face.
    */
    std::array<glm::mat4, 6> light_space_matrices(const glm::vec3 &light_position) const;

    /**
    * @brief Binds the depth cubemap's framebuffer and clears it, ready for the depth pass.
    */
    void begin_capture() const;

    /**
    * @brief Restores the default framebuffer and viewport after the depth pass.
    */
    void end_capture(int screen_width, int screen_height) const;

    /**
    * @brief Binds the depth cubemap to `texture_unit` and sets the `uniform_name` sampler uniform.
    */
    void bind(const resources::Shader *shader, const std::string &uniform_name, int texture_unit) const;

    /**
    * @brief Far plane distance used to normalize cubemap depth values; needed by the shading shader
    * to undo that normalization.
    */
    float far_plane() const {
        return m_far_plane;
    }

    /**
    * @brief Destroys all the OpenGL objects owned by this instance.
    */
    void destroy();

    /**
    * @brief Depth bias used by the shading shader to reduce shadow acne artifacts.
    */
    float &bias() {
        return m_bias;
    }

    /**
    * @brief Whether shadow sampling should be applied when shading the scene.
    */
    bool &enabled() {
        return m_enabled;
    }

private:
    int m_resolution{1024};
    float m_near_plane{0.05f};
    float m_far_plane{15.0f};

    uint32_t m_depth_map_fbo{0};
    uint32_t m_depth_cubemap{0};

    float m_bias{0.15f};
    bool m_enabled{true};
};
}// namespace engine::graphics

#endif//POINT_SHADOW_HPP

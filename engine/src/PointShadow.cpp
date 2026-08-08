// clang-format off
#include <glad/glad.h>
// clang-format on
#include <engine/graphics/OpenGL.hpp>
#include <engine/graphics/PointShadow.hpp>
#include <engine/resources/Shader.hpp>
#include <engine/util/Errors.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace engine::graphics {

void PointShadow::initialize(int resolution, float near_plane, float far_plane) {
    m_resolution = resolution;
    m_near_plane = near_plane;
    m_far_plane = far_plane;

    CHECKED_GL_CALL(glGenFramebuffers, 1, &m_depth_map_fbo);

    CHECKED_GL_CALL(glGenTextures, 1, &m_depth_cubemap);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_CUBE_MAP, m_depth_cubemap);
    for (uint32_t i = 0; i < 6; ++i) {
        CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, m_resolution,
                        m_resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_depth_map_fbo);
    // Attach the whole cubemap so the geometry shader can pick the face via gl_Layer.
    CHECKED_GL_CALL(glFramebufferTexture, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depth_cubemap, 0);
    CHECKED_GL_CALL(glDrawBuffer, GL_NONE);
    CHECKED_GL_CALL(glReadBuffer, GL_NONE);
    RG_GUARANTEE(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                 "Depth cubemap framebuffer used by PointShadow is not complete.");
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
}

std::array<glm::mat4, 6> PointShadow::light_space_matrices(const glm::vec3 &light_position) const {
    // Square cubemap faces: aspect ratio 1.0, 90 degree FOV covers exactly one face.
    glm::mat4 shadow_proj = glm::perspective(glm::radians(90.0f), 1.0f, m_near_plane, m_far_plane);

    // View matrix per face, looking down each of the 6 principal axes.
    return {
            shadow_proj * glm::lookAt(light_position, light_position + glm::vec3(1.0f, 0.0f, 0.0f),
                                      glm::vec3(0.0f, -1.0f, 0.0f)),
            shadow_proj * glm::lookAt(light_position, light_position + glm::vec3(-1.0f, 0.0f, 0.0f),
                                      glm::vec3(0.0f, -1.0f, 0.0f)),
            shadow_proj * glm::lookAt(light_position, light_position + glm::vec3(0.0f, 1.0f, 0.0f),
                                      glm::vec3(0.0f, 0.0f, 1.0f)),
            shadow_proj * glm::lookAt(light_position, light_position + glm::vec3(0.0f, -1.0f, 0.0f),
                                      glm::vec3(0.0f, 0.0f, -1.0f)),
            shadow_proj * glm::lookAt(light_position, light_position + glm::vec3(0.0f, 0.0f, 1.0f),
                                      glm::vec3(0.0f, -1.0f, 0.0f)),
            shadow_proj * glm::lookAt(light_position, light_position + glm::vec3(0.0f, 0.0f, -1.0f),
                                      glm::vec3(0.0f, -1.0f, 0.0f)),
    };
}

void PointShadow::begin_capture() const {
    CHECKED_GL_CALL(glViewport, 0, 0, m_resolution, m_resolution);
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_depth_map_fbo);
    CHECKED_GL_CALL(glClear, GL_DEPTH_BUFFER_BIT);
}

void PointShadow::end_capture(int screen_width, int screen_height) const {
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
    CHECKED_GL_CALL(glViewport, 0, 0, screen_width, screen_height);
}

void PointShadow::bind(const resources::Shader *shader, const std::string &uniform_name, int texture_unit) const {
    CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0 + texture_unit);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_CUBE_MAP, m_depth_cubemap);
    shader->set_int(uniform_name, texture_unit);
}

void PointShadow::destroy() {
    if (m_depth_map_fbo == 0) {
        return;
    }
    CHECKED_GL_CALL(glDeleteFramebuffers, 1, &m_depth_map_fbo);
    CHECKED_GL_CALL(glDeleteTextures, 1, &m_depth_cubemap);
    m_depth_map_fbo = 0;
}
}// namespace engine::graphics

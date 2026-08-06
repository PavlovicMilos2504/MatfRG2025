// clang-format off
#include <glad/glad.h>
// clang-format on
#include <engine/graphics/Bloom.hpp>
#include <engine/graphics/OpenGL.hpp>
#include <engine/resources/Shader.hpp>
#include <engine/util/Errors.hpp>

namespace engine::graphics {

void Bloom::resize(int width, int height) {
    m_width = width;
    m_height = height;
    destroy_framebuffers();
    create_framebuffers();
}

void Bloom::create_framebuffers() {
    // HDR framebuffer with two color attachments: scene color (location 0) and bright-pass color (location 1).
    CHECKED_GL_CALL(glGenFramebuffers, 1, &m_hdr_fbo);
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_hdr_fbo);

    uint32_t color_buffers[2];
    CHECKED_GL_CALL(glGenTextures, 2, color_buffers);
    for (uint32_t i = 0; i < 2; ++i) {
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, color_buffers[i]);
        CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D,
                        color_buffers[i], 0);
    }
    m_scene_texture = color_buffers[0];
    m_bright_texture = color_buffers[1];

    CHECKED_GL_CALL(glGenRenderbuffers, 1, &m_depth_rbo);
    CHECKED_GL_CALL(glBindRenderbuffer, GL_RENDERBUFFER, m_depth_rbo);
    CHECKED_GL_CALL(glRenderbufferStorage, GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_width, m_height);
    CHECKED_GL_CALL(glFramebufferRenderbuffer, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth_rbo);

    uint32_t attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    CHECKED_GL_CALL(glDrawBuffers, 2, attachments);

    RG_GUARANTEE(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                 "HDR framebuffer used by the Bloom effect is not complete.");

    // Ping-pong framebuffers used for the two-pass Gaussian blur of the bright-pass texture.
    CHECKED_GL_CALL(glGenFramebuffers, 2, m_pingpong_fbo);
    CHECKED_GL_CALL(glGenTextures, 2, m_pingpong_texture);
    for (uint32_t i = 0; i < 2; ++i) {
        CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_pingpong_fbo[i]);
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_pingpong_texture[i]);
        CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                        m_pingpong_texture[i], 0);
        RG_GUARANTEE(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                     "Ping-pong framebuffer used by the Bloom effect is not complete.");
    }

    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
}

void Bloom::destroy_framebuffers() {
    if (m_hdr_fbo == 0) {
        return;
    }
    CHECKED_GL_CALL(glDeleteFramebuffers, 1, &m_hdr_fbo);
    CHECKED_GL_CALL(glDeleteTextures, 1, &m_scene_texture);
    CHECKED_GL_CALL(glDeleteTextures, 1, &m_bright_texture);
    CHECKED_GL_CALL(glDeleteRenderbuffers, 1, &m_depth_rbo);
    CHECKED_GL_CALL(glDeleteFramebuffers, 2, m_pingpong_fbo);
    CHECKED_GL_CALL(glDeleteTextures, 2, m_pingpong_texture);
    m_hdr_fbo = 0;
}

void Bloom::begin_capture() const {
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_hdr_fbo);
    CHECKED_GL_CALL(glViewport, 0, 0, m_width, m_height);
    OpenGL::clear_buffers();
}

void Bloom::end_capture() const {
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
}

uint32_t Bloom::screen_quad_vao() {
    // NDC fullscreen quad (position.xy, uv.xy), cached like OpenGL::init_skybox_cube.
    static uint32_t quad_vao = 0;
    if (quad_vao != 0) {
        return quad_vao;
    }
    // clang-format off
    float vertices[] = {
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
    };
    // clang-format on
    uint32_t quad_vbo = 0;
    CHECKED_GL_CALL(glGenVertexArrays, 1, &quad_vao);
    CHECKED_GL_CALL(glGenBuffers, 1, &quad_vbo);
    CHECKED_GL_CALL(glBindVertexArray, quad_vao);
    CHECKED_GL_CALL(glBindBuffer, GL_ARRAY_BUFFER, quad_vbo);
    CHECKED_GL_CALL(glBufferData, GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
    CHECKED_GL_CALL(glEnableVertexAttribArray, 0);
    CHECKED_GL_CALL(glVertexAttribPointer, 0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) 0);// NOLINT
    CHECKED_GL_CALL(glEnableVertexAttribArray, 1);
    CHECKED_GL_CALL(glVertexAttribPointer, 1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),// NOLINT
                    (void *) (2 * sizeof(float)));                                     // NOLINT
    return quad_vao;
}

void Bloom::apply(const resources::Shader *blur_shader, const resources::Shader *composite_shader, float exposure,
                  int blur_passes) const {
    uint32_t quad_vao = screen_quad_vao();

    // Two-pass Gaussian blur of the bright-pass texture using the ping-pong framebuffers.
    bool horizontal = true;
    bool first_iteration = true;
    blur_shader->use();
    for (int i = 0; i < 2 * blur_passes; ++i) {
        CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_pingpong_fbo[horizontal]);
        blur_shader->set_bool("horizontal", horizontal);
        CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0);
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D,
                        first_iteration ? m_bright_texture : m_pingpong_texture[!horizontal]);
        blur_shader->set_int("image", 0);
        CHECKED_GL_CALL(glBindVertexArray, quad_vao);
        CHECKED_GL_CALL(glDrawArrays, GL_TRIANGLES, 0, 6);
        horizontal = !horizontal;
        first_iteration = false;
    }

    // Additive composite of the scene texture and the blurred bloom texture onto the currently bound framebuffer.
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
    composite_shader->use();
    CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_scene_texture);
    composite_shader->set_int("scene", 0);
    CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE1);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_pingpong_texture[!horizontal]);
    composite_shader->set_int("bloomBlur", 1);
    composite_shader->set_float("exposure", exposure);
    CHECKED_GL_CALL(glBindVertexArray, quad_vao);
    CHECKED_GL_CALL(glDrawArrays, GL_TRIANGLES, 0, 6);
}

void Bloom::destroy() {
    destroy_framebuffers();
}
}// namespace engine::graphics

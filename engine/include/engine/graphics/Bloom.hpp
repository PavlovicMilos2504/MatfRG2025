/**
 * @file Bloom.hpp
 * @brief Defines the Bloom class that implements the HDR Bloom post-processing effect.
 */

#ifndef BLOOM_HPP
#define BLOOM_HPP

#include <cstdint>

namespace engine::resources {
class Shader;
}

namespace engine::graphics {
/**
 * @class Bloom
 * @brief Implements the HDR Bloom post-processing effect: bright-pass extraction (MRT),
 * two-pass Gaussian blur (ping-pong framebuffers), and additive composite with tone mapping.
 *
 * Usage:
 * @code
 * bloom->begin_capture();  // bind HDR framebuffer, clear it
 * // ... draw the lit scene; the scene's shader must write bright fragments
 * // to the second color attachment (see resources/shaders/lighting.glsl)
 * bloom->end_capture();    // restore the default framebuffer
 * bloom->apply(blur_shader, composite_shader, exposure); // blur + composite onto the screen
 * @endcode
 */
class Bloom final {
public:
    /**
    * @brief (Re)creates the framebuffers to match the given size. Should be called once after
    * construction and whenever the window is resized.
    */
    void resize(int width, int height);

    /**
    * @brief Binds the HDR framebuffer (two color attachments: scene color, bright-pass color) and clears it.
    */
    void begin_capture() const;

    /**
    * @brief Unbinds the HDR framebuffer, restoring the default framebuffer as the render target.
    */
    void end_capture() const;

    /**
    * @brief Blurs the bright-pass texture using a two-pass Gaussian blur, then additively composites
    * it with the scene texture, tone maps, and draws the result as a fullscreen quad onto the
    * currently bound framebuffer.
    * @param blur_shader Shader with uniforms: `sampler2D image`, `bool horizontal`, `float blurStride`.
    * @param composite_shader Shader with uniforms: `sampler2D scene`, `sampler2D bloomBlur`, `float exposure`.
    * @param exposure Tone mapping exposure value.
    * @param blur_passes Number of blur iterations (each iteration is one horizontal + one vertical pass).
    * @param blur_stride Per-tap texel offset multiplier; larger values widen the glow spread.
    */
    void apply(const resources::Shader *blur_shader, const resources::Shader *composite_shader,
               float exposure, int blur_passes = 10, float blur_stride = 1.0f) const;

    /**
    * @brief Destroys all the OpenGL objects owned by this instance.
    */
    void destroy();

private:
    void create_framebuffers();
    void destroy_framebuffers();
    static uint32_t screen_quad_vao();

    int m_width{0};
    int m_height{0};

    uint32_t m_hdr_fbo{0};
    uint32_t m_scene_texture{0};
    uint32_t m_bright_texture{0};
    uint32_t m_depth_rbo{0};

    uint32_t m_pingpong_fbo[2]{0, 0};
    uint32_t m_pingpong_texture[2]{0, 0};
};
}// namespace engine::graphics

#endif//BLOOM_HPP

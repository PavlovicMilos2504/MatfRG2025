//#shader vertex
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main() {
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 0.0, 1.0);
}

//#shader fragment
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float exposure;

// Additively blends the blurred bright-pass with the scene, then applies Reinhard
// tone mapping and gamma correction, see: https://learnopengl.com/Advanced-Lighting/Bloom
void main() {
    const float gamma = 2.2;
    vec3 hdr_color = texture(scene, TexCoords).rgb;
    vec3 bloom_color = texture(bloomBlur, TexCoords).rgb;
    hdr_color += bloom_color;

    vec3 result = vec3(1.0) - exp(-hdr_color * exposure);
    result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0);
}

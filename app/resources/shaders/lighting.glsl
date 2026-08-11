//#shader vertex
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}

//#shader fragment
#version 330 core

struct DirLight {
    bool enabled;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    bool enabled;
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;
// Model does not provide dedicated specular maps (PBR baseColor workflow),
// so a constant specular strength is used instead.
uniform float specularStrength;
uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLight;
// Fragments with luminance above this value are written to the bright-pass
// buffer used by the Bloom post-processing effect (see engine::graphics::Bloom).
uniform float bloomThreshold;

// Omnidirectional shadow map for the lamp point light (see engine::graphics::PointShadow).
uniform samplerCube shadowMap;
uniform float farPlane;
uniform float shadowBias;
uniform bool shadowsEnabled;

// Roughly separable sample offsets for softening shadow edges (PCF) with few cubemap samples.
const vec3 sampleOffsetDirections[20] = vec3[](
    vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
    vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
    vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
    vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1)
);

float shadow_calculation(vec3 frag_pos) {
    if (!shadowsEnabled) {
        return 0.0;
    }
    vec3 frag_to_light = frag_pos - pointLight.position;
    float current_depth = length(frag_to_light);

    float view_distance = length(viewPos - frag_pos);
    float disk_radius = (1.0 + (view_distance / farPlane)) / 25.0;

    float shadow = 0.0;
    int samples = 20;
    for (int i = 0; i < samples; ++i) {
        float closest_depth = texture(shadowMap, frag_to_light + sampleOffsetDirections[i] * disk_radius).r;
        closest_depth *= farPlane;// undo [0,1] mapping written by the depth pass
        if (current_depth - shadowBias > closest_depth) {
            shadow += 1.0;
        }
    }
    return shadow / float(samples);
}

vec3 calculate_dir_light(DirLight light, vec3 normal, vec3 view_dir, vec3 diffuse_color, vec3 specular_color) {
    if (!light.enabled) {
        return vec3(0.0);
    }
    vec3 light_dir = normalize(-light.direction);
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float spec = pow(max(dot(normal, halfway_dir), 0.0), 32.0);

    vec3 ambient = light.ambient * diffuse_color;
    vec3 diffuse = light.diffuse * diff * diffuse_color;
    vec3 specular = light.specular * spec * specular_color;
    return ambient + diffuse + specular;
}

vec3 calculate_point_light(PointLight light, vec3 normal, vec3 frag_pos, vec3 view_dir, vec3 diffuse_color, vec3 specular_color, float shadow) {
    if (!light.enabled) {
        return vec3(0.0);
    }
    vec3 light_dir = normalize(light.position - frag_pos);
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float spec = pow(max(dot(normal, halfway_dir), 0.0), 32.0);

    float distance = length(light.position - frag_pos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    vec3 ambient = light.ambient * diffuse_color * attenuation;
    vec3 diffuse = light.diffuse * diff * diffuse_color * attenuation;
    vec3 specular = light.specular * spec * specular_color * attenuation;
    // Only diffuse/specular are occluded by shadows; ambient still reaches shadowed fragments.
    return ambient + (1.0 - shadow) * (diffuse + specular);
}

void main() {
    vec3 normal = normalize(Normal);
    vec3 view_dir = normalize(viewPos - FragPos);
    vec3 diffuse_color = texture(texture_diffuse1, TexCoords).rgb;
    vec3 specular_color = vec3(specularStrength);

    vec3 result = calculate_dir_light(dirLight, normal, view_dir, diffuse_color, specular_color);
    float shadow = shadow_calculation(FragPos);
    result += calculate_point_light(pointLight, normal, FragPos, view_dir, diffuse_color, specular_color, shadow);

    FragColor = vec4(result, 1.0);

    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > bloomThreshold) {
        BrightColor = vec4(FragColor.rgb, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}

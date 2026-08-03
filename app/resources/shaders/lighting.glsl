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

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLight;

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

vec3 calculate_point_light(PointLight light, vec3 normal, vec3 frag_pos, vec3 view_dir, vec3 diffuse_color, vec3 specular_color) {
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
    return ambient + diffuse + specular;
}

void main() {
    vec3 normal = normalize(Normal);
    vec3 view_dir = normalize(viewPos - FragPos);
    vec3 diffuse_color = texture(texture_diffuse1, TexCoords).rgb;
    vec3 specular_color = texture(texture_specular1, TexCoords).rgb;

    vec3 result = calculate_dir_light(dirLight, normal, view_dir, diffuse_color, specular_color);
    result += calculate_point_light(pointLight, normal, FragPos, view_dir, diffuse_color, specular_color);

    FragColor = vec4(result, 1.0);
}

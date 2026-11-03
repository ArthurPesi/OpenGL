#version 410

in vec2 texCoord;
in vec3 normal;
in vec3 fragPos;

out vec4 frag_color;

#define NR_POINT_LIGHTS 4

// Phong material
uniform vec3 matAmbient;
uniform vec3 matDiffuse;
uniform vec3 matSpecular;
uniform float matShininess;

uniform sampler2D diffuseTexture;

uniform vec3 viewPos;

// --- Light casters ------------------------------------------------------
struct DirLight {
    vec3 direction;   // direction from fragment toward the light
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    // Attenuation: 1 / (constant + linear*d + quadratic*d^2)
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;     // direction the spotlight points at
    float cutOff;       // cosine of inner cone angle
    float outerCutOff;  // cosine of outer cone angle (for soft edges)
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;

// --- Distance fog -------------------------------------------------------
uniform vec3 fogColor;     // color distant geometry fades to
uniform float fogDensity;  // exponential-squared fog; 0 disables

// Shared per-fragment surface color sampled from the diffuse texture.
vec3 texColor;

vec3 CalcDirLight(DirLight light, vec3 n, vec3 viewDir) {
    vec3 lightDir = normalize(light.direction);

    float diff = max(dot(n, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, n);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(matShininess, 1.0));

    vec3 ambient  = light.ambient  * matAmbient  * texColor;
    vec3 diffuse  = light.diffuse  * diff * matDiffuse * texColor;
    vec3 specular = light.specular * spec * matSpecular;
    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 n, vec3 viewDir) {
    vec3 toLight = light.position - fragPos;
    float dist = length(toLight);
    vec3 lightDir = toLight / max(dist, 1e-4);

    float diff = max(dot(n, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, n);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(matShininess, 1.0));

    float attenuation = 1.0 / (light.constant + light.linear * dist +
                               light.quadratic * dist * dist);

    vec3 ambient  = light.ambient  * matAmbient  * texColor;
    vec3 diffuse  = light.diffuse  * diff * matDiffuse * texColor;
    vec3 specular = light.specular * spec * matSpecular;
    return (ambient + diffuse + specular) * attenuation;
}

vec3 CalcSpotLight(SpotLight light, vec3 n, vec3 viewDir) {
    vec3 toLight = light.position - fragPos;
    float dist = length(toLight);
    vec3 lightDir = toLight / max(dist, 1e-4);

    float diff = max(dot(n, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, n);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(matShininess, 1.0));

    float attenuation = 1.0 / (light.constant + light.linear * dist +
                               light.quadratic * dist * dist);

    // Soft cone: intensity fades between the inner and outer cutoff cosines.
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / max(epsilon, 1e-4), 0.0, 1.0);

    vec3 ambient  = light.ambient  * matAmbient  * texColor;
    vec3 diffuse  = light.diffuse  * diff * matDiffuse * texColor;
    vec3 specular = light.specular * spec * matSpecular;

    // Ambient stays constant inside attenuation; cone only gates diffuse/specular.
    ambient  *= attenuation;
    diffuse  *= attenuation * intensity;
    specular *= attenuation * intensity;
    return ambient + diffuse + specular;
}

void main() {
    vec3 n = normalize(normal);
    vec3 viewDir = normalize(viewPos - fragPos);
    texColor = texture(diffuseTexture, texCoord).rgb;

    vec3 result = CalcDirLight(dirLight, n, viewDir);
    for (int i = 0; i < NR_POINT_LIGHTS; i++) {
        result += CalcPointLight(pointLights[i], n, viewDir);
    }
    result += CalcSpotLight(spotLight, n, viewDir);

    // Distance fog: blend toward fogColor with exponential-squared falloff.
    float dist = length(viewPos - fragPos);
    float fogFactor = clamp(exp(-(fogDensity * dist) * (fogDensity * dist)), 0.0, 1.0);
    result = mix(fogColor, result, fogFactor);

    frag_color = vec4(result, 1.0);
}

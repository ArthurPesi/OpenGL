#version 410

in vec2 texCoord;
in vec3 normal;
in vec3 fragPos;

out vec4 frag_color;

// Phong material 
uniform vec3 matAmbient;
uniform vec3 matDiffuse;
uniform vec3 matSpecular;
uniform float matShininess;

uniform sampler2D diffuseTexture;

uniform vec3 lightDir;   
uniform vec3 lightColor;
uniform vec3 viewPos;

void main () {
    vec3 n = normalize(normal);

    // Ambient
    vec3 ambient = matAmbient * lightColor;

    // Diffuse
    float diff = max(dot(n, lightDir), 0.0);
    vec3 diffuse = diff * matDiffuse * lightColor;

    // Specular
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, n);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(matShininess, 1.0));
    vec3 specular = spec * matSpecular * lightColor;

    vec3 texColor = texture(diffuseTexture, texCoord).rgb;
    const float texFloor = 0.3;
    vec3 surface = ambient + diffuse + texFloor * lightColor;
    vec3 color = surface * texColor + specular;
    frag_color = vec4(color, 1.0);
}

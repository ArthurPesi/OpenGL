#version 410

in vec2 texCoord;
in vec3 normal;
in vec3 fragPos;

out vec4 frag_color;

// Phong material (from the .mtl file)
uniform vec3 matAmbient;
uniform vec3 matDiffuse;
uniform vec3 matSpecular;
uniform float matShininess;

// Diffuse texture (map_Kd). A 1x1 white texture is bound when the
// material has no map, so the multiply below is a no-op in that case.
uniform sampler2D diffuseTexture;

// Directional light from the sky + camera
uniform vec3 lightDir;   // normalized direction toward the light (world space)
uniform vec3 lightColor;
uniform vec3 viewPos;

void main () {
    vec3 texColor = texture(diffuseTexture, texCoord).rgb;

    vec3 n = normalize(normal);
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, n);

    // Ambient
    vec3 ambient = matAmbient * lightColor;

    // Diffuse
    float diff = max(dot(n, lightDir), 0.0);
    vec3 diffuse = diff * matDiffuse * lightColor;

    // Specular
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(matShininess, 1.0));
    vec3 specular = spec * matSpecular * lightColor;

    // Texture modulates the ambient + diffuse (surface) term; specular is a highlight.
    // A small constant floor keeps the diffuse map visible even when the material's
    // Ka/Kd are zero (some exporters write Kd 0 0 0 and rely on the texture alone).
    const float texFloor = 0.3;
    vec3 surface = ambient + diffuse + texFloor * lightColor;
    vec3 color = surface * texColor + specular;
    frag_color = vec4(color, 1.0);
}

#version 410

in vec2 texCoord;
in vec3 normal;
in vec3 fragPos;

out vec4 frag_color;


void main () {
    vec3 lightPos = vec3(0.0, 1.0, 0.0);
    vec3 n = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(n, lightDir), 0.0);

    vec3 baseColor = vec3(0.5, 0.7, 0.1);
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 ambient = 0.1 * baseColor;
    vec3 color = ambient + diff * lightColor * baseColor;

    frag_color = vec4 (color, 1.0);
}

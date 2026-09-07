#version 410

in vec2 texCoord;
in vec3 normal;
in vec3 fragPos;

out vec4 frag_color;

void main () {
    vec3 lightDir = vec3(0.0, -1.0, 0.0);
    float angle = max(dot(normal, lightDir), 0.0);

    vec3 baseColor = vec3(0.5, 0.7, 0.1);
    vec3 ambient = vec3(0.1, 0.1, 0.1);
    vec3 color = (ambient + angle) * baseColor;

    frag_color = vec4 (color, 1.0);
}

#version 410

in vec2 texCoord;
in vec3 normal;

out vec4 frag_color;

void main () {
    vec3 baseColor = vec3(0.5, 0.7, 0.1);
    frag_color = vec4 (baseColor, 1.0);
}

#version 410

in vec2 texCoord;
in vec3 normal;

out vec4 frag_color;

void main () {
    frag_color = vec4 (normal, 1.0);
}
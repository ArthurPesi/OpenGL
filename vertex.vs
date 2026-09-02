#version 410

layout(location=0) in vec3 vp;
layout(location=1) in vec2 vt;
layout(location=2) in vec3 vn;

out vec2 texCoord;
out vec3 normal;

void main () {
    texCoord = vt;
    normal = vn;
    gl_Position = vec4 (vp, 1.0);
}
#version 410

layout(location=0) in vec3 vp;
layout(location=1) in vec2 vt;
layout(location=2) in vec3 vn;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 transform;

out vec2 texCoord;
out vec3 normal;
out vec3 fragPos;

void main () {
    texCoord = vt;
    normal = vn;
    fragPos = vec3 (view * vec4 (vp, 1.0));
    gl_Position = projection * view * transform * vec4 (vp, 1.0);
}
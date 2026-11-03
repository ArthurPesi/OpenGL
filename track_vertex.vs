#version 410

layout(location=0) in vec2 pos;

uniform mat4 projection;
uniform float pointSize;

void main () {
    // Core profile ignores glPointSize(); gl_PointSize must be set here.
    gl_PointSize = pointSize;
    gl_Position = projection * vec4(pos, 0.0, 1.0);
}

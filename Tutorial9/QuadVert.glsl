#version 430 core

layout(location = 0) in vec2 v2VertexPos;

void main() {
    gl_Position = vec4(v2VertexPos, 0.0f, 1.0f);
}
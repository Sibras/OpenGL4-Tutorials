#version 430 core

layout(location = 0) in vec2 v2VertexPos2D;

void main()
{
    gl_Position = vec4(v2VertexPos2D, 0.0f, 1.0f);
}
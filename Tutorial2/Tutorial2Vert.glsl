#version 430 core

layout(binding = 0) uniform TransformData {
    mat4 m4Transform;
};
layout(binding = 1) uniform CameraData {
    mat4 m4ViewProjection;
};

layout(location = 0) in vec3 v3VertexPos;

void main()
{
    gl_Position = m4ViewProjection * (m4Transform * vec4(v3VertexPos, 1.0f));
}
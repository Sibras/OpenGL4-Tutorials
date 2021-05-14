#version 430 core

layout(binding = 0) uniform TransformData {
    mat4 m4Transform;
};
layout(binding = 1) uniform CameraData {
    mat4 m4ViewProjection;
    vec3 v3CameraPosition;
};

layout(location = 0) in vec3 v3VertexPos;
layout(location = 1) in vec3 v3VertexNormal;

layout(location = 0) smooth out vec3 v3PositionOut;
layout(location = 1) smooth out vec3 v3NormalOut;

void main()
{
    // Transform vertex
    vec4 v4Position = m4Transform * vec4(v3VertexPos, 1.0f);
    gl_Position = m4ViewProjection * v4Position;
    v3PositionOut = v4Position.xyz;

    // Transform normal
    vec4 v4Normal = m4Transform * vec4(v3VertexNormal, 0.0f);
    v3NormalOut = v4Normal.xyz;
}
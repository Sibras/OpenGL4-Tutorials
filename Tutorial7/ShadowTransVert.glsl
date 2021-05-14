#version 430 core

layout(binding = 0) uniform TransformData {
    mat4 m4Transform;
};

layout(location = 0) in vec3 v3VertexPos;
layout(location = 2) in vec2 v2VertexUV;

layout(location = 0) smooth out vec3 v3PositionOut;
layout(location = 1) smooth out vec2 v2UVOut;

void main()
{
    // Transform vertex
    vec4 v4Position = m4Transform * vec4(v3VertexPos, 1.0f);
    v3PositionOut = v4Position.xyz;

    //Passthrough UV coordinatees
    v2UVOut = v2VertexUV;
}
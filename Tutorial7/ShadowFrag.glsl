#version 430 core

layout(location = 0) out float fFragDepth;

void main()
{
    // Not really needed, OpenGL does it anyway
    fFragDepth = gl_FragCoord.z;
}
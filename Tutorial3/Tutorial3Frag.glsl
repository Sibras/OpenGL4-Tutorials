#version 430 core

layout(location = 0) in vec3 v3ColourIn;
out vec3 v3ColourOut;

void main()
{
    v3ColourOut = v3ColourIn;
}
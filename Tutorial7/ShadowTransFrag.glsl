#version 430 core

layout(binding = 0) uniform sampler2D s2DiffuseTexture;

layout(location = 0) in vec2 v2UVIn;
out vec3 v3ColourOut;

void main()
{
    // Get texture data
    vec4 v4DiffuseColour = texture(s2DiffuseTexture, v2UVIn);

    // Just write out diffuse weighted by alpha
    v3ColourOut = v4DiffuseColour.rgb * (1.0f - v4DiffuseColour.w);
}
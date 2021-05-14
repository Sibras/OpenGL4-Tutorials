#version 430 core

layout(binding = 7) uniform InvResolution {
    vec2 v2InvResolution;
};
layout(binding = 17) uniform sampler2D s2InputTexture;

out vec3 v3ColourOut;

const float fGaussSigma = 2.5f;

subroutine vec2 SampleOffset(out vec2);
layout(location = 0) subroutine uniform SampleOffset SampleOffsetUniform;

layout(index = 0) subroutine(SampleOffset) vec2 horizSampleOffset(out vec2 v2Du1)
{
    // Calculate horizontal sample offsets
    v2Du1 = vec2(0.524f * v2InvResolution.x * fGaussSigma, 0.0f);
    return vec2(1.282f * v2InvResolution.x * fGaussSigma, 0.0f);
}

layout(index = 1) subroutine(SampleOffset) vec2 vertSampleOffset(out vec2 v2Du1)
{
    // Calculate vertical sample offsets
    v2Du1 = vec2(0.0f, 0.524f * v2InvResolution.y * fGaussSigma);
    return vec2(0.0f, 1.282f * v2InvResolution.y * fGaussSigma);
}

void main() {
    // Get UV coordinates
    vec2 v2UV = gl_FragCoord.xy * v2InvResolution;

    // Get sample offsets
    vec2 v2Du1;
    vec2 v2Du2 = SampleOffsetUniform(v2Du1);

    // Get filtered values
    vec3 v3Filtered = texture(s2InputTexture, v2UV - v2Du2).rgb +
                      texture(s2InputTexture, v2UV - v2Du1).rgb +
                      texture(s2InputTexture, v2UV).rgb +
                      texture(s2InputTexture, v2UV + v2Du1).rgb +
                      texture(s2InputTexture, v2UV + v2Du2).rgb;
    v3ColourOut = v3Filtered / 5.0f;
}
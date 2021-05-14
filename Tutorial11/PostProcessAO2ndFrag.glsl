#version 430 core

layout(binding = 7) uniform InvResolution {
    vec2 v2InvResolution;
};

//layout(binding = 11) uniform sampler2D s2DepthTexture;
layout(binding = 18) uniform sampler2D s2AOTexture;

out vec3 v3AOOut;

// Size of AO radius
const float fAORadius = 0.5f;

void main() {
    // Get UV coordinates
    vec2 v2UV = gl_FragCoord.xy * v2InvResolution;

    // Calculate 4x4 blur start and width
    vec2 v2StartUV = v2UV - (v2InvResolution * 1.5f);
    vec2 v2UVOffset = v2InvResolution * 2.0f;

    // Perform 4x4 blur of AO samples
    float fAO = texture(s2AOTexture, v2StartUV).r;
    fAO += texture(s2AOTexture, v2StartUV + vec2(v2UVOffset.x, 0.0f)).r;
    fAO += texture(s2AOTexture, v2StartUV + vec2(0.0f, v2UVOffset.y)).r;
    fAO += texture(s2AOTexture, v2StartUV + v2UVOffset).r;
    fAO *= 0.25f;

    v3AOOut = vec3(fAO);

    // Get current pixels depth
    //float fDist = texture(s2DepthTexture, v2UV).r;
    //float fDepth = (v2LinearDepth.x * fDist) + v2LinearDepth.y;
    //fDepth /= v2LinearDepth.z * fDist;
    //fDepth = (fDepth * 0.5f) + 0.5f;
    //
    //// Perform 4x4 smart blur of AO samples
    //float fAO = 0.0f;
    //float fSamples = 0.0f;
    //for (int y = -2; y < 2; y++) {
    //    for (int x = -2; x < 2; x++) {
    //        // Get current pixel offset
    //        vec2 v2UVOffset = v2UV + (v2InvResolution * vec2(float(x), float(y)));
    //
    //        // Perform depth check to ensure within hemisphere
    //        float fSampleDist = texture(s2DepthTexture, v2UVOffset).r;
    //        float fSampleDepth = (v2LinearDepth.x * fSampleDist) + v2LinearDepth.y;
    //        fSampleDepth /= v2LinearDepth.z * fSampleDist;
    //        fSampleDepth = (fSampleDepth * 0.5f) + 0.5f;
    //
    //        // Add in AO value
    //        if (abs(fSampleDepth - fDepth) < fAORadius) {
    //            fAO += texture(s2AOTexture, v2UVOffset).r;
    //            ++fSamples;
    //        }
    //    }
    //}
    //
    //v3AOOut = vec3(fAO / fSamples);
}
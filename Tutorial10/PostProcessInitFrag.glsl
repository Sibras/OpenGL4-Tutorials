#version 430 core

layout(binding = 7) uniform InvResolution {
    vec2 v2InvResolution;
};
layout(binding = 15) uniform sampler2D s2AccumulationTexture;

layout(location = 0) out float fYOut;
layout(location = 1) out vec3 v3YBloomOut;

const vec3 v3LuminanceConvert = vec3(0.2126f, 0.7152f, 0.0722f);
const float fEpsilon = 0.00000001f;
const float fYwhite = 0.22f;

void main() {
    // Get UV coordinates
    vec2 v2UV = gl_FragCoord.xy * v2InvResolution;

    // Get colour data
    vec3 v3AccumColour = texture(s2AccumulationTexture, v2UV).rgb;

    // Calculate luminance
    float fY = dot(v3AccumColour, v3LuminanceConvert);

    // Calculate log luminance
    float fYoutTemp = log(fY + fEpsilon);

    // Perform range mapping (optional: only needed with integer textures)
    fYoutTemp = (fYoutTemp - log(fEpsilon)) / (log(fYwhite) - log(fEpsilon));

    fYOut = fYoutTemp;

    // Output bloom values
    v3YBloomOut = (fY >= fYwhite * 2.95f)? v3AccumColour : vec3(0.0f);
}
#version 430 core

layout(binding = 7) uniform InvResolution {
    vec2 v2InvResolution;
};
layout(binding = 15) uniform sampler2D s2AccumulationTexture;
layout(binding = 16) uniform sampler2D s2LuminanceKeyTexture;
layout(binding = 17) uniform sampler2D s2BloomTexture;

out vec3 v3ColourOut;

const vec3 v3LuminanceConvert = vec3(0.2126f, 0.7152f, 0.0722f);
const float fEpsilon = 0.00000001f;
const float fYwhite = 0.22f;

float log10(in float fVal)
{
    // Log10(x) = log2(x) / log2(10)
    return log2(fVal) * 0.30102999566374217366165225171822f;
}

vec3 toneMap(vec3 v3RetColour, vec2 v2UV)
{
    // Get key luminance value
    float fYa = texture(s2LuminanceKeyTexture, v2UV, 1024).r;

    // Perform range mapping (optional: only needed with integer textures)
    fYa = (fYa * (log(fYwhite) - log(fEpsilon))) + log(fEpsilon);

    // Get exponent of log values
    fYa = exp(fYa);

    // Calculate middle-grey
    float fDg = 1.03f - (2.0f / (2.0f + log10(fYa + 1.0f)));

    // Calculate current luminance
    float fY = dot(v3RetColour, v3LuminanceConvert);

    // Calculate relative luminance
    float fYr = (fDg / fYa) * fY;

    // Calculate new luminance
    float fYNew = (fYr * (1.0f + (fYr / (fYwhite * fYwhite)))) / (1.0f + fYr);

    // Perform tone mapping
    return v3RetColour * (fYNew / fY);
}

vec3 bloom(vec3 v3RetColour, vec2 v2UV)
{
    // Perform bloom addition
    vec3 v3Bloom = texture(s2BloomTexture, v2UV, 1024).rgb;
    return v3RetColour + (v3Bloom * 0.98f);
}

void main() {
    // Get UV coordinates
    vec2 v2UV = gl_FragCoord.xy * v2InvResolution;

    // Get colour data
    vec3 v3RetColour = texture(s2AccumulationTexture, v2UV).rgb;

    // Perform bloom
    v3RetColour = bloom(v3RetColour, v2UV);

    // Perform tone map
    v3RetColour = toneMap(v3RetColour, v2UV);

    v3ColourOut = v3RetColour;
}
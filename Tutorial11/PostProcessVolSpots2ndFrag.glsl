#version 430 core

layout(binding = 7) uniform InvResolution {
    vec2 v2InvResolution;
};
layout(binding = 19) uniform sampler2D s2VolumeLightTexture;

out vec3 v3VolumeOut;

void main() {
    // Get UV coordinates
    vec2 v2UV = gl_FragCoord.xy * v2InvResolution;

    // Pass through volume texture
    v3VolumeOut = texture(s2VolumeLightTexture, v2UV).rgb;
}
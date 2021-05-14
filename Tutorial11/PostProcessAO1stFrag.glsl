#version 430 core

layout(binding = 1) uniform CameraData {
    mat4 m4ViewProjection;
    vec3 v3CameraPosition;
    mat4 m4InvViewProjection;
};
layout(binding = 7) uniform InvResolution {
    vec2 v2InvResolution;
};

layout(binding = 11) uniform sampler2D s2DepthTexture;
layout(binding = 12) uniform sampler2D s2NormalTexture;

out float fAOOut;

// Size of AO radius
const float fAORadius = 0.5f;
const float fEpsilon = 0.00000001f;

void main() {
    // Get UV coordinates
    vec2 v2UV = gl_FragCoord.xy * v2InvResolution;

    // Get deferred data
    float fDepth = texture(s2DepthTexture, v2UV).r;
    vec3 v3Normal = texture(s2NormalTexture, v2UV).rgb;

    // Calculate position from depth
    fDepth = (fDepth * 2.0f) - 1.0f;
    vec2 v2NDCUV = (v2UV * 2.0f) - 1.0f;
    vec4 v4Position = m4InvViewProjection * vec4(v2NDCUV, fDepth, 1.0);
    vec3 v3PositionIn = v4Position.xyz / v4Position.w;

    // Define sampling values
    const vec3 v3Samples[16] = vec3[](
        vec3( 0.000000000f, -0.066666670f, 0.188561812f),
        vec3(-0.200000003f,  0.133333340f, 0.319722116f),
        vec3( 0.300000012f, -0.466666669f, 0.228521824f),
        vec3(-0.600000024f, -0.088888891f, 0.521630883f),
        vec3( 0.009999999f,  0.022222222f, 0.031720228f),
        vec3(-0.059999998f, -0.133333340f, 0.190321371f),
        vec3( 0.330000013f,  0.048888888f, 0.286896974f),
        vec3( 0.079999998f, -0.592592597f, 0.228109658f),
        vec3(-0.314999998f, -0.217777774f, 0.747628152f),
        vec3( 0.050000000f,  0.032592590f, 0.053270280f),
        vec3(-0.174999997f, -0.197037041f, 0.094611868f),
        vec3( 0.180000007f, -0.017777778f, 0.444616646f),
        vec3(-0.085000000f,  0.428148150f, 0.521405935f),
        vec3( 0.769999981f, -0.423703700f, 0.044442899f),
        vec3(-0.112499997f,  0.022222222f, 0.035354249f),
        vec3( 0.019999999f,  0.272592604f, 0.166412979f)
       );

    // Get 4x4 sample
    ivec2 i2SamplePos = ivec2(gl_FragCoord.xy) % 4;
    vec3 v3Sample = v3Samples[(i2SamplePos.y * 4) + i2SamplePos.x];

    // Determine world space tangent axis
    vec3 v3SampleTangent = vec3(v3Sample.xy, 0.0f);
    vec3 v3Tangent = normalize(v3SampleTangent - v3Normal * dot(v3SampleTangent, v3Normal));
    vec3 v3BiTangent = cross(v3Normal, v3Tangent);
    mat3 m3TangentAxis = mat3(v3Tangent,
        v3BiTangent,
        v3Normal);

    float fAO = 0.0f;
    for (int i = 0; i < 16; i++) {
        // Offset position along tangent plane
        vec3 v3Offset = m3TangentAxis * v3Samples[i] * fAORadius;
        vec3 v3OffsetPos = v3PositionIn + v3Offset;

        // Compute screen space coordinates
        vec4 v4OffsetPos = m4ViewProjection * vec4(v3OffsetPos, 1.0);
        v3OffsetPos = v4OffsetPos.xyz / v4OffsetPos.w;
        v3OffsetPos = (v3OffsetPos * 0.5f) + 0.5f;

        // Read depth buffer
        float fSampleDepth = texture(s2DepthTexture, v3OffsetPos.xy).r;

        // Compare sample depth with depth buffer value
        float fRangeCheck = (abs(v3OffsetPos.z - fSampleDepth) < fAORadius)? 1.0f : 0.0f;
        fAO += (fSampleDepth <= v3OffsetPos.z + fEpsilon)? fRangeCheck : 0.0f;
    }

    fAOOut = 1.0f - (fAO * 0.0625f);
}
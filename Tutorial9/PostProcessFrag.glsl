#version 430 core

layout(binding = 7) uniform InvResolution {
    vec2 v2InvResolution;
};
layout(binding = 15) uniform sampler2D s2AccumulationTexture;

out vec3 v3ColourOut;

//Additional textures for GBuffer display
layout(binding = 11) uniform sampler2D s2DepthTexture;
layout(binding = 12) uniform sampler2D s2NormalTexture;
layout(binding = 13) uniform sampler2D s2DiffuseTexture;
layout(binding = 14) uniform sampler2D s2SpecularRoughTexture;
layout(std140, binding = 1) uniform CameraData {
    mat4 m4ViewProjection;
    vec3 v3CameraPosition;
    mat4 m4InvViewProjection;
};

subroutine vec3 OutputBuffer(vec2);
layout(location = 0) subroutine uniform OutputBuffer OutputBufferUniform;
layout(index = 0) subroutine(OutputBuffer) vec3 accumulation(vec2 v2UV)
{
    return texture(s2AccumulationTexture, v2UV).rgb;
}

layout(index = 1) subroutine(OutputBuffer) vec3 depth(vec2 v2UV)
{
    // Calculate near and far frustum positions using inverse view projection
    // Note: This is much slower than just passing in the near/far values directly, however as
    //    this is for a simple debug output then there is no need to modify the existing UBOs
    vec4 v4Far = m4InvViewProjection * vec4(0.0f, 0.0f, 1.0f, 1.0f);
    vec3 v3Far = vec3(v4Far) / v4Far.w;
    vec4 v4Near = m4InvViewProjection * vec4(0.0f, 0.0f, -1.0f, 1.0f);
    vec3 v3Near = vec3(v4Near) / v4Near.w;

    // Calculate near and far distances as length of vector from origin to near/far frustum point
    vec2 v2NearFar = vec2(length(v3Near - v3CameraPosition), length(v3Far - v3CameraPosition));

    // Get depth from input texture
    float fDepthVal = texture(s2DepthTexture, v2UV).r;

    // Convert depth to -1->1 range
    fDepthVal = (2.0f * fDepthVal) - 1.0f;

    // Calculate world space depth by doing inverse projection operation (using -z)
    float fDepth = (2.0f * v2NearFar.x * v2NearFar.y) / ((v2NearFar.y + v2NearFar.x) - (fDepthVal * (v2NearFar.y - v2NearFar.x)));

    // Scale the new depth value between near and far (0->1 range)
    fDepth = (fDepth - v2NearFar.x) / (v2NearFar.y - v2NearFar.x);
    return vec3(fDepth);
}

layout(index = 2) subroutine(OutputBuffer) vec3 normal(vec2 v2UV)
{
    return texture(s2NormalTexture, v2UV).rgb;
}

layout(index = 3) subroutine(OutputBuffer) vec3 diffuse(vec2 v2UV)
{
    return texture(s2DiffuseTexture, v2UV).rgb;
}

layout(index = 4) subroutine(OutputBuffer) vec3 specular(vec2 v2UV)
{
    return texture(s2SpecularRoughTexture, v2UV).rgb;
}

layout(index = 5) subroutine(OutputBuffer) vec3 rough(vec2 v2UV)
{
    return vec3(texture(s2SpecularRoughTexture, v2UV).a);
}

void main() {
    // Get UV coordinates
    vec2 v2UV = gl_FragCoord.xy * v2InvResolution;

    // Pass through colour data
    //v3ColourOut = texture(s2AccumulationTexture, v2UV).rgb;

    //Use subroutines to toggle GBuffer display
    v3ColourOut = OutputBufferUniform(v2UV);
}
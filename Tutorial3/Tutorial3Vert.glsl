#version 430 core

layout(binding = 0) uniform TransformData {
    mat4 m4Transform;
};
layout(binding = 1) uniform CameraData {
    mat4 m4ViewProjection;
    vec3 v3CameraPosition;
};
struct PointLight {
    vec3 v3LightPosition;
    vec3 v3LightIntensity;
    vec3 v3Falloff;
};
#define MAX_LIGHTS 16
layout(std140, binding = 2) uniform PointLightData {
    PointLight PointLights[MAX_LIGHTS];
};
layout(location = 0) uniform int iNumPointLights;
layout(std140, binding = 3) uniform MaterialData {
    vec3  v3DiffuseColour;
    vec3  v3SpecularColour;
    float fRoughness;
};

#define M_RCPPI 0.31830988618379067153776752674503f
#define M_PI 3.1415926535897932384626433832795f

layout(location = 0) in vec3 v3VertexPos;
layout(location = 1) in vec3 v3VertexNormal;

layout(location = 0) smooth out vec3 v3ColourOut;

vec3 lightFalloff(in vec3 v3LightIntensity, in vec3 v3Falloff, in vec3 v3LightPosition, in vec3 v3Position)
{
    // Calculate distance from light
    float fDist = distance(v3LightPosition, v3Position);

    // Return falloff
    //return v3LightIntensity / (fFalloff * fDist * fDist);
    float fFalloff = v3Falloff.x + (v3Falloff.y * fDist) + (v3Falloff.z * fDist * fDist);
    return v3LightIntensity / fFalloff;
}

vec3 blinnPhong(in vec3 v3Normal, in vec3 v3LightDirection, in vec3 v3ViewDirection, in vec3 v3LightIrradiance, in vec3 v3DiffuseColour, in vec3 v3SpecularColour, in float fRoughness)
{
    // Get diffuse component
    vec3 v3Diffuse = v3DiffuseColour;

    // Calculate half vector
    vec3 v3HalfVector = normalize(v3ViewDirection + v3LightDirection);

    // Convert roughness to Phong shininess
    float fRoughnessPhong = (2.0f / (fRoughness * fRoughness)) - 2.0f;

    // Calculate specular component
    vec3 v3Specular = pow(max(dot(v3Normal, v3HalfVector), 0.0f), fRoughnessPhong) * v3SpecularColour;

    // Normalise diffuse and specular component and add
    v3Diffuse *= M_RCPPI;
    v3Specular *= (fRoughnessPhong + 8.0f) / (8.0f * M_PI);

    // Combine diffuse and specular
    vec3 v3RetColour = v3Diffuse + v3Specular;

    // Multiply by view angle
    v3RetColour *= max(dot(v3Normal, v3LightDirection), 0.0f);

    // Combine with incoming light value
    v3RetColour *= v3LightIrradiance;

    return v3RetColour;
}

void main()
{
    // Transform vertex
    vec4 v4Position = m4Transform * vec4(v3VertexPos, 1.0f);
    gl_Position = m4ViewProjection * v4Position;

    // Transform normal
    vec4 v4Normal = m4Transform * vec4(v3VertexNormal, 0.0f);

    // Normalise the inputs
    vec3 v3Position = v4Position.xyz / v4Position.w;
    vec3 v3Normal = normalize(v4Normal.xyz);
    vec3 v3ViewDirection = normalize(v3CameraPosition - v3Position);

    //vec3 v3RetColour = blinnPhong(v3Normal, v3LightDirection, v3ViewDirection, v3LightIrradiance, v3DiffuseColour, v3SpecularColour, fRoughness);
    // Loop over each point light
    vec3 v3RetColour = vec3(0.0f);
    for (int i = 0; i < iNumPointLights; i++) {
        vec3 v3LightDirection = normalize(PointLights[i].v3LightPosition - v3Position);

        // Calculate light falloff
        vec3 v3LightIrradiance = lightFalloff(PointLights[i].v3LightIntensity, PointLights[i].v3Falloff, PointLights[i].v3LightPosition, v3Position);

        // Perform shading
        v3RetColour += blinnPhong(v3Normal, v3LightDirection, v3ViewDirection, v3LightIrradiance, v3DiffuseColour, v3SpecularColour, fRoughness);
    }

    // Add in ambient contribution
    v3RetColour += v3DiffuseColour * vec3(0.3f);
    v3ColourOut = v3RetColour;
}
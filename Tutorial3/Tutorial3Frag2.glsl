#version 430 core

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
layout(binding = 2) uniform PointLightData {
    PointLight PointLights[MAX_LIGHTS];
};
layout(location = 0) uniform int iNumPointLights;
layout(binding = 3) uniform MaterialData {
    vec3  v3DiffuseColour;
    vec3  v3SpecularColour;
    float fRoughness;
};

subroutine vec3 BRDF(in vec3, in vec3, in vec3, in vec3, in vec3, in vec3, in float);
layout(location = 0) subroutine uniform BRDF BRDFUniform;

#define M_RCPPI 0.31830988618379067153776752674503f
#define M_PI 3.1415926535897932384626433832795f

layout(location = 0) in vec3 v3PositionIn;
layout(location = 1) in vec3 v3NormalIn;
out vec3 v3ColourOut;


vec3 lightFalloff(in vec3 v3LightIntensity, in vec3 v3Falloff, in vec3 v3LightPosition, in vec3 v3Position)
{
    // Calculate distance from light
    float fDist = distance(v3LightPosition, v3Position);

    // Return falloff
    //return v3LightIntensity / (fFalloff * fDist * fDist);
    float fFalloff = v3Falloff.x + (v3Falloff.y * fDist) + (v3Falloff.z * fDist * fDist);
    return v3LightIntensity / fFalloff;
}

layout(index = 0) subroutine(BRDF) vec3 blinnPhong(in vec3 v3Normal, in vec3 v3LightDirection, in vec3 v3ViewDirection, in vec3 v3LightIrradiance, in vec3 v3DiffuseColour, in vec3 v3SpecularColour, in float fRoughness)
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

vec3 schlickFresnel(in vec3 v3LightDirection, in vec3 v3Normal, in vec3 v3SpecularColour)
{
    // Schlick Fresnel approximation
    float fLH = dot(v3LightDirection, v3Normal);
    return v3SpecularColour + (1.0f - v3SpecularColour) * pow(1.0f - fLH, 5);
}

float TRDistribution(in vec3 v3Normal, in vec3 v3HalfVector, in float fRoughness)
{
    // Trowbridge-Reitz Distribution function
    float fNSq = fRoughness * fRoughness;
    float fNH = max(dot(v3Normal, v3HalfVector), 0.0f);
    float fDenom = fNH * fNH * (fNSq - 1.0f) + 1.0f;
    return fNSq / (M_PI * fDenom * fDenom);
}

float GGXVisibility(in vec3 v3Normal, in vec3 v3LightDirection, in vec3 v3ViewDirection, in float fRoughness)
{
    // GGX Visibility function
    float fNL = max(dot(v3Normal, v3LightDirection), 0.0f);
    float fNV = max(dot(v3Normal, v3ViewDirection), 0.0f);
    float fRSq = fRoughness * fRoughness;
    float fRMod = 1.0f - fRSq;
    float fRecipG1 = fNL + sqrt(fRSq + (fRMod * fNL * fNL));
    float fRecipG2 = fNV + sqrt(fRSq + (fRMod * fNV * fNV));

    return 1.0f / (fRecipG1 * fRecipG2);
}

layout(index = 1) subroutine(BRDF) vec3 GGX(in vec3 v3Normal, in vec3 v3LightDirection, in vec3 v3ViewDirection, in vec3 v3LightIrradiance, in vec3 v3DiffuseColour, in vec3 v3SpecularColour, in float fRoughness)
{
    // Calculate diffuse component
    vec3 v3Diffuse = v3DiffuseColour * M_RCPPI;

    // Calculate half vector
    vec3 v3HalfVector = normalize(v3ViewDirection + v3LightDirection);

    // Calculate Toorance-Sparrow components
    vec3 v3F = schlickFresnel(v3LightDirection, v3HalfVector, v3SpecularColour);
    float fD = TRDistribution(v3Normal, v3HalfVector, fRoughness);
    float fV = GGXVisibility(v3Normal, v3LightDirection, v3ViewDirection, fRoughness);

    // Modify diffuse by Fresnel reflection
    v3Diffuse *= (1.0f - v3F);

    // Combine diffuse and specular
    vec3 v3RetColour = v3Diffuse + (v3F * fD * fV);

    // Multiply by view angle
    v3RetColour *= max(dot(v3Normal, v3LightDirection), 0.0f);

    // Combine with incoming light value
    v3RetColour *= v3LightIrradiance;

    return v3RetColour;
}

void main()
{
    // Normalise the inputs
    vec3 v3Normal = normalize(v3NormalIn);
    vec3 v3ViewDirection = normalize(v3CameraPosition - v3PositionIn);

    // Loop over each point light
    vec3 v3RetColour = vec3(0.0f);
    for (int i = 0; i < iNumPointLights; i++) {
        vec3 v3LightDirection = normalize(PointLights[i].v3LightPosition - v3PositionIn);

        // Calculate light falloff
        vec3 v3LightIrradiance = lightFalloff(PointLights[i].v3LightIntensity, PointLights[i].v3Falloff, PointLights[i].v3LightPosition, v3PositionIn);

        // Perform shading
        v3RetColour += BRDFUniform(v3Normal, v3LightDirection, v3ViewDirection, v3LightIrradiance, v3DiffuseColour, v3SpecularColour, fRoughness);
    }

    // Add in ambient contribution
    v3RetColour += v3DiffuseColour * vec3(0.3f);
    v3ColourOut = v3RetColour;
}
#version 430 core

struct PointLight {
    vec3 v3LightPosition;
    vec3 v3LightIntensity;
    vec3 v3Falloff;
};
#define MAX_LIGHTS 16
layout(binding = 1) uniform CameraData {
    mat4 m4ViewProjection;
    vec3 v3CameraPosition;
};
layout(binding = 2) uniform PointLightData {
    PointLight PointLights[MAX_LIGHTS];
};
layout(binding = 3) uniform ReflectPlaneData {
    mat4 m4ReflectVP;
};

layout(location = 0) uniform int iNumPointLights;
layout(location = 1) uniform float fEmissivePower;

layout(binding = 0) uniform sampler2D s2DiffuseTexture;
layout(binding = 1) uniform sampler2D s2SpecularTexture;
layout(binding = 2) uniform sampler2D s2RoughnessTexture;
layout(binding = 3) uniform samplerCube scRefractMapTexture;
layout(binding = 4) uniform sampler2D s2ReflectTexture;
layout(binding = 5) uniform samplerCube scReflectMapTexture;

#define M_RCPPI 0.31830988618379067153776752674503f
#define M_PI 3.1415926535897932384626433832795f

layout(location = 0) in vec3 v3PositionIn;
layout(location = 1) in vec3 v3NormalIn;
layout(location = 2) in vec2 v2UVIn;
out vec3 v3ColourOut;

subroutine vec3 Emissive(vec3, vec3);
subroutine vec3 RefractMap(vec3, vec3, vec3, vec4, vec3);
subroutine vec3 ReflectMap(vec3, vec3, vec3, vec3, float);

layout(location = 0) subroutine uniform Emissive EmissiveUniform;
layout(location = 1) subroutine uniform RefractMap RefractMapUniform;
layout(location = 2) subroutine uniform ReflectMap ReflectMapUniform;

vec3 lightFalloff(in vec3 v3LightIntensity, in vec3 v3Falloff, in vec3 v3LightPosition, in vec3 v3Position)
{
    // Calculate distance from light
    float fDist = distance(v3LightPosition, v3Position);

    // Return falloff
    float fFalloff = v3Falloff.x + (v3Falloff.y * fDist) + (v3Falloff.z * fDist * fDist);
    return v3LightIntensity / fFalloff;
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

vec3 GGX(in vec3 v3Normal, in vec3 v3LightDirection, in vec3 v3ViewDirection, in vec3 v3LightIrradiance, in vec3 v3DiffuseColour, in vec3 v3SpecularColour, in float fRoughness)
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

vec3 SpecularTransmit(in vec3 v3Normal, in vec3 v3ViewDirection, in vec3 v3DiffuseColour, in vec3 v3SpecularColour)
{
    // Calculate index of refraction from Fresnel term
    float fRootF0 = sqrt(v3SpecularColour.x);
    float fIOR = (1.0f - fRootF0) / (1.0f + fRootF0);

    // Get refraction direction
    vec3 v3Refract = refract(-v3ViewDirection, v3Normal, fIOR);

    // Get refraction map data
    vec3 v3RefractColour = texture(scRefractMapTexture, v3Refract).rgb;

    // Evaluate specular transmittance
    vec3 v3RetColour = fIOR * (1.0f - schlickFresnel(v3Refract, -v3Normal, v3SpecularColour));
    v3RetColour *= v3DiffuseColour;

    // Combine with incoming light value
    v3RetColour *= v3RefractColour;

    return v3RetColour;
}

vec3 GGXReflect(in vec3 v3Normal, in vec3 v3ReflectDirection, in vec3 v3ViewDirection, in vec3 v3ReflectRadiance, in vec3 v3SpecularColour, in float fRoughness)
{
    // Calculate Toorance-Sparrow components
    vec3 v3F = schlickFresnel(v3ReflectDirection, v3Normal, v3SpecularColour);
    float fV = GGXVisibility(v3Normal, v3ReflectDirection, v3ViewDirection, fRoughness);

    // Combine specular
    vec3 v3RetColour = v3F * fV;

    // Modify by pdf
    v3RetColour *= (4.0f * dot(v3ViewDirection, v3Normal));

    // Multiply by view angle
    v3RetColour *= max(dot(v3Normal, v3ReflectDirection), 0.0f);

    // Combine with incoming light value
    v3RetColour *= v3ReflectRadiance;

    return v3RetColour;
}

layout(index = 0) subroutine(Emissive) vec3 noEmissive(vec3 v3ColourOut, vec3 v3DiffuseColour)
{
    // Return colour unmodified
    return v3ColourOut;
}

layout(index = 1) subroutine(Emissive) vec3 textureEmissive(vec3 v3ColourOut, vec3 v3DiffuseColour)
{
    // Add in emissive contribution
    return v3ColourOut + (fEmissivePower * v3DiffuseColour);
}

layout(index = 2) subroutine(RefractMap) vec3 noRefractMap(vec3 v3ColourOut, vec3 v3Normal, vec3 v3ViewDirection, vec4 v4DiffuseColour, vec3 v3SpecularColour)
{
    // Return colour unmodified
    return v3ColourOut;
}

layout(index = 3) subroutine(RefractMap) vec3 textureRefractMap(vec3 v3ColourOut, vec3 v3Normal, vec3 v3ViewDirection, vec4 v4DiffuseColour, vec3 v3SpecularColour)
{
    // Get specular transmittance term
    vec3 v3Transmit = SpecularTransmit(v3Normal, v3ViewDirection, v4DiffuseColour.rgb, v3SpecularColour);

    // Add in transparent contribution and blend with existing
    return mix(v3Transmit, v3ColourOut, v4DiffuseColour.w);
}

layout(index = 4) subroutine(ReflectMap) vec3 noReflectMap(vec3 v3ColourOut, vec3 v3Normal, vec3 v3ViewDirection, vec3 v3SpecularColour, float fRoughness)
{
    return v3ColourOut;
}

layout(index = 5) subroutine(ReflectMap) vec3 textureReflectPlane(vec3 v3ColourOut, vec3 v3Normal, vec3 v3ViewDirection, vec3 v3SpecularColour, float fRoughness)
{
    // Get position in reflection texture
    vec4 v4RVPPosition = m4ReflectVP * vec4(v3PositionIn, 1.0f);
    vec2 v2ReflectUV = v4RVPPosition.xy / v4RVPPosition.w;
    v2ReflectUV = (v2ReflectUV + 1.0f) * 0.5f;

    // Calculate LOD offset
    float fLOD = textureQueryLod(s2ReflectTexture, v2ReflectUV).y;
    float fGloss = 1.0f - fRoughness;
    fLOD += ((2.0f / (fGloss * fGloss)) - 1.0f);

    // Get reflect texture data
    vec3 v3ReflectRadiance = textureLod(s2ReflectTexture, v2ReflectUV, fLOD).rgb;

    // Get reflect direction
    vec3 v3ReflectDirection = normalize(reflect(-v3ViewDirection, v3Normal));

    // Perform shading
    vec3 v3RetColour = GGXReflect(v3Normal, v3ReflectDirection, v3ViewDirection, v3ReflectRadiance, v3SpecularColour, fRoughness);

    return v3ColourOut + v3RetColour;
}

layout(index = 6) subroutine(ReflectMap) vec3 textureReflectCube(vec3 v3ColourOut, vec3 v3Normal, vec3 v3ViewDirection, vec3 v3SpecularColour, float fRoughness)
{
    // Get reflect direction
    vec3 v3ReflectDirection = normalize(reflect(-v3ViewDirection, v3Normal));

    // Calculate LOD offset
    float fLOD = textureQueryLod(scReflectMapTexture, v3ReflectDirection).y;
    float fGloss = 1.0f - fRoughness;
    fLOD += ((2.0f / (fGloss * fGloss)) - 1.0f);

    // Get reflect texture data
    vec3 v3ReflectRadiance = textureLod(scReflectMapTexture, v3ReflectDirection, fLOD).xyz;

    // Perform shading
    vec3 v3RetColour = GGXReflect(v3Normal, v3ReflectDirection, v3ViewDirection, v3ReflectRadiance, v3SpecularColour, fRoughness);

    return v3ColourOut + v3RetColour;
}

void main()
{
    // Normalise the inputs
    vec3 v3Normal = normalize(v3NormalIn);
    vec3 v3ViewDirection = normalize(v3CameraPosition - v3PositionIn);

    // Get texture data
    vec4 v4DiffuseColour = texture(s2DiffuseTexture, v2UVIn);
    vec3 v3DiffuseColour = v4DiffuseColour.rgb;
    vec3 v3SpecularColour = texture(s2SpecularTexture, v2UVIn).rgb;
    float fRoughness = texture(s2RoughnessTexture, v2UVIn).r;

    // Loop over each point light
    vec3 v3RetColour = vec3(0.0f);
    for (int i = 0; i < iNumPointLights; i++) {
        vec3 v3LightDirection = normalize(PointLights[i].v3LightPosition - v3PositionIn);

        // Calculate light falloff
        vec3 v3LightIrradiance = lightFalloff(PointLights[i].v3LightIntensity, PointLights[i].v3Falloff, PointLights[i].v3LightPosition, v3PositionIn);

        // Perform shading
        v3RetColour += GGX(v3Normal, v3LightDirection, v3ViewDirection, v3LightIrradiance, v3DiffuseColour, v3SpecularColour, fRoughness);
    }

    // Add in ambient contribution
    v3RetColour += v3DiffuseColour * vec3(0.3f);

    // Add in any reflection contribution
    v3RetColour = ReflectMapUniform(v3RetColour, v3Normal, v3ViewDirection, v3SpecularColour, fRoughness);

    // Add in any refraction contribution
    v3RetColour = RefractMapUniform(v3RetColour, v3Normal, v3ViewDirection, v4DiffuseColour, v3SpecularColour);

    // Add in any emissive contribution
    v3RetColour = EmissiveUniform(v3RetColour, v3DiffuseColour);

    v3ColourOut = v3RetColour;
}
#version 430 core

struct PointLight {
    vec3 v3LightPosition;
    vec3 v3LightIntensity;
    vec3 v3Falloff;
    vec2 v2NearFar;
};
struct SpotLight {
    vec3 v3LightPosition;
    vec3 v3LightDirection;
    float fCosAngle;
    vec3 v3LightIntensity;
    vec3 v3Falloff;
};
#define MAX_LIGHTS 16
layout(std140, binding = 1) uniform CameraData {
    mat4 m4ViewProjection;
    vec3 v3CameraPosition;
    mat4 m4InvViewProjection;
};
layout(std140, binding = 2) uniform PointLightData {
    PointLight PointLights[MAX_LIGHTS];
};
layout(std140, binding = 5) uniform SpotLightData {
    SpotLight SpotLights[MAX_LIGHTS];
};
layout(binding = 6) uniform CameraShadowData {
    mat4 m4ViewProjectionShadow[MAX_LIGHTS];
};
layout(binding = 7) uniform InvResolution {
    vec2 v2InvResolution;
};

layout(location = 0) uniform int iNumPointLights;
layout(location = 2) uniform int iNumSpotLights;

layout(binding = 6) uniform sampler2DArrayShadow s2aShadowTexture;
layout(binding = 7) uniform samplerCubeArrayShadow scaPointShadowTexture;
layout(binding = 8) uniform sampler2DArray s2aTransparencyTexture;
layout(binding = 11) uniform sampler2D s2DepthTexture;
layout(binding = 12) uniform sampler2D s2NormalTexture;
layout(binding = 13) uniform sampler2D s2DiffuseTexture;
layout(binding = 14) uniform sampler2D s2SpecularRoughTexture;

#define M_RCPPI 0.31830988618379067153776752674503f
#define M_PI 3.1415926535897932384626433832795f

// Light perspective space size of lights area on near plane
const float fLightSize = 0.07f;

out vec3 v3AccumulationOut;


float random(in vec3 v3Seed, in float fFreq)
{
   // Project seed on random constant vector
   float fdt = dot(floor(v3Seed * fFreq), vec3(53.1215f, 21.1352f, 9.1322f));
   // Return only fractional part (range 0->1)
   return fract(sin(fdt) * 2105.2354f);
}

vec3 lightFalloff(in vec3 v3LightIntensity, in vec3 v3Falloff, in vec3 v3LightPosition, in vec3 v3Position)
{
    // Calculate distance from light
    float fDist = distance(v3LightPosition, v3Position);

    // Return falloff
    float fFalloff = v3Falloff.x + (v3Falloff.y * fDist) + (v3Falloff.z * fDist * fDist);
    return v3LightIntensity / fFalloff;
}

vec3 lightSpotShadow(in int iLight, in vec3 v3Position)
{
    // Get position in shadow texture
    vec4 v4SVPPosition = m4ViewProjectionShadow[iLight] * vec4(v3Position, 1.0f);
    vec3 v3SVPPosition = v4SVPPosition.xyz / v4SVPPosition.w;
    v3SVPPosition = (v3SVPPosition + 1.0f) * 0.5f;

    // Define Poisson disk sampling values
    const vec2 v2PoissonDisk[9] = vec2[](
        vec2(-0.01529481f, -0.07395129f),
        vec2(-0.56232890f, -0.36484920f),
        vec2( 0.95519960f,  0.18418130f),
        vec2( 0.20716880f,  0.49262790f),
        vec2(-0.01290792f, -0.95755550f),
        vec2( 0.68047200f, -0.51716110f),
        vec2(-0.60139470f,  0.37665210f),
        vec2(-0.40243310f,  0.86631060f),
        vec2(-0.96646290f, -0.04688413f));

    // Generate random rotation
    float fAngle = random(v3Position, 500.0f) * (M_PI * 2.0f);
    vec2 v2Rotate = vec2(sin(fAngle), cos(fAngle));

    // Approximate near plane size of light
    float fShadowRegion = fLightSize * v3SVPPosition.z;
    float fShadowSize = fShadowRegion / 9.0f;

    // Perform additional filtering
    float fShadowing = 0.0f;
    for (int i = 0 ; i <= 9 ; i++) {
        vec2 v2RotatedPoisson = (v2PoissonDisk[i].x * v2Rotate.yx) +
            (v2PoissonDisk[i].y * v2Rotate * vec2(-1.0f, 1.0f));
        vec2 v2Offset = v2RotatedPoisson * fShadowSize;
        vec3 v3UVC = v3SVPPosition + vec3(v2Offset, 0.0f);
        float fText = texture(s2aShadowTexture, vec4(v3UVC.xy, iLight, v3UVC.z));
        fShadowing += fText;
    }

    fShadowing /= 9.0f;

    // Get transparency information
    vec3 v3Trans = texture(s2aTransparencyTexture, vec3(v3SVPPosition.xy, iLight)).xyz;
    return v3Trans * fShadowing;
}

float lightPointShadow(in int iLight, in vec3 v3LightDirection, in vec3 v3LightDirectionUN, in vec2 v2NearFar, in vec3 v3Position)
{
    // Get depth in shadow texture
    vec3 v3AbsDirect = abs(v3LightDirectionUN);
    float fDist = max(v3AbsDirect.x, max(v3AbsDirect.y, v3AbsDirect.z));
    float fDepth = (v2NearFar.y + v2NearFar.x) * fDist;
    fDepth += (-2 * v2NearFar.y * v2NearFar.x);
    fDepth /= (v2NearFar.y - v2NearFar.x) * fDist;
    fDepth = (fDepth * 0.5) + 0.5;

    // Define Poisson sampling values
    const vec3 v3PoissonDisk[9] = vec3[](
        vec3(-0.023860920f, -0.115901396f,  0.985948205f),
        vec3(-0.649357200f, -0.542242587f,  0.066411376f),
        vec3( 0.956068397f,  0.285292149f, -0.865215898f),
        vec3( 0.228669465f,  0.698871136f,  0.355417848f),
        vec3(-0.001350721f, -0.997778296f, -0.866783142f),
        vec3( 0.602961421f, -0.725908756f, -0.338202178f),
        vec3(-0.672571659f,  0.557726085f, -0.027191758f),
        vec3(-0.123172671f,  0.978031158f, -0.663645744f),
        vec3(-0.995905936f, -0.073578961f, -0.894974828f));

    // Generate random rotation
    float fAngle = random(v3Position, 500.0f) * (M_PI * 2.0f);
    vec3 v3Rotate = vec3(sin(fAngle), cos(fAngle), 1.0f);

    // Approximate near plane size of light
    float fShadowRegion = fLightSize * fDepth;
    float fShadowSize = fShadowRegion / 9.0f;

    // Perform additional filtering
    float fShadowing = 0.0f;
    for (int i = 0 ; i <= 9 ; i++) {
        vec3 v3RotatedPoisson = (v3PoissonDisk[i].x * v3Rotate.yyz * v3Rotate.zxx) +
            (v3PoissonDisk[i].y * v3Rotate.xyx * v3Rotate.zyy * vec3(-1.0f, 1.0f, 1.0f) +
            (v3PoissonDisk[i].z * v3Rotate.zxy * vec3(0.0f, -1.0f, 1.0f)));
        vec3 v3Offset = v3RotatedPoisson * fShadowSize;
        vec3 v3UVC = -v3LightDirection + v3Offset;
        float fText = texture(scaPointShadowTexture, vec4(v3UVC, iLight), fDepth);
        fShadowing += fText;
    }

    return fShadowing / 9.0f;
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

void main() {
    // Get UV coordinates
    vec2 v2UV = gl_FragCoord.xy * v2InvResolution;

    // Get deferred data
    float fDepth = texture(s2DepthTexture, v2UV).r;
    vec3 v3Normal = vec3(texture(s2NormalTexture, v2UV).rg, 1.0f);
    v3Normal *= 2.0f / dot(v3Normal, v3Normal);
    v3Normal -= vec3(0.0f, 0.0f, 1.0f);
    vec3 v3DiffuseColour = texture(s2DiffuseTexture, v2UV).rgb;
    vec4 v4SpecularRough = texture(s2SpecularRoughTexture, v2UV);
    vec3 v3SpecularColour = v4SpecularRough.rgb;
    float fRoughness = v4SpecularRough.a;

    // Calculate position from depth
    fDepth = (fDepth * 2.0f) - 1.0f;
    vec2 v2NDCUV = (v2UV * 2.0f) - 1.0f;
    vec4 v4Position = m4InvViewProjection * vec4(v2NDCUV, fDepth, 1.0f);
    vec3 v3PositionIn = v4Position.xyz / v4Position.w;

    // Normalise the inputs
    vec3 v3ViewDirection = normalize(v3CameraPosition - v3PositionIn);

    // Loop over each point light
    vec3 v3RetColour = vec3(0.0f);
    for (int i = 0; i < iNumPointLights; i++) {
        vec3 v3LightDirectionUN = PointLights[i].v3LightPosition - v3PositionIn;
        vec3 v3LightDirection = normalize(v3LightDirectionUN);

        // Calculate light falloff
        vec3 v3LightIrradiance = lightFalloff(PointLights[i].v3LightIntensity, PointLights[i].v3Falloff, PointLights[i].v3LightPosition, v3PositionIn);

        // Calculate shadowing
        float fShadowing = lightPointShadow(i, v3LightDirection, v3LightDirectionUN, PointLights[i].v2NearFar, v3PositionIn);
        v3LightIrradiance *= fShadowing;

        // Perform shading
        v3RetColour += GGX(v3Normal, v3LightDirection, v3ViewDirection, v3LightIrradiance, v3DiffuseColour, v3SpecularColour, fRoughness);
    }

    // Loop over each spot light
    for (int i = 0; i < iNumSpotLights; i++) {
        vec3 v3LightDirection = normalize(SpotLights[i].v3LightPosition - v3PositionIn);

        // Check light angle
        float fLightAngle = dot(v3LightDirection, SpotLights[i].v3LightDirection);
        if (fLightAngle >= SpotLights[i].fCosAngle) {
            // Calculate light falloff
            vec3 v3LightIrradiance = lightFalloff(SpotLights[i].v3LightIntensity, SpotLights[i].v3Falloff, SpotLights[i].v3LightPosition, v3PositionIn);

            // Calculate shadowing
            vec3 v3Shadowing = lightSpotShadow(i, v3PositionIn);
            v3LightIrradiance *= v3Shadowing;

            // Perform shading
            v3RetColour += GGX(v3Normal, v3LightDirection, v3ViewDirection, v3LightIrradiance, v3DiffuseColour, v3SpecularColour, fRoughness);
        }
    }

    v3AccumulationOut = v3RetColour;
}
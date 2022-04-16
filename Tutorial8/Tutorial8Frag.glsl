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
layout(binding = 1) uniform CameraData {
    mat4 m4ViewProjection;
    vec3 v3CameraPosition;
};
layout(std140, binding = 2) uniform PointLightData {
    PointLight PointLights[MAX_LIGHTS];
};
layout(binding = 3) uniform ReflectPlaneData {
    mat4 m4ReflectVP;
};
layout(std140, binding = 5) uniform SpotLightData {
    SpotLight SpotLights[MAX_LIGHTS];
};
layout(binding = 6) uniform CameraShadowData {
    mat4 m4ViewProjectionShadow[MAX_LIGHTS];
};

layout(location = 0) uniform int iNumPointLights;
layout(location = 1) uniform float fEmissivePower;
layout(location = 2) uniform int iNumSpotLights;
layout(location = 3) uniform float fBumpScale;

layout(binding = 0) uniform sampler2D s2DiffuseTexture;
layout(binding = 1) uniform sampler2D s2SpecularTexture;
layout(binding = 2) uniform sampler2D s2RoughnessTexture;
layout(binding = 3) uniform samplerCube scRefractMapTexture;
layout(binding = 4) uniform sampler2D s2ReflectTexture;
layout(binding = 5) uniform samplerCube scReflectMapTexture;
layout(binding = 6) uniform sampler2DArrayShadow s2aShadowTexture;
layout(binding = 7) uniform samplerCubeArrayShadow scaPointShadowTexture;
layout(binding = 8) uniform sampler2DArray s2aTransparencyTexture;
layout(binding = 9) uniform sampler2D s2NormalTexture;
layout(binding = 10) uniform sampler2D s2BumpTexture;

#define M_RCPPI 0.31830988618379067153776752674503f
#define M_PI 3.1415926535897932384626433832795f

// Light perspective space size of lights area on near plane
const float fLightSize = 0.07f;

layout(location = 0) in vec3 v3PositionIn;
layout(location = 1) in vec3 v3NormalIn;
layout(location = 2) in vec2 v2UVIn;
layout(location = 3) in vec3 v3TangentIn;
out vec3 v3ColourOut;

subroutine vec3 Emissive(vec3, vec3);
subroutine vec3 RefractMap(vec3, vec3, vec3, vec4, vec3);
subroutine vec3 ReflectMap(vec3, vec3, vec3, vec3, float);

layout(location = 0) subroutine uniform Emissive EmissiveUniform;
layout(location = 1) subroutine uniform RefractMap RefractMapUniform;
layout(location = 2) subroutine uniform ReflectMap ReflectMapUniform;

vec3 normalMap(in vec3 v3Normal, in vec3 v3Tangent, in vec3 v3BiTangent, in vec2 v2LocalUV)
{
    // Get normal map value
    vec2 v2NormalMap = (texture(s2NormalTexture, v2LocalUV).rg - 0.5f) * 2.0f;
    vec3 v3NormalMap = vec3(v2NormalMap, sqrt(1.0f - dot(v2NormalMap, v2NormalMap)));

    // Convert from tangent space
    vec3 v3RetNormal = mat3(v3Tangent,
        v3BiTangent,
        v3Normal) * v3NormalMap;
    return normalize(v3RetNormal);
}

vec2 parallaxMap(in vec3 v3Normal, in vec3 v3Tangent, in vec3 v3BiTangent, in vec3 v3ViewDirection)
{
    // Get tangent space view direction
    vec3 v3TangentView = vec3(dot(v3ViewDirection, v3Tangent),
         dot(v3ViewDirection, v3BiTangent),
         dot(v3ViewDirection, v3Normal));
    v3TangentView = normalize(v3TangentView);

    // Get number of layers based on view direction
    const float fMinLayers = 5.0f;
    const float fMaxLayers = 15.0f;
    float fNumLayers = round(mix(fMaxLayers, fMinLayers, abs(v3TangentView.z)));

    // Determine layer height
    float fLayerHeight = 1.0f / fNumLayers;
    // Determine texture offset per layer
    vec2 v2DTex = fBumpScale * v3TangentView.xy / (v3TangentView.z * fNumLayers);

    // Get texture gradients to allow for dynamic branching
    vec2 v2Dx = dFdx(v2UVIn);
    vec2 v2Dy = dFdy(v2UVIn);

    // Initialise height from texture
    vec2 v2CurrUV = v2UVIn;
    float fCurrHeight = textureGrad(s2BumpTexture, v2CurrUV, v2Dx, v2Dy).r;

    // Loop over each step until lower height is found
    float fViewHeight = 1.0f;
    float fLastHeight = 1.0f;
    vec2 v2LastUV;
    for (int i = 0; i < int(fNumLayers); i++) {
        if(fCurrHeight >= fViewHeight)
            break;
        // Set current values as previous
        fLastHeight = fCurrHeight;
        v2LastUV = v2CurrUV;
        // Go to next layer
        fViewHeight -= fLayerHeight;
        // Shift UV coordinates
        v2CurrUV -= v2DTex;
        // Get new texture height
        fCurrHeight = textureGrad(s2BumpTexture, v2CurrUV, v2Dx, v2Dy).r;
    }

    // Get heights for linear interpolation
    float fNextHeight = fCurrHeight - fViewHeight;
    float fPrevHeight = fLastHeight - (fViewHeight + fLayerHeight);

    // Interpolate based on height difference
    float fWeight = fNextHeight / (fNextHeight - fPrevHeight);
    return mix(v2CurrUV, v2LastUV, fWeight);
}

float parallaxMapShadow(in vec3 v3Normal, in vec3 v3Tangent, in vec3 v3BiTangent, in vec3 v3TLightDirection, in vec2 v2UVPO)
{
    // Get tangent space light direction
    vec3 v3TangentLight = vec3(dot(v3TLightDirection, v3Tangent),
         dot(v3TLightDirection, v3BiTangent),
         dot(v3TLightDirection, v3Normal));
    v3TangentLight = normalize(v3TangentLight);

    float fShadowing = 0.0f;

    // Check if surface is oriented toward light
    if (v3TangentLight.z > 0.0f) {
        // Get number of layers based on view direction
        const float fMinLayers = 10.0f;
        const float fMaxLayers = 20.0f;
        float fNumLayers = round(mix(fMaxLayers, fMinLayers, abs(v3TangentLight.z)));

        // Determine layer height
        float fHeight = texture(s2BumpTexture, v2UVPO).r - 0.0005f;
        float fLayerHeight = (1.0f - fHeight) / fNumLayers;
        // Determine texture offset per layer
        vec2 v2DTex = fBumpScale * v3TangentLight.xy / (v3TangentLight.z * fNumLayers);

        // Initialise values
        float fViewHeight = fHeight + fLayerHeight;
        vec2 v2CurrUV = v2UVPO + v2DTex;

        // Get texture gradients to allow for dynamic branching
        vec2 v2Dx = dFdx(v2CurrUV);
        vec2 v2Dy = dFdy(v2CurrUV);

        // Initialise height from texture
        float fCurrHeight = textureGrad(s2BumpTexture, v2CurrUV, v2Dx, v2Dy).r;

        // Loop over each step
        for (int i = 1; i <= int(fNumLayers); i++) {
            // Check if point is under surface
            if (fViewHeight < fCurrHeight) {
                // Get negative shadowing
                float fNewShadowing = (fCurrHeight - fViewHeight) * (1.0f - (float(i) / fNumLayers));
                fNewShadowing = fLayerHeight / fNewShadowing;
                fShadowing = max(fShadowing, fNewShadowing);
            }

            // Go to next layer
            fViewHeight += fLayerHeight;
            // Shift UV coordinates
            v2CurrUV += v2DTex;
            // Get new texture height
            fCurrHeight = textureGrad(s2BumpTexture, v2CurrUV, v2Dx, v2Dy).r;
        }
        // Clamp shadowing to 0-1
        fShadowing = 1.0f - min(fShadowing, 1.0f);
    }
    return fShadowing;
}

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

    // Add in any backside lighting

    // Loop over each spot light
    for (int i=0; i<iNumSpotLights; i++) {
        vec3 v3LightDirection = normalize(SpotLights[i].v3LightPosition - v3PositionIn);

        // Check light angle
        float fLightAngle = dot(v3LightDirection, SpotLights[i].v3LightDirection);
        if (fLightAngle >= SpotLights[i].fCosAngle) {
            // Calculate light falloff
            vec3 v3LightIrradiance = lightFalloff(SpotLights[i].v3LightIntensity, SpotLights[i].v3Falloff, SpotLights[i].v3LightPosition, v3PositionIn);

            // Perform shading
            float fAngle = dot(v3LightDirection, -v3ViewDirection);
            fAngle = (fAngle > 0.99f)? (fAngle - 0.99f) * 100.0f : 0.0f;
            v3Transmit += v4DiffuseColour.xyz * v3LightIrradiance * fAngle;
        }
    }

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

void main() {
    // Normalise the inputs
    vec3 v3Normal = normalize(v3NormalIn);
    vec3 v3ViewDirection = normalize(v3CameraPosition - v3PositionIn);
    vec3 v3Tangent = normalize(v3TangentIn);

    // Generate bitangent
    vec3 v3BiTangent = cross(v3Normal, v3Tangent);

    // Perform Parallax Occlusion Mapping
    vec2 v2UVPO = parallaxMap(v3Normal, v3Tangent, v3BiTangent, v3ViewDirection);

    // Perform Bump Mapping
    //v3Normal = normalMap(v3Normal, v3Tangent, v3BiTangent, v2UVIn);
    //vec3 v2UVPO = v2UVIn;
    v3Normal = normalMap(v3Normal, v3Tangent, v3BiTangent, v2UVPO);

    // Get texture data
    vec4 v4DiffuseColour = texture(s2DiffuseTexture, v2UVPO);
    vec3 v3DiffuseColour = v4DiffuseColour.rgb;
    vec3 v3SpecularColour = texture(s2SpecularTexture, v2UVPO).rgb;
    float fRoughness = texture(s2RoughnessTexture, v2UVPO).r;

    // Loop over each point light
    vec3 v3RetColour = vec3(0.0f);
    for (int i = 0; i < iNumPointLights; i++) {
        vec3 v3LightDirectionUN = PointLights[i].v3LightPosition - v3PositionIn;
        vec3 v3LightDirection = normalize(v3LightDirectionUN);

        // Calculate light falloff
        vec3 v3LightIrradiance = lightFalloff(PointLights[i].v3LightIntensity, PointLights[i].v3Falloff, PointLights[i].v3LightPosition, v3PositionIn);

        // Calculate shadowing
        float fShadowing = lightPointShadow(i, v3LightDirection, v3LightDirectionUN, PointLights[i].v2NearFar, v3PositionIn);
        //fShadowing *= parallaxMapShadow(v3Normal, v3Tangent, v3BiTangent, v3LightDirection, v2UVPO);
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
            //v3Shadowing *= parallaxMapShadow(v3Normal, v3Tangent, v3BiTangent, v3LightDirection, v2UVPO);
            v3LightIrradiance *= v3Shadowing;

            // Perform shading
            v3RetColour += GGX(v3Normal, v3LightDirection, v3ViewDirection, v3LightIrradiance, v3DiffuseColour, v3SpecularColour, fRoughness);
        }
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
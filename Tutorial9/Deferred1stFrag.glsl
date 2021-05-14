#version 430 core

layout(binding = 1) uniform CameraData {
    mat4 m4ViewProjection;
    vec3 v3CameraPosition;
};
layout(binding = 3) uniform ReflectPlaneData {
    mat4 m4ReflectVP;
};

layout(location = 1) uniform float fEmissivePower;
layout(location = 3) uniform float fBumpScale;

layout(binding = 0) uniform sampler2D s2DiffuseTexture;
layout(binding = 1) uniform sampler2D s2SpecularTexture;
layout(binding = 2) uniform sampler2D s2RoughnessTexture;
layout(binding = 3) uniform samplerCube scRefractMapTexture;
layout(binding = 4) uniform sampler2D s2ReflectTexture;
layout(binding = 5) uniform samplerCube scReflectMapTexture;
layout(binding = 9) uniform sampler2D s2NormalTexture;
layout(binding = 10) uniform sampler2D s2BumpTexture;

#define M_RCPPI 0.31830988618379067153776752674503f
#define M_PI 3.1415926535897932384626433832795f

layout(location = 0) in vec3 v3PositionIn;
layout(location = 1) in vec3 v3NormalIn;
layout(location = 2) in vec2 v2UVIn;
layout(location = 3) in vec3 v3TangentIn;

layout(location = 0) out vec3 v3AccumulationOut;
layout(location = 1) out vec2 v2NormalOut;
layout(location = 2) out vec3 v3DiffuseOut;
layout(location = 3) out vec4 v4SpecularRoughOut;

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
        if (fCurrHeight >= fViewHeight)
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

vec3 schlickFresnel(in vec3 v3LightDirection, in vec3 v3Normal, in vec3 v3SpecularColour)
{
    // Schlick Fresnel approximation
    float fLH = dot(v3LightDirection, v3Normal);
    return v3SpecularColour + (1.0f - v3SpecularColour) * pow(1.0f - fLH, 5);
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
    v3Normal = normalMap(v3Normal, v3Tangent, v3BiTangent, v2UVPO);

    // Get texture data
    vec4 v4DiffuseColour = texture(s2DiffuseTexture, v2UVPO);
    vec3 v3DiffuseColour = v4DiffuseColour.rgb;
    vec3 v3SpecularColour = texture(s2SpecularTexture, v2UVPO).rgb;
    float fRoughness = texture(s2RoughnessTexture, v2UVPO).r;

    // Add in ambient contribution
    vec3 v3RetColour = v3DiffuseColour * vec3(0.3f);

    // Add in any reflection contribution
    v3RetColour = ReflectMapUniform(v3RetColour, v3Normal, v3ViewDirection, v3SpecularColour, fRoughness);

    // Add in any refraction contribution
    v3RetColour = RefractMapUniform(v3RetColour, v3Normal, v3ViewDirection, v4DiffuseColour, v3SpecularColour);

    // Add in any emissive contribution
    v3RetColour = EmissiveUniform(v3RetColour, v3DiffuseColour);

    // Output to deferred G-Buffers
    v3AccumulationOut = v3RetColour;
    v2NormalOut = v3Normal.xy / (1.0f + v3Normal.z);
    v3DiffuseOut = v3DiffuseColour;
    v4SpecularRoughOut = vec4(v3SpecularColour, fRoughness);
}
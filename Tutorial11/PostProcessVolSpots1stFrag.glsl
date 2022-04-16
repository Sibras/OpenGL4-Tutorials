#version 430 core

struct SpotLight {
    vec3 v3LightPosition;
    vec3 v3LightDirection;
    float fCosAngle;
    vec3 v3LightIntensity;
    vec3 v3Falloff;
    float fFalloffDist;
};
#define MAX_LIGHTS 16
layout(std140, binding = 1) uniform CameraData {
    mat4 m4ViewProjection;
    vec3 v3CameraPosition;
    mat4 m4InvViewProjection;
};
layout(std140) layout(binding = 5) uniform SpotLightData {
    SpotLight SpotLights[MAX_LIGHTS];
};
layout(binding = 6) uniform CameraShadowData {
    mat4 m4ViewProjectionShadow[MAX_LIGHTS];
};
layout(binding = 7) uniform InvResolution {
    vec2 v2InvResolution;
};

layout(location = 2) uniform int iNumSpotLights;

layout(binding = 6) uniform sampler2DArrayShadow s2aShadowTexture;
layout(binding = 8) uniform sampler2DArray s2aTransparencyTexture;
layout(binding = 11) uniform sampler2D s2DepthTexture;

#define M_RCP4PI 0.07957747154594766788444188168626f

out vec3 v3VolumeOut;

// Volumetric light parameters
const float fStepSize = 0.05f;
float fPhaseK = -0.2f;
float fScatterring = 0.5f;

vec3 lightFalloff(in vec3 v3LightIntensity, in vec3 v3Falloff, in vec3 v3LightPosition, in vec3 v3Position)
{
    // Calculate distance from light
    float fDist = distance(v3LightPosition, v3Position);

    // Return falloff
    float fFalloff = v3Falloff.x + (v3Falloff.y * fDist) + (v3Falloff.z * fDist * fDist);
    return v3LightIntensity / fFalloff;
}

vec3 volumeSpotLight(in int iLight, in vec3 v3Position)
{
    // Get ray from the camera to first object intersection
    vec3 v3RayDirection = v3Position - v3CameraPosition;
    float fRayLength = length(v3RayDirection);
    v3RayDirection /= fRayLength;

    // Initialise ray parameters
    vec2 v2TRange = vec2(0.0f, fRayLength);

    // Calculate quadratic coefficients
    vec3 v3LigthDirection = -SpotLights[iLight].v3LightDirection;
    vec3 v3FromLightDir = v3CameraPosition - SpotLights[iLight].v3LightPosition;
    float fDotLDRD = dot(v3LigthDirection, v3RayDirection);
    float fDotLDTLD = dot(v3LigthDirection, v3FromLightDir);
    float fDotRDTLD = dot(v3RayDirection, v3FromLightDir);
    float fDotTLDTLD = dot(v3FromLightDir, v3FromLightDir);
    float fCosSq = SpotLights[iLight].fCosAngle * SpotLights[iLight].fCosAngle;
    float fA = (fDotLDRD * fDotLDRD) - fCosSq;
    float fB = (fDotLDRD * fDotLDTLD) - (fCosSq * fDotRDTLD);
    float fC = (fDotLDTLD * fDotLDTLD) - (fCosSq * fDotTLDTLD);

    // Calculate quadratic intersection points
    float fVolumeSize = 0.0f;
    float fRoot = (fB * fB) - (fA * fC);
    if (fA != 0.0f && fRoot > 0.0f) {
        fRoot = sqrt(fRoot);
        float fInvA = 1.0f / fA;

        // Check for any intersections against the positive cone
        vec2 v2ConeInt = (vec2(-fB) + vec2(-fRoot, fRoot)) * fInvA;
        bvec2 b2Comp = greaterThanEqual((fDotLDRD * v2ConeInt) + fDotLDTLD, vec2(0.0f));
        if (any(b2Comp)) {
            // Clip out negative cone intersections
            vec2 v2MinMax = (fDotLDRD >= 0.0)? vec2(2e20) : vec2(-2e20);
            v2ConeInt.x = (b2Comp.x)? v2ConeInt.x : v2MinMax.x;
            v2ConeInt.y = (b2Comp.y)? v2ConeInt.y : v2MinMax.y;

            // Sort cone intersections
            float fConeIntBack = v2ConeInt.x;
            v2ConeInt.x = min(v2ConeInt.x, v2ConeInt.y);
            v2ConeInt.y = max(fConeIntBack, v2ConeInt.y);

            // Clip to cone height
            vec2 v2ConeRange = (vec2(0.0f, SpotLights[iLight].fFalloffDist) - fDotLDTLD) / fDotLDRD;
            v2ConeRange = (fDotLDRD > 0.0f)? v2ConeRange : v2ConeRange.yx;
            v2ConeInt.x = max(v2ConeInt.x, v2ConeRange.x);
            v2ConeInt.y = min(v2ConeInt.y, v2ConeRange.y);

            // Clip to ray length
            v2TRange.x = max(v2TRange.x, v2ConeInt.x);
            v2TRange.y = min(v2TRange.y, v2ConeInt.y);

            fVolumeSize = v2TRange.y - v2TRange.x;
        }
    }

    vec3 v3RetColour = vec3(0.0f);
    if (fVolumeSize > 0.0f) {
        //Calculate number of steps through light volume
        int iNumSteps = int(max((fVolumeSize / fStepSize), 8.0f));
        float fUsedStepSize = fVolumeSize / float(iNumSteps);

        //*********TODO: calculate lightfalloff at start and end only and interpolate inbetween !optimize!

        // Define jittered sampling offsets
        const float fSampleOffsets[16] = float[](
            0.442049302f, 0.228878706f, 0.849435568f, 0.192974103f,
            0.001110852f, 0.137045622f, 0.778863043f, 0.989015579f,
            0.463210519f, 0.642646075f, 0.067392051f, 0.330898911f,
            0.533205688f, 0.677708924f, 0.066608429f, 0.486404121f
           );

        // Get 4x4 sample
        ivec2 i2SamplePos = ivec2(gl_FragCoord.xy) % 4;
        float fOffset = fSampleOffsets[ (i2SamplePos.y * 4) + i2SamplePos.x ];
        fOffset *= fUsedStepSize;

        // Loop through each step and check shadow map
        vec3 v3CurrPosition = v3CameraPosition + ((v2TRange.x + fOffset) * v3RayDirection);
        for (int i = 0; i < iNumSteps; i++) {
            // Get position in shadow texture
            vec4 v4SVPPosition = m4ViewProjectionShadow[iLight] * vec4(v3CurrPosition, 1.0);
            vec3 v3SVPPosition = v4SVPPosition.xyz / v4SVPPosition.w;
            v3SVPPosition = (v3SVPPosition + 1.0f) * 0.5f;

            // Get texture value
            float fText = texture(s2aShadowTexture, vec4(v3SVPPosition.xy, iLight, v3SVPPosition.z));

            // Calculate phase function
            float fPhase = M_RCP4PI * (1.0f - (fPhaseK * fPhaseK));
            vec3 v3LightToCurr = normalize(v3CurrPosition - SpotLights[iLight].v3LightPosition);
            float fDotLTCRD = dot(v3LightToCurr, v3RayDirection);
            float fPhaseDenom = 1.0f - (fPhaseK * fDotLTCRD);
            fPhase /= fPhaseDenom * fPhaseDenom;

            // Calculate total in-scattering
            vec3 v3LightVolume = lightFalloff(SpotLights[iLight].v3LightIntensity, SpotLights[iLight].v3Falloff, SpotLights[iLight].v3LightPosition, v3CurrPosition);
            v3LightVolume *= fPhase * fText;

            // Get transparency information
            vec3 v3Trans = texture(s2aTransparencyTexture, vec3(v3SVPPosition.xy, iLight)).rgb;
            v3LightVolume *= v3Trans;

            // Add to current lighting
            v3RetColour += v3LightVolume;

            // Increment position
            v3CurrPosition += fUsedStepSize * v3RayDirection;
        }
        v3RetColour *= fScatterring * fUsedStepSize;
    }
    return v3RetColour;
}

void main() {
    // Get UV coordinates
    vec2 v2UV = gl_FragCoord.xy * v2InvResolution;

    // Calculate position from depth
    float fDepth = texture(s2DepthTexture, v2UV).r;
    fDepth = (fDepth * 2.0f) - 1.0f;
    vec2 v2NDCUV = (v2UV * 2.0f) - 1.0f;
    vec4 v4Position = m4InvViewProjection * vec4(v2NDCUV, fDepth, 1.0f);
    vec3 v3PositionIn = v4Position.xyz / v4Position.w;

    // Loop over each spot light
    vec3 v3RetColour = vec3(0.0f);
    for (int i = 0; i < iNumSpotLights; i++) {
        // Calculate volume scattering
        v3RetColour += volumeSpotLight(i, v3PositionIn);
    }

    v3VolumeOut = v3RetColour;
}
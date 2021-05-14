#version 430 core

layout(binding = 1) uniform CameraData {
    mat4 m4ViewProjection;
    vec3 v3CameraPosition;
};

layout(triangles, fractional_odd_spacing, ccw) in;

layout(location = 0)  in vec3 v3VertexPos[]; //030, 003, 300
layout(location = 3)  in vec3 v3VertexNormal[];
layout(location = 6)  in vec2 v2VertexUV[];
layout(location = 9)  in vec3 v3VertexTangent[];
layout(location = 12) in vec3 v3PatchE1[];   //021, 102, 210
layout(location = 15) in vec3 v3PatchE2[];   //012, 201, 120
layout(location = 18) in patch vec3 v3P111;

layout(location = 0) smooth out vec3 v3PositionOut;
layout(location = 1) smooth out vec3 v3NormalOut;
layout(location = 2) smooth out vec2 v2UVOut;
layout(location = 3) smooth out vec3 v3TangentOut;

void main()
{
    // Get Bezier patch values
    vec3 v3P030 = v3VertexPos[0];
    vec3 v3P021 = v3PatchE1[0];
    vec3 v3P012 = v3PatchE2[0];
    vec3 v3P003 = v3VertexPos[1];
    vec3 v3P102 = v3PatchE1[1];
    vec3 v3P201 = v3PatchE2[1];
    vec3 v3P300 = v3VertexPos[2];
    vec3 v3P210 = v3PatchE1[2];
    vec3 v3P120 = v3PatchE2[2];

    // Get Tesselation values
    float fU = gl_TessCoord.x;
    float fV = gl_TessCoord.y;
    float fW = gl_TessCoord.z;
    float fUU = fU * fU;
    float fVV = fV * fV;
    float fWW = fW * fW;
    float fUU3 = fUU * 3.0f;
    float fVV3 = fVV * 3.0f;
    float fWW3 = fWW * 3.0f;

    // Calculate new position
    vec3 v3PNPoint = v3P030 * fUU * fU +
        v3P003 * fVV * fV +
        v3P300 * fWW * fW +
        v3P021 * fUU3 * fV +
        v3P012 * fVV3 * fU +
        v3P102 * fVV3 * fW +
        v3P201 * fWW3 * fV +
        v3P210 * fWW3 * fU +
        v3P120 * fUU3 * fW +
        v3P111 * 6.0f * fW * fU * fV;

    // Calculate basic interpolated position
    vec3 v3BasePosition = (v3P030 * gl_TessCoord.x) +
     (v3P003 * gl_TessCoord.y) +
     (v3P300 * gl_TessCoord.z);

    // Determine influence on point
    const float fInfluence = 0.75f;
    v3PositionOut = mix(v3BasePosition, v3PNPoint, fInfluence);

    // Interpolate normal and UV
    v3NormalOut = (v3VertexNormal[0] * gl_TessCoord.x) +
     (v3VertexNormal[1] * gl_TessCoord.y) +
     (v3VertexNormal[2] * gl_TessCoord.z);
    v2UVOut = (v2VertexUV[0] * gl_TessCoord.x) +
     (v2VertexUV[1] * gl_TessCoord.y) +
     (v2VertexUV[2] * gl_TessCoord.z);
    v3TangentOut = (v3VertexTangent[0] * gl_TessCoord.x) +
     (v3VertexTangent[1] * gl_TessCoord.y) +
     (v3VertexTangent[2] * gl_TessCoord.z);

    // Update clip space position;
    gl_Position = m4ViewProjection * vec4(v3PositionOut, 1.0f);
}
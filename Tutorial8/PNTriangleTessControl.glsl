#version 430 core

layout(vertices = 3) out;

layout(binding = 1) uniform CameraData {
    mat4 m4ViewProjection;
    vec3 v3CameraPosition;
};

layout(location = 4) uniform vec2 v2Resolution;

// Inputs from vertex shader
layout(location = 0) in vec3 v3VertexPos[];
layout(location = 1) in vec3 v3VertexNormal[];
layout(location = 2) in vec2 v2VertexUV[];
layout(location = 3) in vec3 v3VertexTangent[];

// Passed through outputs
layout(location = 0)  out vec3 v3PositionOut[3]; //030, 003, 300
layout(location = 3)  out vec3 v3NormalOut[3];
layout(location = 6)  out vec2 v2UVOut[3];
layout(location = 9)  out vec3 v3TangentOut[3];
// PN Triangle additional data
layout(location = 12) out vec3 v3PatchE1[3];  //021, 102, 210
layout(location = 15) out vec3 v3PatchE2[3];  //012, 201, 120
layout(location = 18) out patch vec3 v3P111;

vec3 projectToPlane(in vec3 v3Point, in vec3 v3PlanePoint, in vec3 v3PlaneNormal)
{
    // Project point to plane
    float fD = dot(v3Point - v3PlanePoint, v3PlaneNormal);
    vec3 v3D = fD * v3PlaneNormal;
    return v3Point - v3D;
}

float tessLevel(in vec3 v3Point1, in vec3 v3Point2)
{
    const float fPixelsPerEdge = 4.0f;
    vec2 v2P1 = (v3Point1.xy + v3Point2.xy) * 0.5f;
    vec2 v2P2 = v2P1;
    v2P2.y += distance(v3Point1, v3Point2);
    float fLength = length((v2P1 - v2P2) * v2Resolution * 0.5f);
    return clamp(fLength / fPixelsPerEdge, 1.0f, 32.0f);
}

void main()
{
    // Pass through the control points of the patch
    v3PositionOut[gl_InvocationID] = v3VertexPos[gl_InvocationID];
    v3NormalOut[gl_InvocationID] = v3VertexNormal[gl_InvocationID];
    v2UVOut[gl_InvocationID] = v2VertexUV[gl_InvocationID];
    v3TangentOut[gl_InvocationID] = v3VertexTangent[gl_InvocationID];

    // Calculate Bezier patch control points
    const int iNextInvocID = gl_InvocationID < 2 ? gl_InvocationID + 1 : 0;
    vec3 v3CurrPos = v3VertexPos[gl_InvocationID];
    vec3 v3NextPos = v3VertexPos[iNextInvocID];
    vec3 v3CurrNormal = normalize(v3VertexNormal[gl_InvocationID]); //normalize should be done in vertex shader instead
    vec3 v3NextNormal = normalize(v3VertexNormal[iNextInvocID]);

    // Project onto vertex normal plane
    vec3 v3ProjPoint1 = projectToPlane(v3NextPos, v3CurrPos, v3CurrNormal);
    vec3 v3ProjPoint2 = projectToPlane(v3CurrPos, v3NextPos, v3NextNormal);

    // Calculate Bezier CP at 1/3 length
    v3PatchE1[gl_InvocationID] = ((2.0f * v3CurrPos) + v3ProjPoint1) / 3.0f;
    v3PatchE2[gl_InvocationID] = ((2.0f * v3NextPos) + v3ProjPoint2) / 3.0f;

    barrier();
    if (gl_InvocationID == 0) {
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

        // Calculate centre point
        vec3 v3E = (v3P021 + v3P012 + v3P102 + v3P201 + v3P210 + v3P120) / 6.0f;
        vec3 v3V = (v3P300 + v3P003 + v3P030) / 3.0f;
        v3P111 = v3E + ((v3E - v3V) / 2.0f);

        // Calculate the tessellation levels
        //gl_TessLevelOuter[0] = tessLevel(gl_in[1].gl_Position.xyz, gl_in[2].gl_Position.xyz);
        //gl_TessLevelOuter[1] = tessLevel(gl_in[2].gl_Position.xyz, gl_in[0].gl_Position.xyz);
        //gl_TessLevelOuter[2] = tessLevel(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz);
        //gl_TessLevelInner[0] = max(gl_TessLevelOuter[0], max(gl_TessLevelOuter[1], gl_TessLevelOuter[2]));

        int iOutOfBound[6] = int[6](0, 0, 0, 0, 0, 0);
        // Loop over each vertex and get clip space position
        for (int i = 0; i < 3; ++i) {
            // Check if any value is outside clip planes
            if (gl_in[i].gl_Position.x >  gl_in[i].gl_Position.w)
                iOutOfBound[0] = iOutOfBound[0] + 1;
            if (gl_in[i].gl_Position.x < -gl_in[i].gl_Position.w)
                iOutOfBound[1] = iOutOfBound[1] + 1;
            if (gl_in[i].gl_Position.y >  gl_in[i].gl_Position.w)
                iOutOfBound[2] = iOutOfBound[2] + 1;
            if (gl_in[i].gl_Position.y < -gl_in[i].gl_Position.w)
                iOutOfBound[3] = iOutOfBound[3] + 1;
            if (gl_in[i].gl_Position.z >  gl_in[i].gl_Position.w)
                iOutOfBound[4] = iOutOfBound[4] + 1;
            if (gl_in[i].gl_Position.z < -gl_in[i].gl_Position.w)
                iOutOfBound[5] = iOutOfBound[5] + 1;
        }

        // Loop over each clip face and check if triangle is completely outside
        bool bInFrustum = true;
        for (int i = 0; i < 6; ++i)
            if (iOutOfBound[i] == 3)
                bInFrustum = false;

        // Check front face culling
        vec3 v3Normal = cross(v3VertexPos[2] - v3VertexPos[0],
            v3VertexPos[0] - v3VertexPos[1]);
        vec3 v3ViewDirection = v3CameraPosition - v3VertexPos[0];

        // If visible output triangle data
        if (bInFrustum && (dot(v3Normal, v3ViewDirection) > 0.0f)) {
            // Check if object is not flat
            if ((dot(v3VertexNormal[0], v3VertexNormal[1]) * dot(v3VertexNormal[0], v3VertexNormal[2])) < 0.999) {
                vec3 v3Pos0 =  gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;
                vec3 v3Pos1 =  gl_in[1].gl_Position.xyz / gl_in[1].gl_Position.w;
                vec3 v3Pos2 =  gl_in[2].gl_Position.xyz / gl_in[2].gl_Position.w;
                gl_TessLevelOuter[0] = tessLevel(v3Pos1, v3Pos2);
                gl_TessLevelOuter[1] = tessLevel(v3Pos2, v3Pos0);
                gl_TessLevelOuter[2] = tessLevel(v3Pos0, v3Pos1);
                gl_TessLevelInner[0] = max(gl_TessLevelOuter[0], max(gl_TessLevelOuter[1], gl_TessLevelOuter[2]));
            } else {
                 // Don’t bother tessellating flat surfaces
                 gl_TessLevelOuter = float[](1.0f, 1.0f, 1.0f, 0.0f);
                 gl_TessLevelInner = float[](1.0f, 0.0f);
            }
        } else {
            // Discard patches by setting tessellation levels to zero
            gl_TessLevelOuter = float[](0.0f, 0.0f, 0.0f, 0.0f);
            gl_TessLevelInner = float[](0.0f, 0.0f);
        }
    }
}
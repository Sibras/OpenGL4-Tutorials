#version 430 core

#define MAX_LIGHTS 32
layout(binding = 6) uniform CameraShadowData {
    mat4 m4ViewProjectionShadow[MAX_LIGHTS];
};
layout(std140, binding = 9) uniform CameraShadowData2 {
    vec3 v3PositionShadow[MAX_LIGHTS];
};

layout(location = 0) uniform int iNumLights;

layout(triangles, invocations = MAX_LIGHTS) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 v3VertexPos[];
layout(location = 1) in vec2 v2VertexUV[];

layout(location = 0) smooth out vec2 v2UVOut;

void main()
{
    // Check if valid invocation
    if (gl_InvocationID < iNumLights) {
        vec4 v4PositionVPTemp[3];
        int iOutOfBound[6] = int[6](0, 0, 0, 0, 0, 0);
        // Loop over each vertex and get clip space position
        for (int i = 0; i < 3; ++i) {
            // Transform position
            v4PositionVPTemp[i] = m4ViewProjectionShadow[gl_InvocationID] * vec4(v3VertexPos[i], 1.0f);

            // Check if any value is outside clip planes
            if (v4PositionVPTemp[i].x >  v4PositionVPTemp[i].w)
                iOutOfBound[0] = iOutOfBound[0] + 1;
            if (v4PositionVPTemp[i].x < -v4PositionVPTemp[i].w)
                iOutOfBound[1] = iOutOfBound[1] + 1;
            if (v4PositionVPTemp[i].y >  v4PositionVPTemp[i].w)
                iOutOfBound[2] = iOutOfBound[2] + 1;
            if (v4PositionVPTemp[i].y < -v4PositionVPTemp[i].w)
                iOutOfBound[3] = iOutOfBound[3] + 1;
            if (v4PositionVPTemp[i].z >  v4PositionVPTemp[i].w)
                iOutOfBound[4] = iOutOfBound[4] + 1;
            if (v4PositionVPTemp[i].z < -v4PositionVPTemp[i].w)
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
        vec3 v3ViewDirection = v3PositionShadow[gl_InvocationID] - v3VertexPos[0];

        // If visible output triangle data
        if (bInFrustum && (dot(v3Normal, v3ViewDirection) < 0.0f)) {
            // Loop over each vertex in the face and output
            for (int i = 0; i < 3; ++i) {
                // Output position
                gl_Position = v4PositionVPTemp[i];

                //Pass-through UV coordinates
                v2UVOut = v2VertexUV[i];

                // Output to cubemap layer based on invocation ID
                gl_Layer = gl_InvocationID;
                EmitVertex();
            }
            EndPrimitive();
        }
    }
}
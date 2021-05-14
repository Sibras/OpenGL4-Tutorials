#version 430 core

layout(binding = 4) uniform CameraCubeData {
    mat4 m4ViewProjectionCube[6];
};

layout(triangles, invocations = 6) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 v3VertexPos[];
layout(location = 1) in vec3 v3VertexNormal[];
layout(location = 2) in vec2 v2VertexUV[];
layout(location = 3) in vec3 v3VertexTangent[];

layout(location = 0) smooth out vec3 v3PositionOut;
layout(location = 1) smooth out vec3 v3NormalOut;
layout(location = 2) smooth out vec2 v2UVOut;
layout(location = 3) smooth out vec3 v3TangentOut;

void main() {
    //// Loop over each vertex in the face and output
    //for(int i = 0; i < 3; ++i)
    //{
    //    // Transform position
    //    v3PositionOut = v3VertexPos[i];
    //    gl_Position = m4ViewProjectionCube[gl_InvocationID] * vec4(v3VertexPos[i], 1.0f);
    //
    //    // Transform normal
    //    vec4 v4Normal = m4ViewProjectionCube[gl_InvocationID] * vec4(v3VertexNormal[i], 0.0f);
    //    v3NormalOut = v4Normal.xyz;
    //
    //    //Pass-through UV coordinates
    //    v2UVOut = v2VertexUV[i];
    //
    //    // Transform tangent
    //    vec4 v4Tangent = m4ViewProjectionCube[gl_InvocationID] * vec4(v3VertexNormal[i], 0.0f);
    //    v3TangentOut = v4Tangent.xyz;
    //
    //    // Output to cubemap layer based on invocation ID
    //    gl_Layer = gl_InvocationID;
    //    EmitVertex();
    //}
    //EndPrimitive();

    vec4 v4PositionVPTemp[3];
    int iOutOfBound[6] = int[6](0, 0, 0, 0, 0, 0);
    // Loop over each vertex and get clip space position
    for(int i = 0; i < 3; ++i)
    {
        // Transform position
        v4PositionVPTemp[i] = m4ViewProjectionCube[gl_InvocationID] * vec4(v3VertexPos[i], 1.0f);

        // Check if any value is outside clip planes
        if(v4PositionVPTemp[i].x >  v4PositionVPTemp[i].w) iOutOfBound[0] = iOutOfBound[0] + 1;
        if(v4PositionVPTemp[i].x < -v4PositionVPTemp[i].w) iOutOfBound[1] = iOutOfBound[1] + 1;
        if(v4PositionVPTemp[i].y >  v4PositionVPTemp[i].w) iOutOfBound[2] = iOutOfBound[2] + 1;
        if(v4PositionVPTemp[i].y < -v4PositionVPTemp[i].w) iOutOfBound[3] = iOutOfBound[3] + 1;
        if(v4PositionVPTemp[i].z >  v4PositionVPTemp[i].w) iOutOfBound[4] = iOutOfBound[4] + 1;
        if(v4PositionVPTemp[i].z < -v4PositionVPTemp[i].w) iOutOfBound[5] = iOutOfBound[5] + 1;
    }

    // Loop over each clip face and check if triangle is completely outside
    bool bInFrustum = true;
    for(int i = 0; i < 6; ++i)
        if(iOutOfBound[i] == 3) bInFrustum = false;

    // If visible output triangle data
    if(bInFrustum)
    {
        // Loop over each vertex in the face and output
        for(int i = 0; i < 3; ++i)
        {
            // Output position
            v3PositionOut = v3VertexPos[i];
            gl_Position = v4PositionVPTemp[i];

            // Transform normal
            vec4 v4Normal = m4ViewProjectionCube[gl_InvocationID] * vec4(v3VertexNormal[i], 0.0f);
            v3NormalOut = v4Normal.xyz;

            //Pass-through UV coordinates
            v2UVOut = v2VertexUV[i];

            // Transform tangent
            vec4 v4Tangent = m4ViewProjectionCube[gl_InvocationID] *vec4(v3VertexNormal[i], 0.0f);
            v3TangentOut = v4Tangent.xyz;

            // Output to cubemap layer based on invocation ID
            gl_Layer = gl_InvocationID;
            EmitVertex();
        }
        EndPrimitive();
    }
}
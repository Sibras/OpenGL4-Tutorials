#include <GL/glew.h>
#include <math.h>
#include <glm/glm.hpp>
using namespace glm;

struct CustomVertex
{
    vec3 v3Position;
    vec3 v3Normal;
    vec2 v2UV;
};

GLsizei GL_GenerateCube(GLuint uiVBO, GLuint uiIBO)
{
    CustomVertex VertexData[] = {
        // Create back face
        { vec3(0.5f,  0.5f, -0.5f), vec3(0.0f,  0.0f, -1.0f), vec2(1.0f, 1.0f) },
        { vec3(0.5f, -0.5f, -0.5f), vec3(0.0f,  0.0f, -1.0f), vec2(1.0f, 0.5f) },
        { vec3(-0.5f, -0.5f, -0.5f), vec3(0.0f,  0.0f, -1.0f), vec2(0.5f, 0.5f) },
        { vec3(-0.5f,  0.5f, -0.5f), vec3(0.0f,  0.0f, -1.0f), vec2(0.5f, 1.0f) },
        // Create left face
        { vec3(-0.5f,  0.5f, -0.5f), vec3(-1.0f,  0.0f,  0.0f), vec2(0.5f, 1.0f) },
        { vec3(-0.5f, -0.5f, -0.5f), vec3(-1.0f,  0.0f,  0.0f), vec2(0.5f, 0.5f) },
        { vec3(-0.5f, -0.5f,  0.5f), vec3(-1.0f,  0.0f,  0.0f), vec2(0.0f, 0.5f) },
        { vec3(-0.5f,  0.5f,  0.5f), vec3(-1.0f,  0.0f,  0.0f), vec2(0.0f, 1.0f) },
        // Create bottom face
        { vec3(0.5f, -0.5f, -0.5f), vec3(0.0f, -1.0f,  0.0f), vec2(0.5f, 0.0f) },
        { vec3(0.5f, -0.5f,  0.5f), vec3(0.0f, -1.0f,  0.0f), vec2(0.5f, 0.5f) },
        { vec3(-0.5f, -0.5f,  0.5f), vec3(0.0f, -1.0f,  0.0f), vec2(0.0f, 0.5f) },
        { vec3(-0.5f, -0.5f, -0.5f), vec3(0.0f, -1.0f,  0.0f), vec2(0.0f, 0.0f) },
        // Create front face
        { vec3(-0.5f,  0.5f,  0.5f), vec3(0.0f,  0.0f,  1.0f), vec2(1.0f, 1.0f) },
        { vec3(-0.5f, -0.5f,  0.5f), vec3(0.0f,  0.0f,  1.0f), vec2(1.0f, 0.5f) },
        { vec3(0.5f, -0.5f,  0.5f), vec3(0.0f,  0.0f,  1.0f), vec2(0.5f, 0.5f) },
        { vec3(0.5f,  0.5f,  0.5f), vec3(0.0f,  0.0f,  1.0f), vec2(0.5f, 1.0f) },
        // Create right face
        { vec3(0.5f,  0.5f,  0.5f), vec3(1.0f,  0.0f,  0.0f), vec2(0.5f, 1.0f) },
        { vec3(0.5f, -0.5f,  0.5f), vec3(1.0f,  0.0f,  0.0f), vec2(0.5f, 0.5f) },
        { vec3(0.5f, -0.5f, -0.5f), vec3(1.0f,  0.0f,  0.0f), vec2(0.0f, 0.5f) },
        { vec3(0.5f,  0.5f, -0.5f), vec3(1.0f,  0.0f,  0.0f), vec2(0.0f, 1.0f) },
        // Create top face
        { vec3(0.5f,  0.5f,  0.5f), vec3(0.0f,  1.0f,  0.0f), vec2(1.0f, 0.5f) },
        { vec3(0.5f,  0.5f, -0.5f), vec3(0.0f,  1.0f,  0.0f), vec2(1.0f, 0.0f) },
        { vec3(-0.5f,  0.5f, -0.5f), vec3(0.0f,  1.0f,  0.0f), vec2(0.5f, 0.0f) },
        { vec3(-0.5f,  0.5f,  0.5f), vec3(0.0f,  1.0f,  0.0f), vec2(0.5f, 0.5f) },
    };

    GLuint uiIndexData[] = {
        // Create back face
         0,  1,  3,  3,  1,  2,
        // Create left face
         4,  5,  7,  7,  5,  6,
        // Create bottom face
         8,  9, 11, 11,  9, 10,
        // Create front face
        12, 13, 15, 15, 13, 14,
        // Create right face
        16, 17, 19, 19, 17, 18,
        // Create top face
        20, 21, 23, 23, 21, 22
    };

    // Fill Vertex Buffer Object
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData), VertexData, GL_STATIC_DRAW);

    // Fill Index Buffer Object
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uiIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uiIndexData), uiIndexData, GL_STATIC_DRAW);

    // Specify location of data within buffer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CustomVertex), (const GLvoid *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CustomVertex), (const GLvoid *)offsetof(CustomVertex, v3Normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(CustomVertex), (const GLvoid *)offsetof(CustomVertex, v2UV));
    glEnableVertexAttribArray(2);

    return (sizeof(uiIndexData) / sizeof(GLuint));
}

GLsizei GL_GenerateSphere(uint32_t uiTessU, uint32_t uiTessV, GLuint uiVBO, GLuint uiIBO)
{
    // Init params
    float fDPhi = (float)M_PI / (float)uiTessV;
    float fDTheta = (float)(M_PI + M_PI) / (float)uiTessU;

    // Determine required parameters
    uiTessU = uiTessU + 1;
    //uint32_t uiNumVertices = (uiTessU * (uiTessV - 1)) + 2;
    //uint32_t uiNumIndices = (uiTessU * 6) + (uiTessU * (uiTessV - 2) * 6);
    uint32_t uiNumVertices = (uiTessU * (uiTessV + 1));
    uint32_t uiNumIndices = uiTessU * uiTessV * 6;

    // Create the new primitive
    CustomVertex * p_VBuffer = (CustomVertex *)malloc(uiNumVertices * sizeof(CustomVertex));
    GLuint * p_IBuffer = (GLuint *)malloc(uiNumIndices * sizeof(GLuint));

    // Set the top and bottom vertex and reuse
    CustomVertex * p_vBuffer = p_VBuffer;
    //p_vBuffer->v3Position = vec3(0.0f, 1.0f, 0.0f);
    //p_vBuffer->v3Normal = vec3(0.0f, 1.0f, 0.0f);
    //p_vBuffer->v2UV = vec2(0.5f, 1.0f);
    //p_vBuffer[uiNumVertices - 1].v3Position = vec3(0.0f, -1.0f, 0.0f);
    //p_vBuffer[uiNumVertices - 1].v3Normal = vec3(0.0f, -1.0f, 0.0f);
    //p_vBuffer[uiNumVertices - 1].v2UV = vec2(0.5f, 0.0f);
    //p_vBuffer++;

    //float fPhi = fDPhi;
    float fPhi = 0.0f;
    //for (uint32_t uiPhi = 0; uiPhi < uiTessV - 1; uiPhi++) {
    for (uint32_t uiPhi = 0; uiPhi < uiTessV + 1; uiPhi++) {
        // Calculate initial value
        float fRSinPhi = sinf(fPhi);
        float fRCosPhi = cosf(fPhi);

        float fY = fRCosPhi;

        float fTheta = 0.0f;
        for (uint32_t uiTheta = 0; uiTheta < uiTessU; uiTheta++) {
            // Calculate positions
            float fCosTheta = cosf(fTheta);
            float fSinTheta = sinf(fTheta);

            // Determine position
            float fX = fRSinPhi * fCosTheta;
            float fZ = fRSinPhi * fSinTheta;

            // Create vertex
            p_vBuffer->v3Position = vec3(fX, fY, fZ);
            p_vBuffer->v3Normal = vec3(fX, fY, fZ);
            p_vBuffer->v2UV = vec2(1.0f - (fTheta / (float)(M_PI + M_PI)),
                                   1.0f - (fPhi / (float)M_PI));
            p_vBuffer++;
            fTheta += fDTheta;
        }
        fPhi += fDPhi;
    }

    GLuint * p_iBuffer = p_IBuffer;
    //for (GLuint j = 1; j <= uiTessU; j++) {
    //    // Top triangles all share same vertex point at pos 0
    //    *p_iBuffer++ = 0;
    //    *p_iBuffer++ = j + 1;
    //    *p_iBuffer++ = j;
    //}

    // Create inner triangles
    //for (GLuint i = 0; i < uiTessV - 2; i++) {
    for (GLuint i = 0; i < uiTessV; i++) {
        //for(GLuint j = 1; j <= uiTessU; j++)
        for (GLuint j = 0; j < uiTessU; j++) {
            // Create indexes for each quad face (pair of triangles)
            *p_iBuffer++ = j + (i * uiTessU);
            GLuint Index = j + 1;
            *p_iBuffer++ = Index + (i * uiTessU);
            *p_iBuffer++ = j + ((i + 1) * uiTessU);

            *p_iBuffer = *(p_iBuffer - 2);
            p_iBuffer++;
            *p_iBuffer++ = Index + ((i + 1) * uiTessU);
            *p_iBuffer = *(p_iBuffer - 3);
            p_iBuffer++;
        }
    }

    // Create bottom
    //for (GLuint j = 1; j <= uiTessU; j++) {
    //    // Bottom triangles all share same vertex point at pos uiNumVertices - 1
    //    *p_iBuffer++ = j + ((uiTessV - 2) * uiTessU);
    //    GLuint Index = j + 1;
    //    *p_iBuffer++ = Index + ((uiTessV - 2) * uiTessU);
    //    *p_iBuffer++ = uiNumVertices - 1;
    //}

    // Fill Vertex Buffer Object
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferData(GL_ARRAY_BUFFER, uiNumVertices * sizeof(CustomVertex), p_VBuffer, GL_STATIC_DRAW);

    // Fill Index Buffer Object
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uiIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, uiNumIndices * sizeof(GLuint), p_IBuffer, GL_STATIC_DRAW);

    // Cleanup allocated data
    free(p_VBuffer);
    free(p_IBuffer);

    // Specify location of data within buffer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CustomVertex), (const GLvoid *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CustomVertex), (const GLvoid *)offsetof(CustomVertex, v3Normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(CustomVertex), (const GLvoid *)offsetof(CustomVertex, v2UV));
    glEnableVertexAttribArray(2);

    return uiNumIndices;
}

GLsizei GL_GeneratePyramid(GLuint uiVBO, GLuint uiIBO)
{
    CustomVertex VertexData[] =
    {
        // Create back face
        { vec3(0.0f,  0.5f,  0.0f), vec3(0.0f,  0.5f, -0.5f), vec2(0.5f, 1.0f) },
        { vec3(0.5f, -0.5f, -0.5f), vec3(0.0f,  0.5f, -0.5f), vec2(1.0f, 0.5f) },
        { vec3(-0.5f, -0.5f, -0.5f), vec3(0.0f,  0.5f, -0.5f), vec2(0.0f, 0.5f) },
        // Create left face
        { vec3(0.0f,  0.5f,  0.0f), vec3(-0.5f,  0.5f,  0.0f), vec2(0.5f, 1.0f) },
        { vec3(-0.5f, -0.5f, -0.5f), vec3(-0.5f,  0.5f,  0.0f), vec2(1.0f, 0.5f) },
        { vec3(-0.5f, -0.5f,  0.5f), vec3(-0.5f,  0.5f,  0.0f), vec2(0.0f, 0.5f) },
        // Create bottom face
        { vec3(0.5f, -0.5f, -0.5f), vec3(0.0f, -1.0f,  0.0f), vec2(1.0f, 0.0f) },
        { vec3(0.5f, -0.5f,  0.5f), vec3(0.0f, -1.0f,  0.0f), vec2(1.0f, 0.5f) },
        { vec3(-0.5f, -0.5f,  0.5f), vec3(0.0f, -1.0f,  0.0f), vec2(0.0f, 0.5f) },
        { vec3(-0.5f, -0.5f, -0.5f), vec3(0.0f, -1.0f,  0.0f), vec2(0.0f, 0.0f) },
        // Create front face
        { vec3(0.0f,  0.5f,  0.0f), vec3(0.0f,  0.5f,  0.5f), vec2(0.5f, 1.0f) },
        { vec3(-0.5f, -0.5f,  0.5f), vec3(0.0f,  0.5f,  0.5f), vec2(1.0f, 0.5f) },
        { vec3(0.5f, -0.5f,  0.5f), vec3(0.0f,  0.5f,  0.5f), vec2(0.0f, 0.5f) },
        // Create right face
        { vec3(0.0f,  0.5f,  0.0f), vec3(0.5f,  0.5f,  0.0f), vec2(0.5f, 1.0f) },
        { vec3(0.5f, -0.5f,  0.5f), vec3(0.5f,  0.5f,  0.0f), vec2(1.0f, 0.5f) },
        { vec3(0.5f, -0.5f, -0.5f), vec3(0.5f,  0.5f,  0.0f), vec2(0.0f, 0.5f) },
    };

    GLuint uiIndexData[] =
    {
        // Create back face
         0,  1,  2,
        // Create left face
         3,  4,  5,
        // Create bottom face
         6,  7,  9,  9,  7,  8,
        // Create front face
        10, 11, 12,
        // Create right face
        13, 14, 15
    };

    // Fill Vertex Buffer Object
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData), VertexData, GL_STATIC_DRAW);

    // Fill Index Buffer Object
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uiIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uiIndexData), uiIndexData, GL_STATIC_DRAW);

    // Specify location of data within buffer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CustomVertex), (const GLvoid *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CustomVertex), (const GLvoid *)offsetof(CustomVertex, v3Normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(CustomVertex), (const GLvoid *)offsetof(CustomVertex, v2UV));
    glEnableVertexAttribArray(2);

    return (sizeof(uiIndexData) / sizeof(GLuint));
}
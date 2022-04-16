#include <GL/glew.h>
#include <math.h>
#include <glm/glm.hpp>
using namespace glm;

struct CustomVertex
{
    vec3 v3Position;
};

GLsizei GL_GenerateCube(GLuint uiVBO, GLuint uiIBO)
{
    CustomVertex VertexData[] = {
        { vec3(0.5f,  0.5f, -0.5f) },
        { vec3(0.5f, -0.5f, -0.5f) },
        { vec3(-0.5f, -0.5f, -0.5f) },
        { vec3(-0.5f,  0.5f, -0.5f) },
        { vec3(-0.5f, -0.5f,  0.5f) },
        { vec3(-0.5f,  0.5f,  0.5f) },
        { vec3(0.5f, -0.5f,  0.5f) },
        { vec3(0.5f,  0.5f,  0.5f) }
    };

    GLuint uiIndexData[] = {
        0, 1, 3, 3, 1, 2,  // Create back face
        3, 2, 5, 5, 2, 4,  // Create left face
        1, 6, 2, 2, 6, 4,  // Create bottom face
        5, 4, 7, 7, 4, 6,  // Create front face
        7, 6, 0, 0, 6, 1,  // Create right face
        7, 0, 5, 5, 0, 3   // Create top face
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

    return (sizeof(uiIndexData) / sizeof(GLuint));
}

GLsizei GL_GenerateSphere(uint32_t uiTessU, uint32_t uiTessV, GLuint uiVBO, GLuint uiIBO)
{
    // Init params
    float fDPhi = (float)M_PI / (float)uiTessV;
    float fDTheta = (float)(M_PI + M_PI) / (float)uiTessU;

    // Determine required parameters
    uint32_t uiNumVertices = (uiTessU * (uiTessV - 1)) + 2;
    uint32_t uiNumIndices = (uiTessU * 6) + (uiTessU * (uiTessV - 2) * 6);

    // Create the new primitive
    CustomVertex * p_VBuffer = (CustomVertex *)malloc(uiNumVertices * sizeof(CustomVertex));
    GLuint * p_IBuffer = (GLuint *)malloc(uiNumIndices * sizeof(GLuint));

    // Set the top and bottom vertex and reuse
    CustomVertex * p_vBuffer = p_VBuffer;
    p_vBuffer->v3Position = vec3(0.0f, 1.0f, 0.0f);
    p_vBuffer[uiNumVertices - 1].v3Position = vec3(0.0f, -1.0f, 0.0f);
    p_vBuffer++;

    float fPhi = fDPhi;
    for (uint32_t uiPhi = 0; uiPhi < uiTessV - 1; uiPhi++) {
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
            p_vBuffer++;
            fTheta += fDTheta;
        }
        fPhi += fDPhi;
    }

    // Create top
    GLuint * p_iBuffer = p_IBuffer;
    for (GLuint j = 1; j <= uiTessU; j++) {
        // Top triangles all share same vertex point at pos 0
        *p_iBuffer++ = 0;
        // Loop back to start if required
        *p_iBuffer++ = ((j + 1) > uiTessU) ? 1 : j + 1;
        *p_iBuffer++ = j;
    }

    // Create inner triangles
    for (GLuint i = 0; i < uiTessV - 2; i++) {
        for (GLuint j = 1; j <= uiTessU; j++) {
            // Create indexes for each quad face (pair of triangles)
            *p_iBuffer++ = j + (i * uiTessU);
            // Loop back to start if required
            GLuint Index = ((j + 1) > uiTessU) ? 1 : j + 1;
            *p_iBuffer++ = Index + (i * uiTessU);
            *p_iBuffer++ = j + ((i + 1) * uiTessU);

            *p_iBuffer = *(p_iBuffer - 2);
            p_iBuffer++;
            // Loop back to start if required
            *p_iBuffer++ = Index + ((i + 1) * uiTessU);
            *p_iBuffer = *(p_iBuffer - 3);
            p_iBuffer++;
        }
    }

    // Create bottom
    for (GLuint j = 1; j <= uiTessU; j++) {
        // Bottom triangles all share same vertex at uiNumVertices-1
        *p_iBuffer++ = j + ((uiTessV - 2) * uiTessU);
        // Loop back to start if required
        GLuint Index = ((j + 1) > uiTessU) ? 1 : j + 1;
        *p_iBuffer++ = Index + ((uiTessV - 2) * uiTessU);
        *p_iBuffer++ = uiNumVertices - 1;
    }

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

    return uiNumIndices;
}

GLsizei GL_GeneratePyramid(GLuint uiVBO, GLuint uiIBO)
{
    CustomVertex VertexData[] =
    {
        { vec3(0.0f,  0.5f,  0.0f) },
        { vec3(0.5f, -0.5f, -0.5f) },
        { vec3(-0.5f, -0.5f, -0.5f) },
        { vec3(-0.5f, -0.5f,  0.5f) },
        { vec3(0.5f, -0.5f,  0.5f) }
    };

    GLuint uiIndexData[] =
    {
        // Create back face
        0, 1, 2,
        // Create left face
        0, 2, 3,
        // Create bottom face
        1, 4, 2, 2, 4, 3,
        // Create front face
        0, 3, 4,
        // Create right face
        0, 4, 1
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

    return (sizeof(uiIndexData) / sizeof(GLuint));
}
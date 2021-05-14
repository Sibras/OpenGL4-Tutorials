// Using GLM and math headers
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GLScene.h> //Need '<' to enforce loading of project local header

//Main.cpp
extern int g_iWindowWidth;
extern int g_iWindowHeight;
extern GLuint g_uiMainProgram;
extern GLuint g_uiShadowProgram;
extern GLuint g_uiShadowTransProgram;
extern SceneData g_SceneData;
//Reflection.cpp
void GL_CalculateCubeMapVP(const vec3 & v3Position, mat4 * p_m4CubeViewProjections, float fNear, float fFar);

// Spot Shadows
GLuint g_uiFBOShadow;
GLuint g_uiShadowArray;
GLuint g_uiShadowUBO;
GLuint g_uiShadowPosUBO;
// Point Shadows
GLuint g_uiShadowCubeArray;
GLuint g_uiShadowCubeUBO;
GLuint g_uiShadowCubePosUBO;
// Spot Transparency
GLuint g_uiFBOTransparency;
GLuint g_uiTransparencyArray;
GLuint g_uiTransparencyDepthArray;

struct ShadowPosData
{
    // This is required to ensure the compiler respects the alignment of elements in an array
    aligned_vec3 m_v3Position;
};

void GL_RenderObjectsDepth()
{
    // Clear the depth buffer
    glClear(GL_DEPTH_BUFFER_BIT);

    // Loop through each object
    for (unsigned i = 0; i < g_SceneData.m_uiNumObjects; i++) {
        const ObjectData * p_Object = &g_SceneData.mp_Objects[i];

        // Skip if transparent
        if (p_Object->m_bTransparent)
            continue;

        // Bind VAO
        glBindVertexArray(p_Object->m_uiVAO);

        // Bind the Transform UBO
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, p_Object->m_uiTransformUBO);

        // Draw the Object
        glDrawElements(GL_TRIANGLES, p_Object->m_uiNumIndices, GL_UNSIGNED_INT, 0);
    }
}

float GL_CalculateFalloffDistance(const vec3 & v3Falloff)
{
    // Calculate falloff size
    const float fFalloffThreshold = 256.0f;
    float fK0 = v3Falloff.x;
    float fK1 = v3Falloff.y;
    float fK2 = v3Falloff.z;
    float fFalloff = FLT_MAX;
    if (fK2 != 0.0f) {
        // Use Quadratic falloff
        fFalloff = -fK1 + sqrt((fK1 * fK1) - (4.0f * fK2 * (fK0 - fFalloffThreshold)));
        fFalloff /= 2.0f * fK2;
    } else if (fK1 != 0.0f) {
        // Use linear falloff
        fFalloff = (fFalloffThreshold - fK0) / fK1;
    }
    return fFalloff;
}

void GL_RenderSpotShadows()
{
    // Update the program and viewport
    glUseProgram(g_uiShadowProgram);
    glViewport(0, 0, g_iWindowWidth, g_iWindowWidth);

    unsigned uiSizeLights = sizeof(mat4) * g_SceneData.m_uiNumSpotLights;
    mat4 * p_ViewProjections = (mat4 *)malloc(uiSizeLights);
    unsigned uiSizeLightsPos = sizeof(ShadowPosData) * g_SceneData.m_uiNumSpotLights;
    ShadowPosData * p_Positions = (ShadowPosData *)malloc(uiSizeLightsPos);

    // Generate spot light view projection matrices
    for (unsigned i = 0; i < g_SceneData.m_uiNumSpotLights; i++) {
        SpotLightData * p_SpotLight = &g_SceneData.mp_SpotLights[i];

        // Calculate view matrix
        mat4 m4LightView = lookAt(p_SpotLight->m_v3Position,
                                  p_SpotLight->m_v3Position - p_SpotLight->m_v3Direction,
                                  vec3(0.0f, 1.0f, 0.0f));

        // Get falloff distance
        float fFalloff = GL_CalculateFalloffDistance(p_SpotLight->m_v3Falloff);

        // Clamp falloff to scene bounds
        fFalloff = min(fFalloff, g_SceneData.m_LocalCamera.m_fFar * 1.5f);

        // Calculate projection matrix
        mat4 m4LightProjection = perspective(
            acos(p_SpotLight->m_fAngle) * 2.0f,
            1.0f,
            0.1f, fFalloff
        );

        p_ViewProjections[i] = m4LightProjection * m4LightView;

        // Set light positions
        p_Positions[i].m_v3Position = p_SpotLight->m_v3Position;

        // Update lights falloff value
        p_SpotLight->m_fFalloffDist = fFalloff;
    }

    // Update spot light UBO with new falloff value
    glBindBuffer(GL_UNIFORM_BUFFER, g_SceneData.m_uiSpotLightUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(SpotLightData) * g_SceneData.m_uiNumSpotLights, g_SceneData.mp_SpotLights, GL_STATIC_DRAW);

    // Fill shadow UBO
    glBindBuffer(GL_UNIFORM_BUFFER, g_uiShadowUBO);
    glBufferData(GL_UNIFORM_BUFFER, uiSizeLights, p_ViewProjections, GL_STATIC_DRAW);

    free(p_ViewProjections);

    // Fill shadow position UBO
    glBindBuffer(GL_UNIFORM_BUFFER, g_uiShadowPosUBO);
    glBufferData(GL_UNIFORM_BUFFER, uiSizeLightsPos, p_Positions, GL_STATIC_DRAW);

    free(p_Positions);

    // Bind shadow map UBO
    glBindBufferBase(GL_UNIFORM_BUFFER, 6, g_uiShadowUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 9, g_uiShadowPosUBO);

    // Update number of valid lights
    glProgramUniform1i(g_uiShadowProgram, 0, g_SceneData.m_uiNumSpotLights);

    // Attach buffers to FBO
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, g_uiShadowArray, 0);

    GL_RenderObjectsDepth();
}

void GL_RenderPointShadows()
{
    // Update the program and viewport
    glUseProgram(g_uiShadowProgram);
    glViewport(0, 0, g_iWindowWidth, g_iWindowWidth);

    unsigned uiSizeLights = sizeof(mat4) * g_SceneData.m_uiNumPointLights * 6;
    mat4 * p_ViewProjections = (mat4 *)malloc(uiSizeLights);
    unsigned uiSizeLightsPos = sizeof(ShadowPosData) * g_SceneData.m_uiNumPointLights * 6;
    ShadowPosData * p_Positions = (ShadowPosData *)malloc(uiSizeLightsPos);

    // Generate point light view projection matrices
    for (unsigned i = 0; i < g_SceneData.m_uiNumPointLights; i++) {
        PointLightData * p_PointLight = &g_SceneData.mp_PointLights[i];

        // Get falloff distance
        float fFalloff = GL_CalculateFalloffDistance(p_PointLight->m_v3Falloff);

        // Clamp falloff to scene bounds
        fFalloff = min(fFalloff, g_SceneData.m_LocalCamera.m_fFar);

        // Update lights near and far plane
        p_PointLight->m_v2NearFar = vec2(0.1f, fFalloff);

        // Calculate cube map VPs
        GL_CalculateCubeMapVP(p_PointLight->m_v3Position, &p_ViewProjections[i * 6],
                              p_PointLight->m_v2NearFar.x, p_PointLight->m_v2NearFar.y);

        // Set light positions
        for (unsigned j = 0; j < 6; j++) {
            p_Positions[(i * 6) + j].m_v3Position = p_PointLight->m_v3Position;
        }
    }

    // Update point light UBO with new near/far values
    glBindBuffer(GL_UNIFORM_BUFFER, g_SceneData.m_uiPointLightUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(PointLightData) * g_SceneData.m_uiNumPointLights, g_SceneData.mp_PointLights, GL_STATIC_DRAW);

    // Fill shadow UBO
    glBindBuffer(GL_UNIFORM_BUFFER, g_uiShadowCubeUBO);
    glBufferData(GL_UNIFORM_BUFFER, uiSizeLights, p_ViewProjections, GL_STATIC_DRAW);

    free(p_ViewProjections);

    // Fill shadow position UBO
    glBindBuffer(GL_UNIFORM_BUFFER, g_uiShadowCubePosUBO);
    glBufferData(GL_UNIFORM_BUFFER, uiSizeLightsPos, p_Positions, GL_STATIC_DRAW);

    free(p_Positions);

    // Update number of valid lights
    glProgramUniform1i(g_uiShadowProgram, 0, g_SceneData.m_uiNumPointLights * 6);

    // Bind shadow map UBO
    glBindBufferBase(GL_UNIFORM_BUFFER, 6, g_uiShadowCubeUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 9, g_uiShadowCubePosUBO);

    // Attach buffers to FBO
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, g_uiShadowCubeArray, 0);

    GL_RenderObjectsDepth();
}

void GL_RenderSpotTransparency()
{
    // Bind shadow map frame buffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBOTransparency);

    // Update the program and viewport
    glUseProgram(g_uiShadowTransProgram);
    glViewport(0, 0, g_iWindowWidth, g_iWindowWidth);

    // Assumes called after spot shadow so UBO already bound
    glBindBufferBase(GL_UNIFORM_BUFFER, 6, g_uiShadowUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 9, g_uiShadowPosUBO);

    // Update number of valid lights
    glProgramUniform1i(g_uiShadowTransProgram, 0, g_SceneData.m_uiNumSpotLights);

    // Clear the colour buffer only
    glClear(GL_COLOR_BUFFER_BIT);

    // Disable depth writing
    glDepthMask(false);

    // Loop through each object
    for (unsigned i = 0; i < g_SceneData.m_uiNumTransObjects; i++) {
        const ObjectData * p_Object = &g_SceneData.mp_TransObjects[i];

        // Bind VAO
        glBindVertexArray(p_Object->m_uiVAO);

        // Bind the Transform UBO
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, p_Object->m_uiTransformUBO);

        // Bind the diffuse texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, p_Object->m_uiDiffuse);

        // Draw the Object
        glDrawElements(GL_TRIANGLES, p_Object->m_uiNumIndices, GL_UNSIGNED_INT, 0);
    }

    // Reset depth writing
    glDepthMask(true);
}

void GL_RenderShadows()
{
    // Bind shadow map frame buffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBOShadow);

    // Setup culling and depth bias
    glCullFace(GL_FRONT);
    glPolygonOffset(0.9f, 0.0f);
    glEnable(GL_POLYGON_OFFSET_FILL);

    if (g_SceneData.m_uiNumPointLights > 0)
        GL_RenderPointShadows();

    if (g_SceneData.m_uiNumSpotLights > 0) {
        GL_RenderSpotShadows();

        GL_RenderSpotTransparency();
    }

    // Reset culling and depth bias
    glCullFace(GL_BACK);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // Ensure spot shadow UBO is still bound as its used for rendering
    glBindBufferBase(GL_UNIFORM_BUFFER, 6, g_uiShadowUBO);

    // Reset to default program and viewport
    glUseProgram(g_uiMainProgram);
    glViewport(0, 0, g_iWindowWidth, g_iWindowHeight);
}

bool GL_InitShadow()
{
    // Create shadow map frame buffer
    glGenFramebuffers(1, &g_uiFBOShadow);
    glBindFramebuffer(GL_FRAMEBUFFER, g_uiFBOShadow);

    // Disable colour buffer output
    glDrawBuffer(GL_NONE);

    if (g_SceneData.m_uiNumPointLights > 0) {
        // Generate array of point light shadow map textures
        glGenTextures(1, &g_uiShadowCubeArray);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, g_uiShadowCubeArray);
        glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_DEPTH_COMPONENT32F,
                       g_iWindowWidth, g_iWindowWidth, g_SceneData.m_uiNumPointLights * 6);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        // Generate cube shadow UBO
        glGenBuffers(1, &g_uiShadowCubeUBO);
        glGenBuffers(1, &g_uiShadowCubePosUBO);

        // Bind the shadow map texture
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, g_uiShadowCubeArray);
    }

    if (g_SceneData.m_uiNumSpotLights > 0) {
        // Generate array of spot light shadow map textures
        glGenTextures(1, &g_uiShadowArray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, g_uiShadowArray);
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT32F,
                       g_iWindowWidth, g_iWindowWidth, g_SceneData.m_uiNumSpotLights);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        const GLfloat fMinDepth[] = {0.0f, 0.0f, 0.0f, 0.0f};
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, (GLfloat *)&fMinDepth);

        // Generate shadow UBO
        glGenBuffers(1, &g_uiShadowUBO);
        glGenBuffers(1, &g_uiShadowPosUBO);

        // Bind the shadow map texture
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D_ARRAY, g_uiShadowArray);
        glActiveTexture(GL_TEXTURE0);

        // Generate transparency FBO
        glGenFramebuffers(1, &g_uiFBOTransparency);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBOTransparency);

        // Generate transparency colour buffer
        glGenTextures(1, &g_uiTransparencyArray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, g_uiTransparencyArray);
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGB8,
                       g_iWindowWidth, g_iWindowWidth, g_SceneData.m_uiNumSpotLights);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Attach buffers to FBO
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, g_uiShadowArray, 0);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, g_uiTransparencyArray, 0);

        // Bind the shadow map transparency texture
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D_ARRAY, g_uiTransparencyArray);
    }

    // Reset active texture unit
    glActiveTexture(GL_TEXTURE0);

    // Generate shadows
    GL_RenderShadows();

    return true;
}

void GL_QuitShadow()
{
    // Release shadow map FBO data
    glDeleteFramebuffers(1, &g_uiFBOShadow);
    glDeleteTextures(1, &g_uiShadowArray);
    glDeleteBuffers(1, &g_uiShadowUBO);
    glDeleteBuffers(1, &g_uiShadowPosUBO);
    glDeleteTextures(1, &g_uiShadowCubeArray);
    glDeleteBuffers(1, &g_uiShadowCubeUBO);
    glDeleteBuffers(1, &g_uiShadowCubePosUBO);
    glDeleteFramebuffers(1, &g_uiFBOTransparency);
    glDeleteTextures(1, &g_uiTransparencyArray);
}
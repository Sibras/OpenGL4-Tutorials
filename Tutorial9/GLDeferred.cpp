// Using GLM and math headers
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GLScene.h> //Need '<' to enforce loading of project local header

//Main.cpp
extern int g_iWindowWidth;
extern int g_iWindowHeight;
extern GLuint g_uiMainProgram;
extern GLuint g_uiDeferredProgram2;
extern GLuint g_uiPostProcProgram;
extern SceneData g_SceneData;
extern void GL_RenderObjects(ObjectData * p_Object = NULL);
// Subroutine index for GBuffer display
extern unsigned g_uiOutputSubroutine;

// Deferred rendering data
GLuint g_uiFBODeferred;
GLuint g_uiFBODeferred2;
GLuint g_uiDepth;
GLuint g_uiAccumulation;
GLuint g_uiNormal;
GLuint g_uiDiffuse;
GLuint g_uiSpecularRough;

// Screen quad
GLuint g_uiQuadVAO;
GLuint g_uiQuadVBO;
GLuint g_uiQuadIBO;

// Inverse resolution UBO
GLuint g_uiInverseResUBO;

void GL_RenderDeferred(ObjectData * p_Object, GLuint uiAccumBuffer, GLenum uiTextureTarget)
{
    // Bind deferred frame buffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBODeferred);

    // Attach optional accumulation buffer
    if (uiAccumBuffer > 0)
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, uiTextureTarget, uiAccumBuffer, 0);

    // Bind first deferred program
    glUseProgram(g_uiMainProgram);

    // Render all objects
    GL_RenderObjects(p_Object);

    // Disable depth checks
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Bind full screen quad
    glBindVertexArray(g_uiQuadVAO);

    // Bind second deferred frame buffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBODeferred2);

    // Attach optional accumulation buffer
    if (uiAccumBuffer > 0)
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, uiTextureTarget, uiAccumBuffer, 0);

    //Enable blending
    glEnablei(GL_BLEND, 0);

    // Bind second deferred program
    glUseProgram(g_uiDeferredProgram2);

    // Draw full screen quad
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

    // Disable blending
    glDisablei(GL_BLEND, 0);

    // Reset accumulation buffer
    if (uiAccumBuffer > 0) {
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_uiAccumulation, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBODeferred);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_uiAccumulation, 0);

        // Enable depth tests again
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
    }
}

void GL_RenderPostProcess()
{
    // Bind final program
    glUseProgram(g_uiPostProcProgram);

    // Set the subroutines for optional GBuffer display
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &g_uiOutputSubroutine);

    // Bind default frame buffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw full screen quad
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

    // Enable depth tests again
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

bool GL_InitDeferred()
{
    // Get light uniform location and set
    glProgramUniform1i(g_uiDeferredProgram2, 0, g_SceneData.m_uiNumPointLights);
    glProgramUniform1i(g_uiDeferredProgram2, 2, g_SceneData.m_uiNumSpotLights);

    // Create first deferred rendering frame buffer
    glGenFramebuffers(1, &g_uiFBODeferred);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBODeferred);

    // Setup depth attachment
    glGenTextures(1, &g_uiDepth);
    glBindTexture(GL_TEXTURE_2D, g_uiDepth);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, g_iWindowWidth, g_iWindowHeight);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Setup accumulation attachment
    glGenTextures(1, &g_uiAccumulation);
    glBindTexture(GL_TEXTURE_2D, g_uiAccumulation);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R11F_G11F_B10F, g_iWindowWidth, g_iWindowHeight);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Setup normal attachment
    glGenTextures(1, &g_uiNormal);
    glBindTexture(GL_TEXTURE_2D, g_uiNormal);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG16F, g_iWindowWidth, g_iWindowHeight);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Setup diffuse attachment
    glGenTextures(1, &g_uiDiffuse);
    glBindTexture(GL_TEXTURE_2D, g_uiDiffuse);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, g_iWindowWidth, g_iWindowHeight);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Setup specular and rough attachment
    glGenTextures(1, &g_uiSpecularRough);
    glBindTexture(GL_TEXTURE_2D, g_uiSpecularRough);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, g_iWindowWidth, g_iWindowHeight);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Attach frame buffer attachments
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_uiDepth, 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_uiAccumulation, 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_uiNormal, 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, g_uiDiffuse, 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, g_uiSpecularRough, 0);

    // Enable frame buffer attachments
    GLenum uiDrawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glDrawBuffers(4, uiDrawBuffers);

    // Create and attach second frame buffer attachments
    glGenFramebuffers(1, &g_uiFBODeferred2);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBODeferred2);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_uiAccumulation, 0);

    // Bind deferred textures
    glActiveTexture(GL_TEXTURE11);
    glBindTexture(GL_TEXTURE_2D, g_uiDepth);
    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_2D, g_uiNormal);
    glActiveTexture(GL_TEXTURE13);
    glBindTexture(GL_TEXTURE_2D, g_uiDiffuse);
    glActiveTexture(GL_TEXTURE14);
    glBindTexture(GL_TEXTURE_2D, g_uiSpecularRough);
    glActiveTexture(GL_TEXTURE15);
    glBindTexture(GL_TEXTURE_2D, g_uiAccumulation);
    glActiveTexture(GL_TEXTURE0);

    // Generate the full screen quad
    glGenVertexArrays(1, &g_uiQuadVAO);
    glGenBuffers(1, &g_uiQuadVBO);
    glGenBuffers(1, &g_uiQuadIBO);
    glBindVertexArray(g_uiQuadVAO);

    // Create VBO data
    glBindBuffer(GL_ARRAY_BUFFER, g_uiQuadVBO);
    GLfloat fVertexData[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(fVertexData), fVertexData, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const GLvoid *)0);
    glEnableVertexAttribArray(0);

    // Create IBO data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_uiQuadIBO);
    GLubyte ubIndexData[] = {
        0, 1, 3,
        1, 2, 3
    };
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_uiQuadIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ubIndexData), ubIndexData, GL_STATIC_DRAW);

    // Setup blending parameters
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);

    // Setup inverse resolution
    glGenBuffers(1, &g_uiInverseResUBO);
    vec2 v2InverseRes = 1.0f / vec2((float)g_iWindowWidth, (float)g_iWindowHeight);
    glBindBuffer(GL_UNIFORM_BUFFER, g_uiInverseResUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(vec2), &v2InverseRes, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 7, g_uiInverseResUBO);

    return true;
}

void GL_QuitDeferred()
{
    // Release deferred FBO data
    glDeleteFramebuffers(1, &g_uiFBODeferred);
    glDeleteFramebuffers(1, &g_uiFBODeferred2);
    glDeleteTextures(1, &g_uiDepth);
    glDeleteTextures(1, &g_uiNormal);
    glDeleteTextures(1, &g_uiDiffuse);
    glDeleteTextures(1, &g_uiSpecularRough);
    glDeleteTextures(1, &g_uiAccumulation);

    // Release full screen quad
    glDeleteBuffers(1, &g_uiQuadVAO);
    glDeleteBuffers(1, &g_uiQuadVBO);
    glDeleteVertexArrays(1, &g_uiQuadIBO);

    // Release resolution UBO
    glDeleteBuffers(1, &g_uiInverseResUBO);
}
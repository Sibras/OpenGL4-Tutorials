// Using GLEW, GLM and math headers
#include <math.h>
#include <glm/glm.hpp>
#define GLEW_STATIC
#include <GL\glew.h>

using namespace glm;

//Main.cpp
extern int g_iWindowWidth;
extern int g_iWindowHeight;
extern GLuint g_uiPostProcProgram;
extern GLuint g_uiPostProcInitProgram;
extern GLuint g_uiGaussProgram;
//Deferred.cpp
extern GLuint g_uiAccumulation;
extern GLuint g_uiQuadVAO;
extern GLuint g_uiInverseResUBO;

// Post processing frame buffer
GLuint g_uiFBOPostProc;
GLuint g_uiLuminanceKey;
GLuint g_uiBloom;
GLuint g_uiFBOBlur;
GLuint g_uiFBOBloom;
GLuint g_uiBlur;

void GL_RenderPostProcess()
{
    // Bind initialisation program
    glUseProgram(g_uiPostProcInitProgram);

    // Bind post-process frame buffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBOPostProc);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw full screen quad
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

    // Bind texture and generate mipmaps on luminance key texture
    glBindTexture(GL_TEXTURE_2D, g_uiLuminanceKey);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Bind bloom texture and constrain to lowest level
    glActiveTexture(GL_TEXTURE17);
    glBindTexture(GL_TEXTURE_2D, g_uiBloom);
    //glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    // Half viewport
    glViewport(0, 0, g_iWindowWidth / 2, g_iWindowHeight / 2);
    vec2 v2InverseRes = 1.0f / vec2((float)(g_iWindowWidth / 2), (float)(g_iWindowHeight / 2));
    glBindBuffer(GL_UNIFORM_BUFFER, g_uiInverseResUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(vec2), &v2InverseRes, GL_STATIC_DRAW);

    // Bind blur frame buffer and program
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBOBlur);
    glUseProgram(g_uiGaussProgram);

    // Perform horizontal blur
    GLuint uiSubRoutines[2] = {0, 1};
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &uiSubRoutines[0]);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

    // Perform vertical blur
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBOBloom);
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &uiSubRoutines[1]);
    glBindTexture(GL_TEXTURE_2D, g_uiBlur);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

    // Bind second blur mipmap level as texture input
    glBindTexture(GL_TEXTURE_2D, g_uiBloom);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);

    const int iBlurPasses = 5;
    for (int i = 1; i < iBlurPasses; i++) {
        // Perform horizontal blur
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBOBlur);
        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &uiSubRoutines[0]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

        // Perform vertical blur
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBOBloom);
        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &uiSubRoutines[1]);
        glBindTexture(GL_TEXTURE_2D, g_uiBlur);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

        // Rebind bloom texture
        glBindTexture(GL_TEXTURE_2D, g_uiBloom);
    }

    // Reset viewport
    glViewport(0, 0, g_iWindowWidth, g_iWindowHeight);
    v2InverseRes = 1.0f / vec2((float)g_iWindowWidth, (float)g_iWindowHeight);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(vec2), &v2InverseRes, GL_STATIC_DRAW);

    // Bind default frame buffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Bind final program
    glUseProgram(g_uiPostProcProgram);

    // Draw full screen quad
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

    // Enable depth tests again
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

bool GL_InitPostProcess()
{
    // Create post-process frame buffer
    glGenFramebuffers(1, &g_uiFBOPostProc);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBOPostProc);

    // Generate attachment textures
    glGenTextures(1, &g_uiLuminanceKey);
    glGenTextures(1, &g_uiBloom);

    // Setup luminance attachment
    glBindTexture(GL_TEXTURE_2D, g_uiLuminanceKey);
    int iLevels = (int)ceilf(log2f((float)max(g_iWindowWidth, g_iWindowHeight)));
    glTexStorage2D(GL_TEXTURE_2D, iLevels, GL_R16, g_iWindowWidth, g_iWindowHeight);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Setup bloom attachment
    glBindTexture(GL_TEXTURE_2D, g_uiBloom);
    iLevels = 2;
    glTexStorage2D(GL_TEXTURE_2D, iLevels, GL_R11F_G11F_B10F, g_iWindowWidth, g_iWindowHeight);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, iLevels - 1);

    // Attach frame buffer attachments
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_uiLuminanceKey, 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_uiBloom, 0);

    // Enable frame buffer attachments
    GLenum uiDrawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, uiDrawBuffers);

    // Bind luminance texture
    glActiveTexture(GL_TEXTURE16);
    glBindTexture(GL_TEXTURE_2D, g_uiLuminanceKey);
    glActiveTexture(GL_TEXTURE0);

    // Create blur frame buffers
    glGenFramebuffers(1, &g_uiFBOBlur);
    glGenFramebuffers(1, &g_uiFBOBloom);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBOBlur);

    // Create blur texture
    glGenTextures(1, &g_uiBlur);
    glBindTexture(GL_TEXTURE_2D, g_uiBlur);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R11F_G11F_B10F, g_iWindowWidth / 2, g_iWindowHeight / 2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach frame buffer attachments
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_uiBlur, 0);

    // Attach second frame buffer to bloom second mipmap level
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_uiFBOBloom);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_uiBloom, 1);

    return true;
}

void GL_QuitPostProcess()
{
    // Release post-process FBO data
    glDeleteFramebuffers(1, &g_uiFBOPostProc);
    glDeleteTextures(2, &g_uiLuminanceKey);

    // Release blur data
    glDeleteFramebuffers(2, &g_uiFBOBlur);
    glDeleteTextures(1, &g_uiBlur);
}
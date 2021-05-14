// Using GLM and math headers
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/type_aligned.hpp>
using namespace glm;
// Using SDL, GLEW
#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

//Debug.cpp
extern void GLDebug_Init();
//Shaders.cpp
extern bool GL_LoadShader(GLuint & uiShader, GLenum ShaderType, const GLchar * p_cShader);
extern bool GL_LoadShaders(GLuint & uiShader, GLuint uiVertexShader, GLuint uiFragmentShader);
extern bool GL_LoadShaderFile(GLuint & uiShader, GLenum ShaderType, const char * p_cFileName, int iFileID);
//Geometry.cpp
extern GLsizei GL_GenerateCube(GLuint uiVBO, GLuint uiIBO);
extern GLsizei GL_GenerateSphere(uint32_t uiTessU, uint32_t uiTessV, GLuint uiVBO, GLuint uiIBO);
extern GLsizei GL_GeneratePyramid(GLuint uiVBO, GLuint uiIBO);

// Declare window variables
int g_iWindowWidth = 1280;
int g_iWindowHeight = 1024;
bool g_bWindowFullscreen = false;
// Declare OpenGL variables
GLuint g_uiVAO[3];
GLuint g_uiVBO[3];
GLuint g_uiIBO[3];
GLsizei g_iSphereElements;
GLuint g_uiMainProgram;
GLuint g_uiSecondProgram;
mat4 g_m4Transform[5];
GLuint g_uiTransformUBO[5];
struct LocalCameraData
{
    float m_fAngleX;
    float m_fAngleY;
    float m_fMoveZ;
    float m_fMoveX;
    vec3 m_v3Position;
    vec3 m_v3Direction;
    vec3 m_v3Right;
    float m_fFOV;
    float m_fAspect;
    float m_fNear;
    float m_fFar;
};
LocalCameraData g_CameraData;
struct CameraData
{
    aligned_mat4 m_m4ViewProjection;
    aligned_vec3 m_v3Position;
};
GLuint g_uiCameraUBO;
struct PointLightData
{
    aligned_vec3 m_v3Position;
    aligned_vec3 m_v3Colour;
    aligned_vec3 m_v3Falloff;
};
GLuint g_uiPointLightUBO;
struct MaterialData
{
    aligned_vec3 v3DiffuseColour;
    aligned_vec3 v3SpecularColour;
    float        fRoughness;
};
GLuint g_uiMaterialUBO[5];

bool GL_Init()
{
    // Initialize GLEW
    glewExperimental = GL_TRUE; // Allow experimental or pre-release drivers to return all supported extensions
    GLenum GlewError = glewInit();
    if (GlewError != GLEW_OK) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize GLEW: %s\n", glewGetErrorString(GlewError));
        return false;
    }

#ifdef _DEBUG
    // Initialise debug callback
    GLDebug_Init();
#endif

    // Set up initial GL attributes
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f); // Set the cleared back buffer to blue
    glCullFace(GL_BACK);                  // Set back-face culling
    glEnable(GL_CULL_FACE);               // Enable use of back/front face culling
    glEnable(GL_DEPTH_TEST);              // Enable use of depth testing
    glDisable(GL_STENCIL_TEST);           // Disable stencil test for speed

    // Create vertex shader
    GLuint uiVertexShader;
    if (!GL_LoadShaderFile(uiVertexShader, GL_VERTEX_SHADER, "Tutorial3Vert.glsl", 100))
        return false;

    // Create fragment shader
    GLuint uiFragmentShader;
    if (!GL_LoadShaderFile(uiFragmentShader, GL_FRAGMENT_SHADER, "Tutorial3Frag.glsl", 200))
        return false;

    // Create program
    if (!GL_LoadShaders(g_uiMainProgram, uiVertexShader, uiFragmentShader))
        return false;

    // Clean up unneeded shaders
    glDeleteShader(uiVertexShader);
    glDeleteShader(uiFragmentShader);

    // Create the per-fragment lighting program
    if (!GL_LoadShaderFile(uiVertexShader, GL_VERTEX_SHADER, "Tutorial3Vert2.glsl", 300))
        return false;
    if (!GL_LoadShaderFile(uiFragmentShader, GL_FRAGMENT_SHADER, "Tutorial3Frag2.glsl", 400))
        return false;
    if (!GL_LoadShaders(g_uiSecondProgram, uiVertexShader, uiFragmentShader))
        return false;
    glDeleteShader(uiVertexShader);
    glDeleteShader(uiFragmentShader);

    // Specify program to use
    glUseProgram(g_uiMainProgram);

    // Create Vertex Array Objects
    glGenVertexArrays(3, &g_uiVAO[0]);

    // Create VBO and IBOs
    glGenBuffers(3, &g_uiVBO[0]);
    glGenBuffers(3, &g_uiIBO[0]);

    // Bind the Cube VAO
    glBindVertexArray(g_uiVAO[0]);

    // Create Cube VBO and IBO data
    GL_GenerateCube(g_uiVBO[0], g_uiIBO[0]);

    // Bind the Sphere VAO
    glBindVertexArray(g_uiVAO[1]);

    // Create Sphere VBO and IBO data
    g_iSphereElements = GL_GenerateSphere(12, 6, g_uiVBO[1], g_uiIBO[1]);

    // Bind the Pyramid VAO
    glBindVertexArray(g_uiVAO[2]);

    // Create Pyramid VBO and IBO data
    GL_GeneratePyramid(g_uiVBO[2], g_uiIBO[2]);

    // Create initial model transforms
    g_m4Transform[0] = mat4(1.0f); //Identity matrix
    g_m4Transform[1] = translate(mat4(1.0f), vec3(-3.0f, 0.0f, 0.0f));
    g_m4Transform[2] = translate(mat4(1.0f), vec3(0.0f, -3.0f, 0.0f));
    g_m4Transform[3] = translate(mat4(1.0f), vec3(3.0f, 0.0f, 0.0f));
    g_m4Transform[4] = translate(mat4(1.0f), vec3(0.0f, 3.0f, 0.0f));

    // Create transform UBOs
    glGenBuffers(5, &g_uiTransformUBO[0]);

    // Initialise the transform buffers
    for (int i = 0; i < 5; i++) {
        glBindBuffer(GL_UNIFORM_BUFFER, g_uiTransformUBO[i]);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4), &g_m4Transform[i], GL_STATIC_DRAW);
    }

    // Initialise camera data
    g_CameraData.m_fAngleX = (float)M_PI;
    g_CameraData.m_fAngleY = 0.0f;
    g_CameraData.m_fMoveZ = 0.0f;
    g_CameraData.m_fMoveX = 0.0f;
    g_CameraData.m_v3Position = vec3(0.0f, 0.0f, 12.0f);
    g_CameraData.m_v3Direction = vec3(0.0f, 0.0f, -1.0f);
    g_CameraData.m_v3Right = vec3(1.0f, 0.0f, 0.0f);

    // Initialise camera projection values
    g_CameraData.m_fFOV = radians(45.0f);
    g_CameraData.m_fAspect = (float)g_iWindowWidth / (float)g_iWindowHeight;
    g_CameraData.m_fNear = 0.1f;
    g_CameraData.m_fFar = 100.0f;

    // Create camera UBO
    glGenBuffers(1, &g_uiCameraUBO);

    // Bind camera UBO
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, g_uiCameraUBO);

    // Create light UBO
    glGenBuffers(1, &g_uiPointLightUBO);
    //PointLightData Light = {
    //    vec3(6.0f, 6.0f, 6.0f),      // Light position
    //    vec3(1.0f, 1.0f, 1.0f),      // Light colour
    //    0.01f };
    PointLightData Light[3] = {{
        vec3(6.0f, 6.0f, 6.0f),      // Light position
        vec3(1.0f, 0.0f, 0.0f),      // Light colour
        vec3(0.0f, 0.0f, 0.01f) },   // Light falloff
        { vec3(-6.0f, 6.0f, 6.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 0.01f) },
        { vec3(0.0f, 6.0f, 0.0f), vec3(0.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.01f) }};
    glBindBuffer(GL_UNIFORM_BUFFER, g_uiPointLightUBO);
    //glBufferData(GL_UNIFORM_BUFFER, sizeof(PointLightData), &Light, GL_STATIC_DRAW);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(PointLightData) * 3, &Light, GL_STATIC_DRAW);

    // Bind Light UBO
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, g_uiPointLightUBO);

    // Set number of lights
    glProgramUniform1i(g_uiMainProgram, 0, 3);
    glProgramUniform1i(g_uiSecondProgram, 0, 3);

    // Create material UBOs
    glGenBuffers(5, &g_uiMaterialUBO[0]);
    MaterialData Material = {
        vec3(1.0f, 0.0f, 0.0f),    // Diffuse colour
        vec3(1.0f, 0.3f, 0.3f),    // Specular colour
        0.35f};                    // Roughness
    glBindBuffer(GL_UNIFORM_BUFFER, g_uiMaterialUBO[0]);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialData), &Material, GL_STATIC_DRAW);
    Material = {vec3(0.0f, 1.0f, 0.0f), vec3(0.3f, 1.0f, 0.3f), 0.45f};
    glBindBuffer(GL_UNIFORM_BUFFER, g_uiMaterialUBO[1]);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialData), &Material, GL_STATIC_DRAW);
    Material = {vec3(1.0f, 0.0f, 1.0f), vec3(1.3f, 0.3f, 1.0f), 0.30f};
    glBindBuffer(GL_UNIFORM_BUFFER, g_uiMaterialUBO[2]);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialData), &Material, GL_STATIC_DRAW);
    Material = {vec3(1.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 0.3f), 0.25f};
    glBindBuffer(GL_UNIFORM_BUFFER, g_uiMaterialUBO[3]);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialData), &Material, GL_STATIC_DRAW);
    Material = {vec3(0.0f, 1.0f, 1.0f), vec3(0.3f, 1.0f, 1.0f), 0.35f};
    glBindBuffer(GL_UNIFORM_BUFFER, g_uiMaterialUBO[4]);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialData), &Material, GL_STATIC_DRAW);

    // Set Mouse capture and hide cursor
    SDL_ShowCursor(0);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    return true;
}

void GL_Quit()
{
    // Release the shader programs
    glDeleteProgram(g_uiMainProgram);
    glDeleteProgram(g_uiSecondProgram);

    // Delete VBOs/IBOs and VAOs
    glDeleteBuffers(3, &g_uiVBO[0]);
    glDeleteBuffers(3, &g_uiIBO[0]);
    glDeleteVertexArrays(3, &g_uiVAO[0]);

    // Delete transform and camera UBOs
    glDeleteBuffers(5, &g_uiTransformUBO[0]);
    glDeleteBuffers(1, &g_uiCameraUBO);

    // Delete light and material buffers
    glDeleteBuffers(1, &g_uiPointLightUBO);
    glDeleteBuffers(5, &g_uiMaterialUBO[0]);
}

void GL_Render()
{
    // Clear the render output and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Specify Cube VAO
    glBindVertexArray(g_uiVAO[0]);

    // Bind the Transform UBO
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_uiTransformUBO[0]);

    // Bind material UBO
    glBindBufferBase(GL_UNIFORM_BUFFER, 3, g_uiMaterialUBO[0]);

    // Draw the Cube
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // Specify Sphere VAO
    glBindVertexArray(g_uiVAO[1]);

    // Render each sphere
    for (int i = 1; i < 3; i++) {
        // Bind the Transform UBO
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_uiTransformUBO[i]);

        // Bind material UBO
        glBindBufferBase(GL_UNIFORM_BUFFER, 3, g_uiMaterialUBO[i]);

        // Draw the Sphere
        glDrawElements(GL_TRIANGLES, g_iSphereElements, GL_UNSIGNED_INT, 0);
    }

    // Specify Pyramid VAO
    glBindVertexArray(g_uiVAO[2]);

    // Render each pyramid
    for (int i = 3; i < 5; i++) {
        // Bind the Transform UBO
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_uiTransformUBO[i]);

        // Bind material UBO
        glBindBufferBase(GL_UNIFORM_BUFFER, 3, g_uiMaterialUBO[i]);

        // Draw the Pyramid
        glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);
    }
}

void GL_Update(float fElapsedTime)
{
    // Update the cameras position
    g_CameraData.m_v3Position += g_CameraData.m_fMoveZ * fElapsedTime * g_CameraData.m_v3Direction;
    g_CameraData.m_v3Position += g_CameraData.m_fMoveX * fElapsedTime * g_CameraData.m_v3Right;

    // Determine rotation matrix for camera angles
    mat4 m4Rotation = rotate(mat4(1.0f), g_CameraData.m_fAngleX, vec3(0.0f, 1.0f, 0.0f));
    m4Rotation = rotate(m4Rotation, g_CameraData.m_fAngleY, vec3(1.0f, 0.0f, 0.0f));

    // Determine new view and right vectors
    g_CameraData.m_v3Direction = mat3(m4Rotation) * vec3(0.0f, 0.0f, 1.0f);
    g_CameraData.m_v3Right = mat3(m4Rotation) * vec3(-1.0f, 0.0f, 0.0f);

    // Create updated camera View matrix
    mat4 m4View = lookAt(
        g_CameraData.m_v3Position,
        g_CameraData.m_v3Position + g_CameraData.m_v3Direction,
        cross(g_CameraData.m_v3Right, g_CameraData.m_v3Direction)
        );

    // Create updated camera projection matrix
    mat4 m4Projection = perspective(
        g_CameraData.m_fFOV,
        g_CameraData.m_fAspect,
        g_CameraData.m_fNear, g_CameraData.m_fFar
        );

    // Create updated ViewProjection matrix
    mat4 m4ViewProjection = m4Projection * m4View;

    // Create updated camera data
    CameraData Camera = {
        m4ViewProjection,
        g_CameraData.m_v3Position
    };

    // Update the camera buffer
    glBindBuffer(GL_UNIFORM_BUFFER, g_uiCameraUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), &Camera, GL_DYNAMIC_DRAW);

    // Rotate each objects transform
    for (int i = 0; i < 5; i++) {
        // Update the transform
        g_m4Transform[i] = rotate(mat4(1.0f), 0.3f * fElapsedTime, vec3(0.0f, 0.0f, 1.0f)) * g_m4Transform[i];

        // Update the transforms UBO
        glBindBuffer(GL_UNIFORM_BUFFER, g_uiTransformUBO[i]);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4), &g_m4Transform[i], GL_STREAM_DRAW);
    }
}

#ifdef _WIN32
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int)
#else
int main(int argc, char **argv)
#endif
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    // Use OpenGL 4.3 core profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef _DEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    // Turn on double buffering with a 24bit Z buffer
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    //Get desktop resolution
    SDL_DisplayMode CurrentDisplay;
    g_bWindowFullscreen &= (SDL_GetCurrentDisplayMode(0, &CurrentDisplay) == 0);

    // Update screen resolution
    g_iWindowWidth = (g_bWindowFullscreen) ? CurrentDisplay.w : g_iWindowWidth;
    g_iWindowHeight = (g_bWindowFullscreen) ? CurrentDisplay.h : g_iWindowHeight;

    // Create a SDL window
    SDL_Window * Window = SDL_CreateWindow("AGT Tutorial", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                           g_iWindowWidth, g_iWindowHeight,
                                           SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | (g_bWindowFullscreen * SDL_WINDOW_FULLSCREEN));
    if (Window == NULL) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to create OpenGL window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create a OpenGL Context
    SDL_GLContext Context = SDL_GL_CreateContext(Window);
    if (Context == NULL) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to create OpenGL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    // Enable VSync with the OpenGL context
    SDL_GL_SetSwapInterval(-1);

    //Initialize OpenGL
    if (GL_Init()) {
        // Initialise elapsed time
        Uint32 uiOldTime, uiCurrentTime;
        uiCurrentTime = SDL_GetTicks();
        // Start the program message pump
        SDL_Event Event;
        bool bQuit = false;
        while (!bQuit) {
            // Update elapsed frame time
            uiOldTime = uiCurrentTime;
            uiCurrentTime = SDL_GetTicks();
            float fElapsedTime = (float)(uiCurrentTime - uiOldTime) / 1000.0f;

            // Poll SDL for buffered events
            while (SDL_PollEvent(&Event)) {
                if (Event.type == SDL_QUIT)
                    bQuit = true;
                else if ((Event.type == SDL_KEYDOWN) && (Event.key.repeat == 0)) {
                    if (Event.key.keysym.sym == SDLK_ESCAPE)
                        bQuit = true;
                    // Update camera movement vector
                    else if (Event.key.keysym.sym == SDLK_w)
                        g_CameraData.m_fMoveZ += 2.0f;
                    else if (Event.key.keysym.sym == SDLK_a)
                        g_CameraData.m_fMoveX -= 2.0f;
                    else if (Event.key.keysym.sym == SDLK_s)
                        g_CameraData.m_fMoveZ -= 2.0f;
                    else if (Event.key.keysym.sym == SDLK_d)
                        g_CameraData.m_fMoveX += 2.0f;
                    // Swap per-vertex and per-fragment shading
                    else if (Event.key.keysym.sym == SDLK_1)
                        glUseProgram(g_uiMainProgram);
                    // Swap subroutines
                    else if (Event.key.keysym.sym == SDLK_2) {
                        glUseProgram(g_uiSecondProgram);
                        GLuint g_uiSubRoutines[] = {0};
                        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &g_uiSubRoutines[0]);
                    } else if (Event.key.keysym.sym == SDLK_3) {
                        glUseProgram(g_uiSecondProgram);
                        GLuint g_uiSubRoutines[] = {1};
                        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &g_uiSubRoutines[0]);
                    }
                } else if ((Event.type == SDL_KEYUP)) {
                    // Reset camera movement vector
                    if (Event.key.keysym.sym == SDLK_w)
                        g_CameraData.m_fMoveZ -= 2.0f;
                    else if (Event.key.keysym.sym == SDLK_a)
                        g_CameraData.m_fMoveX += 2.0f;
                    else if (Event.key.keysym.sym == SDLK_s)
                        g_CameraData.m_fMoveZ += 2.0f;
                    else if (Event.key.keysym.sym == SDLK_d)
                        g_CameraData.m_fMoveX -= 2.0f;
                } else if (Event.type == SDL_MOUSEMOTION) {
                    // Update camera view angles
                    g_CameraData.m_fAngleX += -0.05f * fElapsedTime * Event.motion.xrel;
                    // Y Coordinates are in screen space so don't get negated
                    g_CameraData.m_fAngleY += 0.05f * fElapsedTime * Event.motion.yrel;
                    // Clip total pitch range to prevent gimbal lock
                    if (g_CameraData.m_fAngleY <= -((float)M_PI_2 * 0.7f))
                        g_CameraData.m_fAngleY = -((float)M_PI_2 * 0.7f);
                    if (g_CameraData.m_fAngleY >= ((float)M_PI_2 * 0.7f))
                        g_CameraData.m_fAngleY = ((float)M_PI_2 * 0.7f);
                } else if (Event.type == SDL_MOUSEWHEEL) {
                    // Update the cameras field of view
                    g_CameraData.m_fFOV += -0.5f * fElapsedTime * Event.motion.x;
                    // Clip FOV to prevent extreme values
                    if (g_CameraData.m_fFOV <= ((float)M_PI_2 * 0.1f))
                        g_CameraData.m_fFOV = ((float)M_PI_2 * 0.1f);
                    if (g_CameraData.m_fFOV >= ((float)M_PI * 0.9f))
                        g_CameraData.m_fFOV = ((float)M_PI * 0.9f);
                }
            }

            // Update the Scene
            GL_Update(fElapsedTime);

            // Render the scene
            GL_Render();

            // Swap the back-buffer and present the render
            SDL_GL_SwapWindow(Window);
        }

        // Delete any created GL resources
        GL_Quit();
    }

    // Delete the OpenGL context, SDL window and shutdown SDL
    SDL_GL_DeleteContext(Context);
    SDL_DestroyWindow(Window);
    SDL_Quit();

    return 0;
}
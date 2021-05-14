// Using SDL, GLEW
#include <math.h>
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

// Declare window variables
int g_iWindowWidth = 1280;
int g_iWindowHeight = 1024;
bool g_bWindowFullscreen = false;
// Declare OpenGL variables
GLuint g_uiVAO;
GLuint g_uiVBO;
GLuint g_uiMainProgram;

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
    //glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set the cleared back buffer to black
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f); // Set the cleared back buffer to blue
    glCullFace(GL_BACK);                  // Set back-face culling
    glEnable(GL_CULL_FACE);               // Enable use of back/front face culling
    glEnable(GL_DEPTH_TEST);              // Enable use of depth testing
    glDisable(GL_STENCIL_TEST);           // Disable stencil test for speed

    // Create vertex shader source
    //const GLchar p_cVertexShaderSource[] = {
    //    "#version 430 core\n \
    //    layout(location = 0) in vec2 v2VertexPos2D;\n \
    //    void main() \n \
    //    {\n \
    //        gl_Position = vec4(v2VertexPos2D, 0.0f, 1.0f);\n \
    //    }"
    //};
    //
    //// Create vertex shader
    //GLuint uiVertexShader;
    //if(!GL_LoadShader(uiVertexShader, GL_VERTEX_SHADER, p_cVertexShaderSource))
    //    return false;

    // Create vertex shader
    GLuint uiVertexShader;
    if (!GL_LoadShaderFile(uiVertexShader, GL_VERTEX_SHADER, "Tutorial1Vert.glsl", 100))
        return false;

    // Create fragment shader source
    //const GLchar p_cFragmentShaderSource[] = {
    //    "#version 430 core\n \
    //    out vec3 v3FragOutput;\n \
    //    void main() \n \
    //    {\n \
    //        //v3FragOutput = vec3(1.0f, 1.0f, 1.0f);\n \
    //        v3FragOutput = vec3(1.0f, 1.0f, 0.0f);\n \
    //    }"
    //};
    //
    //// Create fragment shader
    //GLuint uiFragmentShader;
    //if(!GL_LoadShader(uiFragmentShader, GL_FRAGMENT_SHADER, p_cFragmentShaderSource))
    //    return false;

    // Create fragment shader
    GLuint uiFragmentShader;
    if (!GL_LoadShaderFile(uiFragmentShader, GL_FRAGMENT_SHADER, "Tutorial1Frag.glsl", 200))
        return false;

    // Create program
    if (!GL_LoadShaders(g_uiMainProgram, uiVertexShader, uiFragmentShader))
        return false;

    // Clean up unneeded shaders
    glDeleteShader(uiVertexShader);
    glDeleteShader(uiFragmentShader);

    // Create a Vertex Array Object
    glGenVertexArrays(1, &g_uiVAO);
    glBindVertexArray(g_uiVAO);

    // Create VBO data
    GLfloat fVertexData[] =
    {
        -0.5f, 0.5f,
         0.0f, -0.5f,
         0.5f, 0.5f
    };

    // Create Vertex Buffer Object
    glGenBuffers(1, &g_uiVBO);
    glBindBuffer(GL_ARRAY_BUFFER, g_uiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fVertexData), fVertexData, GL_STATIC_DRAW);

    // Specify location of data within buffer
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const GLvoid *)0);
    glEnableVertexAttribArray(0);

    // Specify program to use
    glUseProgram(g_uiMainProgram);

    return true;
}

void GL_Quit()
{
    // Release the shader program
    glDeleteProgram(g_uiMainProgram);

    // Delete VBO and VAO
    glDeleteBuffers(1, &g_uiVBO);
    glDeleteVertexArrays(1, &g_uiVAO);
}

void GL_Render()
{
    // Clear the render output and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Specify VAO to use
    glBindVertexArray(g_uiVAO);

    // Draw the triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);
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
        // Start the program message pump
        SDL_Event Event;
        bool bQuit = false;
        while (!bQuit) {
            // Poll SDL for buffered events
            while (SDL_PollEvent(&Event)) {
                if (Event.type == SDL_QUIT)
                    bQuit = true;
                else if (Event.type == SDL_KEYDOWN) {
                    if (Event.key.keysym.sym == SDLK_ESCAPE)
                        bQuit = true;
                }
            }

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
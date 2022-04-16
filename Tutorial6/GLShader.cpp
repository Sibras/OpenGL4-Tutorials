// Using SDL, GLEW
#include <math.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

bool GL_LoadShader(GLuint & uiShader, GLenum ShaderType, const GLchar * p_cShader)
{
    // Build and link the shader program
    uiShader = glCreateShader(ShaderType);
    glShaderSource(uiShader, 1, &p_cShader, NULL);
    glCompileShader(uiShader);

    // Check for errors
    GLint iTestReturn;
    glGetShaderiv(uiShader, GL_COMPILE_STATUS, &iTestReturn);
    if (iTestReturn == GL_FALSE) {
        GLchar p_cInfoLog[1024];
        int32_t iErrorLength;
        glGetShaderInfoLog(uiShader, 1024, &iErrorLength, p_cInfoLog);
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to compile shader: %s\n", p_cInfoLog);
        glDeleteShader(uiShader);
        return false;
    }
    return true;
}

bool GL_LoadShaders(GLuint & uiShader, GLuint uiVertexShader, GLuint uiFragmentShader, GLuint uiGeometryShader)
{
    // Link the shaders
    uiShader = glCreateProgram();
    glAttachShader(uiShader, uiVertexShader);
    glAttachShader(uiShader, uiFragmentShader);
    if (uiGeometryShader != -1) {
        glAttachShader(uiShader, uiGeometryShader);
    }
    glLinkProgram(uiShader);

    //Check for error in link
    GLint iTestReturn;
    glGetProgramiv(uiShader, GL_LINK_STATUS, &iTestReturn);
    if (iTestReturn == GL_FALSE) {
        GLchar p_cInfoLog[1024];
        int32_t iErrorLength;
        glGetShaderInfoLog(uiShader, 1024, &iErrorLength, p_cInfoLog);
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to link shaders: %s\n", p_cInfoLog);
        glDeleteProgram(uiShader);
        return false;
    }
    return true;
}

bool GL_LoadShaderFile(GLuint & uiShader, GLenum ShaderType, const char* p_cFileName, int iFileID)
{
#ifdef _WIN32
    // Can load directly from windows resource file
    HINSTANCE hInst = GetModuleHandle(NULL);
    HRSRC hRes = FindResource(hInst, MAKEINTRESOURCE(iFileID), RT_RCDATA);
    HGLOBAL hMem = LoadResource(hInst, hRes);
    DWORD size = SizeofResource(hInst, hRes);
    char* resText = (char*)LockResource(hMem);

    // Load in the shader
    bool bReturn = GL_LoadShader(uiShader, ShaderType, resText);

    // Print the shader name to assist debugging
    if (!bReturn)
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, " Failed shader: %s\n", p_cFileName);

    // Close Resource
    FreeResource(hMem);

    return bReturn;
#else
    // Load directly from the specified file
    SDL_RWops* p_File = SDL_RWFromFile(p_cFileName, "r");
    if (p_File == NULL) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to open shader file: %s\n", p_cFileName);
        return false;
    }

    // Allocate a string to store contents
    size_t lFileSize = (size_t)SDL_RWsize(p_File);
    GLchar * p_cFileContents = (GLchar *)malloc(lFileSize + 1);
    SDL_RWread(p_File, p_cFileContents, lFileSize, 1);
    p_cFileContents[lFileSize] = '\0'; // Add terminating character

    // Load in the shader
    bool bReturn = GL_LoadShader(uiShader, ShaderType, p_cFileContents);

    // Print the shader name to assist debugging
    if (!bReturn)
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, " Failed shader: %s\n", p_cFileName);

    // Close file
    SDL_RWclose(p_File);

    return bReturn;
#endif
}
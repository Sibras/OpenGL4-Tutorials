// Using SDL, GLEW
#include <math.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

static char * gp_cSeverity[] = {"High", "Medium", "Low", "Notification"};
static char * gp_cType[] = {"Error", "Deprecated", "Undefined", "Portability", "Performance", "Other"};
static char * gp_cSource[] = {"OpenGL", "OS", "GLSL Compiler", "3rd Party", "Application", "Other"};

void APIENTRY DebugCallback(uint32_t uiSource, uint32_t uiType, uint32_t uiID, uint32_t uiSeverity, int32_t iLength, const char * p_cMessage, void* p_UserParam)
{
    // Get the severity
    uint32_t uiSevID = 3;
    switch (uiSeverity) {
        case GL_DEBUG_SEVERITY_HIGH:
            uiSevID = 0; break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            uiSevID = 1; break;
        case GL_DEBUG_SEVERITY_LOW:
            uiSevID = 2; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
        default:
            uiSevID = 3; break;
    }

    // Get the type
    uint32_t uiTypeID = 5;
    switch (uiType) {
        case GL_DEBUG_TYPE_ERROR:
            uiTypeID = 0; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            uiTypeID = 1; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            uiTypeID = 2; break;
        case GL_DEBUG_TYPE_PORTABILITY:
            uiTypeID = 3; break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            uiTypeID = 4; break;
        case GL_DEBUG_TYPE_OTHER:
        default:
            uiTypeID = 5; break;
    }

    // Get the source
    uint32_t uiSourceID = 5;
    switch (uiSource) {
        case GL_DEBUG_SOURCE_API:
            uiSourceID = 0; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            uiSourceID = 1; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            uiSourceID = 2; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            uiSourceID = 3; break;
        case GL_DEBUG_SOURCE_APPLICATION:
            uiSourceID = 4; break;
        case GL_DEBUG_SOURCE_OTHER:
        default:
            uiSourceID = 5; break;
    }

    // Output to the Log
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "OpenGL Debug: Severity=%s, Type=%s, Source=%s - %s",
                    gp_cSeverity[uiSevID], gp_cType[uiTypeID], gp_cSource[uiSourceID], p_cMessage);
    if (uiSeverity == GL_DEBUG_SEVERITY_HIGH) {
        //This a serious error so we need to shutdown the program
        SDL_Event event;
        event.type = SDL_QUIT;
        SDL_PushEvent(&event);
    }
}

void GLDebug_Init()
{
    //Allow for synchronous callbacks.
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    //Set up the debug info callback
    glDebugMessageCallback((GLDEBUGPROC)&DebugCallback, NULL);

    //Set up the type of debug information we want to receive
    uint32_t uiUnusedIDs = 0;
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &uiUnusedIDs, GL_TRUE); //Enable all
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE); //Disable notifications
    //glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, NULL, GL_FALSE); //Disable low severity
    //glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, NULL, GL_FALSE); //Disable medium severity warnings
}
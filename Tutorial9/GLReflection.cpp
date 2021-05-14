// Using GLM and math headers
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GLScene.h> //Need '<' to enforce loading of project local header

//Main.cpp
extern int g_iWindowWidth;
extern int g_iWindowHeight;
extern GLuint g_uiDeferredProgram2;
extern SceneData g_SceneData;
extern void GL_RenderObjects(ObjectData * p_Object = NULL);
//Deferred.cpp
extern void GL_RenderDeferred(ObjectData * p_Object = NULL, GLuint uiAccumBuffer = 0, GLenum uiTextureTarget = GL_TEXTURE_2D);
extern void GL_RenderPostProcess();

GLuint g_uiReflectCameraUBO;

struct CameraData
{
    mat4 m_m4ViewProjection;
    vec3 m_v3Position;
    mat4 m_m4InvViewProjection;
};

void GL_RenderPlanarReflection(ReflectObjectData * p_RObject, ObjectData * p_Object, const vec3 & v3Direction, const vec3 & v3Up,
                               const vec3 & v3Position, float fFOV, float fAspect, const vec2 & v2NearFar)
{
    // Transform plane to world space
    vec3 v3PlanePosition = vec3(p_RObject->m_v4PlaneOrPosition) * p_RObject->m_v4PlaneOrPosition.w;
    v3PlanePosition = vec3(p_Object->m_4Transform * vec4(v3PlanePosition, 1.0f));
    vec4 v4Plane = p_Object->m_4Transform * p_RObject->m_v4PlaneOrPosition;
    v4Plane.w = dot(vec3(v4Plane), -v3PlanePosition);

    // Calculate reflection view position and direction
    vec3 v3ReflectView = reflect(v3Direction, vec3(v4Plane));
    vec3 v3ReflectUp = reflect(v3Up, vec3(v4Plane));
    float fDistanceToPlane = (dot(v3Position, vec3(v4Plane)) + v4Plane.w)
        / length(vec3(v4Plane));
    vec3 v3ReflectPosition = v3Position - (2.0f * fDistanceToPlane * vec3(v4Plane));

    // Calculate reflection view and projection matrix
    mat4 m4ReflectView = lookAt(v3ReflectPosition,
                                v3ReflectPosition + v3ReflectView,
                                v3ReflectUp);
    mat4 m4ReflectProj = perspective(
        fFOV,
        fAspect,
        v2NearFar.x, v2NearFar.y * 2.0f
    );

    // Calculate the oblique view frustum
    vec4 v4ClipPlane = transpose(inverse(m4ReflectView)) * v4Plane;
    vec4 v4Oblique = vec4((sign(v4ClipPlane.x) + m4ReflectProj[2][0]) / m4ReflectProj[0][0],
        (sign(v4ClipPlane.y) + m4ReflectProj[2][1]) / m4ReflectProj[1][1],
                          -1.0f,
                          (1.0f + m4ReflectProj[2][2]) / m4ReflectProj[3][2]);

    // Calculate the scaled plane vector
    v4Oblique = v4ClipPlane * (2.0f / dot(v4ClipPlane, v4Oblique));

    // Replace the third row of the projection matrix
    m4ReflectProj[0][2] = v4Oblique.x;
    m4ReflectProj[1][2] = v4Oblique.y;
    m4ReflectProj[2][2] = v4Oblique.z + 1.0f;
    m4ReflectProj[3][2] = v4Oblique.w;

    // Create updated camera data
    mat4 m4ViewProjection = m4ReflectProj * m4ReflectView;
    mat4 m4InvViewProjection = inverse(m4ViewProjection);
    CameraData Camera = {
        m4ViewProjection,
        v3ReflectPosition,
        m4InvViewProjection};

    // Update the camera buffer
    glBindBuffer(GL_UNIFORM_BUFFER, g_uiReflectCameraUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), &Camera, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, g_uiReflectCameraUBO);

    // Update the objects projection UBO
    glBindBuffer(GL_UNIFORM_BUFFER, p_Object->m_uiReflectVPUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4), &Camera.m_m4ViewProjection, GL_STATIC_DRAW);

    // Perform deferred render pass
    GL_RenderDeferred(p_Object, p_Object->m_uiReflect);

    // Generate mipmaps for texture
    glBindTexture(GL_TEXTURE_2D, p_Object->m_uiReflect);
    glGenerateMipmap(GL_TEXTURE_2D);
}

// World space normals
const vec3 v3CubeNormals[] = {
    vec3(1.0f,  0.0f,  0.0f),     // positive x
    vec3(-1.0f,  0.0f,  0.0f),     // negative x
    vec3(0.0f,  1.0f,  0.0f),     // positive y
    vec3(0.0f, -1.0f,  0.0f),     // negative y
    vec3(0.0f,  0.0f,  1.0f),     // positive z
    vec3(0.0f,  0.0f, -1.0f),     // negative z
};

// World space up directions
const vec3 v3CubeUps[] = {
    vec3(0.0f, -1.0f,  0.0f),     // positive x
    vec3(0.0f, -1.0f,  0.0f),     // negative x
    vec3(0.0f,  0.0f,  1.0f),     // positive y
    vec3(0.0f,  0.0f, -1.0f),     // negative y
    vec3(0.0f, -1.0f,  0.0f),     // positive z
    vec3(0.0f, -1.0f,  0.0f),     // negative z
};

void GL_CalculateCubeMapVP(const vec3 & v3Position, mat4 * p_m4CubeViewProjections, float fNear, float fFar)
{
    // Calculate view matrices
    mat4 m4CubeViews[6];
    for (unsigned i = 0; i < 6; i++) {
        m4CubeViews[i] = lookAt(v3Position,
                                v3Position + v3CubeNormals[i],
                                v3CubeUps[i]);
    }

    // Calculate projection matrix
    mat4 m4CubeProjection = perspective(
        radians(90.0f),
        1.0f,
        fNear, fFar
    );

    // Calculate combined view projection matrices
    for (unsigned i = 0; i < 6; i++) {
        p_m4CubeViewProjections[i] = m4CubeProjection * m4CubeViews[i];
    }
}

void GL_RenderEnvironmentReflection(ReflectObjectData * p_RObject, ObjectData * p_Object)
{
    // Calculate position in world space
    vec3 v3Position = vec3(p_Object->m_4Transform * p_RObject->m_v4PlaneOrPosition);

    // Calculate cube map VPs
    mat4 m4CubeViewProjections[6];
    GL_CalculateCubeMapVP(v3Position, m4CubeViewProjections, g_SceneData.m_LocalCamera.m_fNear, g_SceneData.m_LocalCamera.m_fFar);

    // Generate planar reflection maps so they will be visible in environment map
    for (unsigned i = 0; i < g_SceneData.m_uiNumReflecObjects; i++) {
        ReflectObjectData * p_RObject2 = &g_SceneData.mp_ReflecObjects[i];
        ObjectData * p_Object2 = &g_SceneData.mp_Objects[p_RObject2->m_uiObjectPos];

        // Check if planar or cube reflection
        if (p_Object2->m_uiReflective == 1) {
            // Only 1 face can be used to generate the planar reflection so find the closest one
            const vec3 v3Dir = normalize(vec3(p_Object2->m_4Transform[3]) - v3Position); //This should use the centre of the AABB instead of transform
            float fClosest = -1;
            unsigned uiClosestIndex;
            for (unsigned j = 0; j < 6; j++) {
                float fDot = dot(v3CubeNormals[j], v3Dir);
                if (fDot > fClosest) {
                    fClosest = fDot;
                    uiClosestIndex = j;
                }
            }
            GL_RenderPlanarReflection(p_RObject2, p_Object2, v3CubeNormals[uiClosestIndex], v3CubeUps[uiClosestIndex], v3Position, radians(90.0f),
                                      1.0f, vec2(g_SceneData.m_LocalCamera.m_fNear, g_SceneData.m_LocalCamera.m_fFar));
            // This could in theory be added into the below per-face loop however for performance reasons we will just generate a single reflection
        }
    }

    // Update the viewport
    glViewport(0, 0, g_iWindowHeight, g_iWindowHeight);

    // Bind the UBO buffer as camera
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, g_uiReflectCameraUBO);

    // Loop over each face in cube map
    for (int i = 0; i < 6; i++) {
        // Create updated camera data
        CameraData Camera = {
            m4CubeViewProjections[i],
            v3Position,
            inverse(m4CubeViewProjections[i])};

        // Update the objects projection UBO
        glBindBuffer(GL_UNIFORM_BUFFER, g_uiReflectCameraUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), &Camera, GL_STATIC_DRAW);

        // Perform deferred render pass
        GL_RenderDeferred(p_Object, p_Object->m_uiReflect, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
    }

    // Generate mipmaps for texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, p_Object->m_uiReflect);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // Reset to default viewport
    glViewport(0, 0, g_iWindowWidth, g_iWindowHeight);
}

bool GL_InitReflection()
{
    // Generate FBO camera data
    glGenBuffers(1, &g_uiReflectCameraUBO);

    // Pre-Generate environment reflection maps
    for (unsigned i = 0; i < g_SceneData.m_uiNumReflecObjects; i++) {
        ReflectObjectData * p_RObject = &g_SceneData.mp_ReflecObjects[i];
        ObjectData * p_Object = &g_SceneData.mp_Objects[p_RObject->m_uiObjectPos];

        // Check if planar or cube reflection
        if (p_Object->m_uiReflective == 2) {
            GL_RenderEnvironmentReflection(p_RObject, p_Object);
        }
    }
    return true;
}

void GL_QuitReflection()
{
    // Release single FBO data
    glDeleteBuffers(1, &g_uiReflectCameraUBO);
}
#ifndef _GLSCENE_H_
#define _GLSCENE_H_

// Using GLM, math headers and GLEW
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtx/type_aligned.hpp>
#include <GL/glew.h>

using namespace glm;

struct MeshData
{
    GLuint m_uiVAO;
    GLuint m_uiVBO;
    GLuint m_uiIBO;
    unsigned m_uiNumIndices;
};

struct MaterialData
{
    GLuint m_uiDiffuse;
    GLuint m_uiSpecular;
    GLuint m_uiRough;
    float m_fEmissive;
    bool m_bTransparent;
    bool m_bReflective;
};

struct ObjectData
{
    GLuint m_uiVAO;
    unsigned m_uiNumIndices;
    GLuint m_uiDiffuse;
    GLuint m_uiSpecular;
    GLuint m_uiRough;
    float m_fEmissive;
    bool m_bTransparent;
    mat4 m_4Transform;
    GLuint m_uiTransformUBO;
    unsigned m_uiReflective;
    GLuint m_uiReflect;
    GLuint m_uiReflectVPUBO;
};

struct ReflectObjectData
{
    unsigned m_uiObjectPos;
    // Reflection data
    //vec4 m_v4Plane;
    //vec4 m_v4Position;
    vec4 m_v4PlaneOrPosition;
};

struct PointLightData
{
    aligned_vec3 m_v3Position;
    aligned_vec3 m_v3Colour;
    aligned_vec3 m_v3Falloff;
};

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

struct SceneData
{
    MeshData * mp_Meshes;
    unsigned m_uiNumMeshes;
    MaterialData * mp_Materials;
    unsigned m_uiNumMaterials;
    ObjectData * mp_Objects;
    unsigned m_uiNumObjects;
    ObjectData * mp_TransObjects;
    unsigned m_uiNumTransObjects;
    ReflectObjectData * mp_ReflecObjects;
    unsigned m_uiNumReflecObjects;
    PointLightData * mp_PointLights;
    unsigned m_uiNumPointLights;
    GLuint m_uiPointLightUBO;
    LocalCameraData m_LocalCamera;
    GLuint m_uiCameraUBO;
};

bool GL_LoadScene(const char * p_cSceneFile, SceneData & SceneInfo);

void GL_UnloadScene(SceneData & SceneInfo);

#endif
#include "GLScene.h"
// Using SDL
#include <SDL2/SDL.h>
// Using Assimp
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

//Texture.cpp
extern bool GL_LoadTextureKTX(GLuint uiTexture, const char * p_cTextureFile);
extern bool GL_ConvertDDS2KTX(const char * p_cTextureFile);
//Main.cpp
extern int g_iWindowWidth;
extern int g_iWindowHeight;

struct CustomVertex
{
    vec3 v3Position;
    vec3 v3Normal;
    vec2 v2UV;
};

void GL_LoadSceneNode(aiNode * p_Node, const aiScene * p_Scene, SceneData & SceneInfo, const mat4 & m4Transform, bool bAddObjects)
{
    // Update current transform
    mat4 m4CurrentTransform = transpose(*(mat4 *)&p_Node->mTransformation) * m4Transform;
    // Loop over each mesh in the node
    for (unsigned i = 0; i < p_Node->mNumMeshes; i++) {
        // Load in each nodes mesh as an object
        if (bAddObjects) {
            ObjectData * p_Object = &SceneInfo.mp_Objects[SceneInfo.m_uiNumObjects];

            // Get data from corresponding mesh
            const MeshData * p_Mesh = &SceneInfo.mp_Meshes[p_Node->mMeshes[i]];
            p_Object->m_uiVAO = p_Mesh->m_uiVAO;
            p_Object->m_uiNumIndices = p_Mesh->m_uiNumIndices;

            // Get data from corresponding material
            const MaterialData * p_Material = &SceneInfo.mp_Materials[p_Scene->mMeshes[p_Node->mMeshes[i]]->mMaterialIndex];
            p_Object->m_uiDiffuse = p_Material->m_uiDiffuse;
            p_Object->m_uiSpecular = p_Material->m_uiSpecular;
            p_Object->m_uiRough = p_Material->m_uiRough;
            p_Object->m_fEmissive = p_Material->m_fEmissive;

            // Generate and fill transform UBO
            p_Object->m_4Transform = m4CurrentTransform;
            glGenBuffers(1, &p_Object->m_uiTransformUBO);
            glBindBuffer(GL_UNIFORM_BUFFER, p_Object->m_uiTransformUBO);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4), &p_Object->m_4Transform, GL_STATIC_DRAW);

            // Add to transparent object list as needed
            p_Object->m_bTransparent = p_Material->m_bTransparent;
            if (p_Material->m_bTransparent) {
                SceneInfo.mp_TransObjects[SceneInfo.m_uiNumTransObjects] = *p_Object;
                ++SceneInfo.m_uiNumTransObjects;
            }

            // Add to reflective object list as needed
            p_Object->m_uiReflective = 0;
            if (p_Material->m_bReflective) {
                // Calculate objects bounding box
                const aiMesh * p_AIMesh = p_Scene->mMeshes[p_Node->mMeshes[i]];
                vec3 v3AABBMin = vec3(FLT_MAX);
                vec3 v3AABBMax = vec3(-FLT_MAX);
                for (unsigned j = 0; j < p_AIMesh->mNumVertices; j++) {
                    vec3 v3Vert(p_AIMesh->mVertices[j].x,
                                p_AIMesh->mVertices[j].y,
                                p_AIMesh->mVertices[j].z);
                    v3AABBMin = min(v3AABBMin, v3Vert);
                    v3AABBMax = max(v3AABBMax, v3Vert);
                }

                // Check if planar or not
                vec3 v3AABBSize = v3AABBMax - v3AABBMin;
                if ((v3AABBSize.x < 0.00001f) || (v3AABBSize.y < 0.00001f) || (v3AABBSize.z < 0.00001f)) {
                    // Set object as planar reflective
                    p_Object->m_uiReflective = 1;
                    ReflectObjectData * p_RObject = &SceneInfo.mp_ReflecObjects[SceneInfo.m_uiNumReflecObjects];

                    // Set object position in reflective object
                    p_RObject->m_uiObjectPos = SceneInfo.m_uiNumObjects;

                    // Generate texture for reflection map
                    glGenTextures(1, &p_Object->m_uiReflect);
                    glBindTexture(GL_TEXTURE_2D, p_Object->m_uiReflect);
                    int iLevels = (int)ceilf(log2f((float)max(g_iWindowWidth, g_iWindowHeight)));
                    glTexStorage2D(GL_TEXTURE_2D, iLevels, GL_RGB8, g_iWindowWidth, g_iWindowHeight);

                    // Initialise the texture filtering and wrap values
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                    // Generate a UBO for reflected projection storage
                    glGenBuffers(1, &p_Object->m_uiReflectVPUBO);

                    // Calculate objects plane in model space
                    vec3 v3PlaneNormal = vec3(p_AIMesh->mNormals[0].x,
                                              p_AIMesh->mNormals[0].y,
                                              p_AIMesh->mNormals[0].z);
                    vec3 v3AABBCentre = v3AABBMin + (v3AABBSize * 0.5f);
                    //p_RObject->m_v4Plane = vec4(v3PlaneNormal, dot(v3PlaneNormal, -v3AABBCentre));
                    p_RObject->m_v4PlaneOrPosition = vec4(v3PlaneNormal, dot(v3PlaneNormal, -v3AABBCentre));

                    ++SceneInfo.m_uiNumReflecObjects;
                } else {
                    // Set object as cube reflective
                    p_Object->m_uiReflective = 2;
                    ReflectObjectData * p_RObject = &SceneInfo.mp_ReflecObjects[SceneInfo.m_uiNumReflecObjects];

                    // Set object position in reflective object
                    p_RObject->m_uiObjectPos = SceneInfo.m_uiNumObjects;

                    // Generate texture for reflection map
                    glGenTextures(1, &p_Object->m_uiReflect);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, p_Object->m_uiReflect);
                    int iLevels = (int)ceilf(log2f((float)g_iWindowHeight));
                    glTexStorage2D(GL_TEXTURE_CUBE_MAP, iLevels, GL_RGB8, g_iWindowHeight, g_iWindowHeight);

                    // Initialise the texture filtering and wrap values
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                    // Calculate AABB centre position in model space
                    //p_RObject->m_v4Position = vec4(v3AABBMin + (v3AABBSize * 0.5f), 1.0f);
                    p_RObject->m_v4PlaneOrPosition = vec4(v3AABBMin + (v3AABBSize * 0.5f), 1.0f);

                    ++SceneInfo.m_uiNumReflecObjects;
                }
            }
        }
        ++SceneInfo.m_uiNumObjects;
    }

    // Loop over each child node
    for (unsigned i = 0; i < p_Node->mNumChildren; i++) {
        GL_LoadSceneNode(p_Node->mChildren[i], p_Scene, SceneInfo, m4CurrentTransform, bAddObjects);
    }
}

bool GL_FindSceneNode(aiNode * p_Node, const aiString & Name, const aiScene * p_Scene, const mat4 & m4Transform, mat4 & m4RetTransform)
{
    // Update current transform
    mat4 m4CurrentTransform = transpose(*(mat4 *)&p_Node->mTransformation) * m4Transform;
    if (strcmp(p_Node->mName.data, Name.data) == 0) {
        m4RetTransform = m4CurrentTransform;
        return true;
    }

    // Loop over each child node
    for (unsigned i = 0; i < p_Node->mNumChildren; i++) {
        bool bRet = GL_FindSceneNode(p_Node->mChildren[i], Name, p_Scene, m4CurrentTransform, m4RetTransform);
        if (bRet) {
            return true;
        }
    }
    return false;
}

bool GL_LoadScene(const char * p_cSceneFile, SceneData & SceneInfo)
{
    // Load scene from file
    const aiScene * p_Scene = aiImportFile(p_cSceneFile,
                                           aiProcess_GenSmoothNormals |
                                           aiProcess_CalcTangentSpace |
                                           aiProcess_Triangulate |
                                           aiProcess_ImproveCacheLocality |
                                           aiProcess_SortByPType);

                                       // Check if import failed
    if (!p_Scene) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to open scene file: %s\n", aiGetErrorString());
        return false;
    }

    // Get import file base path
    char * p_cPath = (char *)malloc(255);
    *p_cPath = '\0';
    unsigned uiPathLength = 0;
    const char * p_cDirSlash = strrchr(p_cSceneFile, '/');
    if (p_cDirSlash != NULL) {
        uiPathLength = (unsigned)(p_cDirSlash - p_cSceneFile) + 1;
        strncat(p_cPath, p_cSceneFile, uiPathLength);
    }
    // Allocate buffers for each mesh
    SceneInfo.mp_Meshes = (MeshData *)realloc(SceneInfo.mp_Meshes, p_Scene->mNumMeshes * sizeof(MeshData));
    // Load in each mesh
    for (unsigned i = 0; i < p_Scene->mNumMeshes; i++) {
        MeshData * p_Mesh = &SceneInfo.mp_Meshes[i];
        const aiMesh * p_AIMesh = p_Scene->mMeshes[i];
        // Generate the buffers
        glGenVertexArrays(1, &p_Mesh->m_uiVAO);
        glGenBuffers(1, &p_Mesh->m_uiVBO);
        glGenBuffers(1, &p_Mesh->m_uiIBO);

        // Create the new mesh data
        p_Mesh->m_uiNumIndices = p_AIMesh->mNumFaces * 3;
        const unsigned uiSizeVertices = p_AIMesh->mNumVertices * sizeof(CustomVertex);
        const unsigned uiSizeIndices = p_Mesh->m_uiNumIndices * sizeof(GLuint);
        CustomVertex * p_VBuffer = (CustomVertex *)malloc(uiSizeVertices);
        GLuint * p_IBuffer = (GLuint *)malloc(uiSizeIndices);

        // Load in vertex data
        CustomVertex * p_vBuffer = p_VBuffer;
        for (unsigned j = 0; j < p_AIMesh->mNumVertices; j++) {
            p_vBuffer->v3Position = vec3(p_AIMesh->mVertices[j].x,
                                         p_AIMesh->mVertices[j].y,
                                         p_AIMesh->mVertices[j].z);
            p_vBuffer->v3Normal = vec3(p_AIMesh->mNormals[j].x,
                                       p_AIMesh->mNormals[j].y,
                                       p_AIMesh->mNormals[j].z);
            p_vBuffer->v2UV = vec2(p_AIMesh->mTextureCoords[0][j].x,
                                   p_AIMesh->mTextureCoords[0][j].y);
            ++p_vBuffer;
        }

        // Load in Indexes
        GLuint * p_iBuffer = p_IBuffer;
        for (unsigned j = 0; j < p_AIMesh->mNumFaces; j++) {
            *p_iBuffer++ = p_AIMesh->mFaces[j].mIndices[0];
            *p_iBuffer++ = p_AIMesh->mFaces[j].mIndices[1];
            *p_iBuffer++ = p_AIMesh->mFaces[j].mIndices[2];
        }

        //Bind the VAO
        glBindVertexArray(p_Mesh->m_uiVAO);

        // Fill Vertex Buffer Object
        glBindBuffer(GL_ARRAY_BUFFER, p_Mesh->m_uiVBO);
        glBufferData(GL_ARRAY_BUFFER, uiSizeVertices, p_VBuffer, GL_STATIC_DRAW);

        // Fill Index Buffer Object
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p_Mesh->m_uiIBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, uiSizeIndices, p_IBuffer, GL_STATIC_DRAW);

        // Cleanup allocated data
        free(p_VBuffer);
        free(p_IBuffer);

        // Specify location of data within buffer
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CustomVertex), (const GLvoid *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CustomVertex), (const GLvoid *)offsetof(CustomVertex, v3Normal));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(CustomVertex), (const GLvoid *)offsetof(CustomVertex, v2UV));
        glEnableVertexAttribArray(2);
        ++SceneInfo.m_uiNumMeshes;
    }

    // Allocate buffers for each material
    SceneInfo.mp_Materials = (MaterialData *)realloc(SceneInfo.mp_Materials, p_Scene->mNumMaterials * sizeof(MaterialData));
    // Load in each material
    for (unsigned i = 0; i < p_Scene->mNumMaterials; i++) {
        MaterialData * p_Material = &SceneInfo.mp_Materials[i];
        // Generate the buffers
        glGenTextures(3, &p_Material->m_uiDiffuse);

        // Get each texture from scene and load
        aiString sTexture;
        aiGetMaterialTexture(p_Scene->mMaterials[i], aiTextureType_DIFFUSE, 0, &sTexture);
        strcpy(&p_cPath[uiPathLength], sTexture.data);// Add scene path to filename
        GL_LoadTextureKTX(p_Material->m_uiDiffuse, p_cPath);
        aiGetMaterialTexture(p_Scene->mMaterials[i], aiTextureType_SPECULAR, 0, &sTexture);
        strcpy(&p_cPath[uiPathLength], sTexture.data);
        GL_LoadTextureKTX(p_Material->m_uiSpecular, p_cPath);
        aiGetMaterialTexture(p_Scene->mMaterials[i], aiTextureType_SHININESS, 0, &sTexture);
        strcpy(&p_cPath[uiPathLength], sTexture.data);
        GL_LoadTextureKTX(p_Material->m_uiRough, p_cPath);

        // Check for emissive material
        aiColor4D EmissiveColour(0.f, 0.f, 0.f, 0.0f);
        aiGetMaterialColor(p_Scene->mMaterials[i], AI_MATKEY_COLOR_EMISSIVE, &EmissiveColour);
        aiColor4D DiffuseColour(1.f, 1.f, 1.f, 1.0f);
        aiGetMaterialColor(p_Scene->mMaterials[i], AI_MATKEY_COLOR_DIFFUSE, &DiffuseColour);
        p_Material->m_fEmissive = EmissiveColour.r / DiffuseColour.r;

        // Check for transparent material
        glBindTexture(GL_TEXTURE_2D, p_Material->m_uiDiffuse);
        GLint iAlpha = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_SIZE, &iAlpha);
        p_Material->m_bTransparent = (iAlpha > 0);

        // Check for reflective material
        float fReflectivity = 0.0f;
        aiGetMaterialFloat(p_Scene->mMaterials[i], AI_MATKEY_REFLECTIVITY, &fReflectivity);
        p_Material->m_bReflective = (fReflectivity > 0.0f);
        ++SceneInfo.m_uiNumMaterials;
    }

    // Allocate buffers for each object
    unsigned uiNumBackup = SceneInfo.m_uiNumObjects;
    GL_LoadSceneNode(p_Scene->mRootNode, p_Scene, SceneInfo, glm::mat4(1.0f), false);
    SceneInfo.mp_Objects = (ObjectData *)realloc(SceneInfo.mp_Objects, SceneInfo.m_uiNumObjects * sizeof(ObjectData));
    SceneInfo.mp_TransObjects = (ObjectData *)realloc(SceneInfo.mp_TransObjects, SceneInfo.m_uiNumObjects * sizeof(ObjectData));
    SceneInfo.mp_ReflecObjects = (ReflectObjectData *)realloc(SceneInfo.mp_ReflecObjects, SceneInfo.m_uiNumObjects * sizeof(ReflectObjectData));
    SceneInfo.m_uiNumObjects = uiNumBackup; // Reset
    // Load in each object
    GL_LoadSceneNode(p_Scene->mRootNode, p_Scene, SceneInfo, glm::mat4(1.0f), true);

    // Allocate buffers for each light
    SceneInfo.mp_PointLights = (PointLightData *)realloc(SceneInfo.mp_PointLights, p_Scene->mNumLights * sizeof(PointLightData));
    // Load in each light
    for (unsigned i = 0; i < p_Scene->mNumLights; i++) {
        const aiLight * p_AILight = p_Scene->mLights[i];
        mat4 m4Ret(1.0f);
        GL_FindSceneNode(p_Scene->mRootNode, p_AILight->mName, p_Scene, m4Ret, m4Ret);
        if (p_AILight->mType == aiLightSource_POINT) {
            // Get point light
            PointLightData * p_Light = &SceneInfo.mp_PointLights[SceneInfo.m_uiNumPointLights];
            vec3 v3Position = vec3(p_AILight->mPosition.x,
                                   p_AILight->mPosition.y,
                                   p_AILight->mPosition.z);
            p_Light->m_v3Position = (vec3)(m4Ret * vec4(v3Position, 1.0f));
            p_Light->m_v3Colour = vec3(p_AILight->mColorDiffuse.r,
                                       p_AILight->mColorDiffuse.g,
                                       p_AILight->mColorDiffuse.b);
            // Divide linear and quadratic components by 2 to compensate for using a attenuation constant of 1
            p_Light->m_v3Falloff = vec3(
                (p_AILight->mAttenuationConstant == 0.0f) ? 1.0f : p_AILight->mAttenuationConstant,
                p_AILight->mAttenuationLinear / 2.0f,
                p_AILight->mAttenuationQuadratic / 2.0f);
            ++SceneInfo.m_uiNumPointLights;
        }
    }

    // Add lights to Light UBO
    if (SceneInfo.m_uiPointLightUBO == 0)
        glGenBuffers(1, &SceneInfo.m_uiPointLightUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, SceneInfo.m_uiPointLightUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(PointLightData) * SceneInfo.m_uiNumPointLights, SceneInfo.mp_PointLights, GL_STATIC_DRAW);

    // Load in camera
    if (p_Scene->mNumCameras > 0) {
        const aiCamera * p_AICamera = p_Scene->mCameras[0];
        mat4 m4Ret(1.0f);
        GL_FindSceneNode(p_Scene->mRootNode, p_AICamera->mName, p_Scene, m4Ret, m4Ret);
        vec4 v4Position = vec4(p_AICamera->mPosition.x,
                               p_AICamera->mPosition.y,
                               p_AICamera->mPosition.z, 1.0f);
        SceneInfo.m_LocalCamera.m_v3Position = (vec3)(m4Ret * v4Position);
        vec3 v3Direction = vec3(p_AICamera->mLookAt.x,
                                p_AICamera->mLookAt.y,
                                p_AICamera->mLookAt.z);
        v3Direction = normalize(mat3(m4Ret) * v3Direction);
        SceneInfo.m_LocalCamera.m_v3Direction = v3Direction;
        vec3 v3Up = normalize(mat3(m4Ret) * vec3(p_AICamera->mUp.x,
                                                 p_AICamera->mUp.y,
                                                 p_AICamera->mUp.z));
          // Assimp doesn't store a right vector so we calculate from up and direction
        SceneInfo.m_LocalCamera.m_v3Right = cross(v3Direction, v3Up);
        // Use orientation vectors to calculate corresponding axis angles
        SceneInfo.m_LocalCamera.m_fAngleX = atan2(v3Direction.x, v3Direction.z);
        SceneInfo.m_LocalCamera.m_fAngleY = asin(-v3Direction.y);
        SceneInfo.m_LocalCamera.m_fMoveZ = 0.0f;
        SceneInfo.m_LocalCamera.m_fMoveX = 0.0f;
        SceneInfo.m_LocalCamera.m_fFOV = p_AICamera->mHorizontalFOV;
        SceneInfo.m_LocalCamera.m_fAspect = (float)g_iWindowWidth / (float)g_iWindowHeight;
        SceneInfo.m_LocalCamera.m_fNear = p_AICamera->mClipPlaneNear;
        SceneInfo.m_LocalCamera.m_fFar = p_AICamera->mClipPlaneFar;
    } else if (SceneInfo.m_uiCameraUBO == 0) {
        // Initialise camera with default values
        SceneInfo.m_LocalCamera.m_fAngleX = (float)M_PI;
        SceneInfo.m_LocalCamera.m_fAngleY = 0.0f;
        SceneInfo.m_LocalCamera.m_fMoveZ = 0.0f;
        SceneInfo.m_LocalCamera.m_fMoveX = 0.0f;
        SceneInfo.m_LocalCamera.m_v3Position = vec3(0.0f, 0.0f, 12.0f);
        SceneInfo.m_LocalCamera.m_v3Direction = vec3(0.0f, 0.0f, -1.0f);
        SceneInfo.m_LocalCamera.m_v3Right = vec3(1.0f, 0.0f, 0.0f);
        SceneInfo.m_LocalCamera.m_fFOV = radians(45.0f);
        SceneInfo.m_LocalCamera.m_fAspect = (float)g_iWindowWidth / (float)g_iWindowHeight;
        SceneInfo.m_LocalCamera.m_fNear = 0.1f;
        SceneInfo.m_LocalCamera.m_fFar = 100.0f;
    }

    // Create camera UBO
    if (SceneInfo.m_uiCameraUBO == 0)
        glGenBuffers(1, &SceneInfo.m_uiCameraUBO);

    // Destroy the scene
    aiReleaseImport(p_Scene);
    free(p_cPath);
    return true;
}

void GL_UnloadScene(SceneData & SceneInfo)
{
    // Delete VBOs/IBOs and VAOs
    for (unsigned i = 0; i < SceneInfo.m_uiNumMeshes; i++) {
        glDeleteBuffers(1, &SceneInfo.mp_Meshes[i].m_uiVBO);
        glDeleteBuffers(1, &SceneInfo.mp_Meshes[i].m_uiIBO);
        glDeleteVertexArrays(1, &SceneInfo.mp_Meshes[i].m_uiVAO);
    }
    free(SceneInfo.mp_Meshes);

    // Delete materials
    for (unsigned i = 0; i < SceneInfo.m_uiNumMaterials; i++) {
        glDeleteTextures(3, &SceneInfo.mp_Materials[i].m_uiDiffuse);
    }
    free(SceneInfo.mp_Materials);

    // Delete objects
    for (unsigned i = 0; i < SceneInfo.m_uiNumObjects; i++) {
        glDeleteBuffers(1, &SceneInfo.mp_Objects[i].m_uiTransformUBO);
    }
    free(SceneInfo.mp_Objects);
    free(SceneInfo.mp_TransObjects);
    for (unsigned i = 0; i < SceneInfo.m_uiNumReflecObjects; i++) {
        const ReflectObjectData * p_RObject = &SceneInfo.mp_ReflecObjects[i];
        const ObjectData * p_Object = &SceneInfo.mp_Objects[p_RObject->m_uiObjectPos];
        glDeleteTextures(1, &p_Object->m_uiReflect);
        if (p_Object->m_uiReflective == 1)
            glDeleteBuffers(1, &p_Object->m_uiReflectVPUBO);
    }
    free(SceneInfo.mp_ReflecObjects);

    // Delete light UBO
    glDeleteBuffers(1, &SceneInfo.m_uiPointLightUBO);
    free(SceneInfo.mp_PointLights);

    // Delete camera UBO
    glDeleteBuffers(1, &SceneInfo.m_uiCameraUBO);
}
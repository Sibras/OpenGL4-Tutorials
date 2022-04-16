// Using SDL, SDL OpenGL, GLEW
#include <math.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
// Using KTX import
#include <ktx.h>

bool GL_LoadTextureBMP(GLuint uiTexture, const char * p_cTextureFile)
{
    // Load texture data
    SDL_Surface * p_Surface = SDL_LoadBMP(p_cTextureFile);
    if (p_Surface == NULL) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to open texture file: %s (%s)\n", p_cTextureFile, SDL_GetError());
        return false;
    }

    // Determine image format
    GLenum Format;
    GLint iInternalFormat;
    if (p_Surface->format->BytesPerPixel == 4) {
        iInternalFormat = GL_RGBA;
        if (p_Surface->format->Rmask == 0x000000ff)
            Format = GL_RGBA;
        else
            Format = GL_BGRA;
    } else if (p_Surface->format->BytesPerPixel == 3) {
        iInternalFormat = GL_RGBA;
        if (p_Surface->format->Rmask == 0x000000ff)
            Format = GL_RGB;
        else
            Format = GL_BGR;
    } else if (p_Surface->format->BytesPerPixel == 1) {
        iInternalFormat = GL_RED;
        Format = GL_RED;
    } else {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Unknown texture format: %d\n", p_Surface->format->BytesPerPixel);
        return false;
    }

    // Correctly flip image data
    const int iRowSize = p_Surface->w * p_Surface->format->BytesPerPixel;
    const int iImageSize = iRowSize * p_Surface->h;
    GLubyte * p_TextureData = (GLubyte*)malloc(iImageSize);
    for (int i = 0; i < p_Surface->h * iRowSize; i += iRowSize) {
        memcpy(&p_TextureData[i], &((GLubyte*)p_Surface->pixels)[iImageSize - i], iRowSize);
    }

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, uiTexture);

    // Copy data into texture
    glTexImage2D(GL_TEXTURE_2D, 0, iInternalFormat, p_Surface->w, p_Surface->h, 0, Format, GL_UNSIGNED_BYTE, p_TextureData);

    // Unload the SDL surface
    SDL_FreeSurface(p_Surface);
    free(p_TextureData);

    // Initialise the texture filtering values
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);

    // Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);

    return true;
}

bool GL_LoadTextureKTX(GLuint uiTexture, const char * p_cTextureFile)
{
    // Load texture data
    ktxTexture* kTexture;
    KTX_error_code ktxerror = ktxTexture_CreateFromNamedFile(p_cTextureFile,
        KTX_TEXTURE_CREATE_NO_FLAGS,
        &kTexture);
    if (ktxerror != KTX_SUCCESS) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to read texture file: %s (%s)\n", p_cTextureFile, ktxErrorString(ktxerror));
        return false;
    }
    GLenum GLTarget, GLError;
    ktxerror = ktxTexture_GLUpload(kTexture, &uiTexture, &GLTarget, &GLError);
    if (ktxerror != KTX_SUCCESS) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to upload texture file: %s\n", ktxErrorString(ktxerror));
        return false;
    }

    // Generate mipmaps
    if (kTexture->numLevels == 1)
        glGenerateMipmap(GL_TEXTURE_2D);

    ktxTexture_Destroy(kTexture);

    // Initialise the texture filtering values
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);

    return true;
}

// Using GLI
//#include <gli/gli.hpp>
//bool GL_ConvertDDS2KTX(const char * p_cTextureFile)
//{
//    // Load texture data
//    gli::texture Texture(gli::load_dds(p_cTextureFile));
//    if (Texture.empty()) {
//        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to open texture file: %s\n", p_cTextureFile);
//        return false;
//    }
//    const char * p_cKTXExt = ".ktx";
//    const unsigned uiInLength = strlen(p_cTextureFile) - 4;
//    char * p_cOutFile = (char *)malloc(uiInLength + 4 + 1);
//    memcpy(p_cOutFile, p_cTextureFile, uiInLength);
//    memcpy(p_cOutFile + uiInLength, p_cKTXExt, 5);
//    if (!gli::save_ktx(Texture, p_cOutFile)) {
//        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to write texture file: %s\n", p_cOutFile);
//        return false;
//    }
//    return true;
//}
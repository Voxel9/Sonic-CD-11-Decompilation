#include "platform/Graphics.hpp"
#include <cstdlib>
#include <cmath>
#include <cassert>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Context functions

typedef struct GfxContext {
    GLFWwindow* window;
} GfxContext;

GfxContext* Gfx_Initialize(int width, int height, const char* gameTitle)
{
    GfxContext *ret = (GfxContext*)malloc(sizeof(GfxContext));

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GL_TRUE);

    ret->window = glfwCreateWindow(width, height, gameTitle, NULL, NULL);
    assert(ret->window != nullptr);

    glfwMakeContextCurrent(ret->window);
    glfwSwapInterval(1);

    // glew Setup
    GLenum err = glewInit();
    assert(err == GLEW_OK);

    glDisable(GL_LIGHTING);
    glDisable(GL_DITHER);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    return ret;
}

bool Gfx_MainLoop(GfxContext* ctx)
{
    return !glfwWindowShouldClose(ctx->window);
}

bool Gfx_IsDevMenuTriggered(GfxContext* ctx)
{
    return glfwGetKey(ctx->window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
}

void Gfx_FrameBegin()
{
    glfwPollEvents();
}

void Gfx_FrameEnd(GfxContext* ctx)
{
    glfwSwapBuffers(ctx->window);
}

void Gfx_Finalize(GfxContext* ctx)
{
    glfwDestroyWindow(ctx->window);
    free(ctx);
}

// Renderstate functions

void Gfx_Clear()
{
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Gfx_SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void Gfx_SetBlend(bool enable)
{
    glBlendFunc(
        enable ? GL_SRC_ALPHA : GL_ONE,
        enable ? GL_ONE_MINUS_SRC_ALPHA : GL_ZERO);
}

void Gfx_SetVertexBufs(int stride, void* verts)
{
    unsigned char* ptr = (unsigned char*)verts;
    bool isVertex3D = (stride == 20);

    glVertexPointer(isVertex3D ? 3 : 2, isVertex3D ? GL_FLOAT : GL_SHORT, stride, ptr);
    ptr += isVertex3D ? (sizeof(float) * 3) : (sizeof(short) * 2);
    glTexCoordPointer(2, GL_SHORT, stride, ptr);
    ptr += sizeof(short) * 2;
    glColorPointer(4, GL_UNSIGNED_BYTE, stride, ptr);
}

void Gfx_DrawElements(int count, void* indices)
{
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
}

// Texture functions

typedef struct GfxTexture {
    int width;
    int height;
    GLenum type;
    GLuint texID;
} GfxTexture;

GfxTexture* Gfx_TextureCreate(int width, int height, bool isRGB5A1, bool isVRAM)
{
    GfxTexture *ret = (GfxTexture*)malloc(sizeof(GfxTexture));
    ret->width = width;
    ret->height = height;
    ret->type = isRGB5A1 ? GL_UNSIGNED_SHORT_5_5_5_1 : GL_UNSIGNED_INT_8_8_8_8;

    glGenTextures(1, &ret->texID);
    glBindTexture(GL_TEXTURE_2D, ret->texID);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        ret->width,
        ret->height,
        0,
        GL_RGBA,
        ret->type,
        NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return ret;
}

void Gfx_TextureBind(GfxTexture* tex)
{
    glBindTexture(GL_TEXTURE_2D, tex->texID);
}

void Gfx_TextureUpload(GfxTexture* tex, void* pixels)
{
    glBindTexture(GL_TEXTURE_2D, tex->texID);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        tex->width,
        tex->height,
        GL_RGBA,
        tex->type,
        pixels);
}

void Gfx_TextureSetFilter(GfxTexture* tex, bool isLinear)
{
    glBindTexture(GL_TEXTURE_2D, tex->texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, isLinear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, isLinear ? GL_LINEAR : GL_NEAREST);
}

void Gfx_TextureDestroy(GfxTexture* tex)
{
    glDeleteTextures(1, &tex->texID);
    free(tex);
}

// Framebuffer functions

typedef struct GfxRenderTarget {
    GLuint rtID;
} GfxRenderTarget;

GfxRenderTarget* Gfx_RenderTargetCreateFromTexture(GfxTexture* tex)
{
    GfxRenderTarget *ret = (GfxRenderTarget*)malloc(sizeof(GfxRenderTarget));

    glGenFramebuffers(1, &ret->rtID);
    glBindFramebuffer(GL_FRAMEBUFFER, ret->rtID);

    glBindTexture(GL_TEXTURE_2D, tex->texID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->texID, 0);

    return ret;
}

void Gfx_RenderTargetBind(GfxRenderTarget* rt)
{
    glBindFramebuffer(GL_FRAMEBUFFER, rt ? rt->rtID : 0);
}

void Gfx_RenderTargetDestroy(GfxRenderTarget* rt)
{
    glDeleteFramebuffers(1, &rt->rtID);
    free(rt);
}

// Matrix functions

void Gfx_MatrixMode(GfxMatrixMode mode)
{
    GLenum glMtxMode[MAX_MTX_MODES] = {
        GL_MODELVIEW,
        GL_PROJECTION,
        GL_TEXTURE,
    };

    glMatrixMode(glMtxMode[mode]);
}

void Gfx_LoadIdentity(void)
{
    glLoadIdentity();
}

void Gfx_PushMatrix(void)
{
    glPushMatrix();
}

void Gfx_PopMatrix(void)
{
    glPopMatrix();
}

void Gfx_Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
{
    glOrtho(left, right, bottom, top, zNear, zFar);
}

void Gfx_PerspStereo(float fovy, float aspect, float near, float far, float iod, float screen)
{
    float matrix[16];
    float w = 1.0 / tanf(fovy * 0.5f);
    float h = 1.0 / (w * aspect);
    float q = (near + far) / (far - near);

    matrix[0] = w;
    matrix[1] = 0;
    matrix[2] = 0;
    matrix[3] = 0;

    matrix[4] = 0;
    matrix[5] = h / 2;
    matrix[6] = 0;
    matrix[7] = 0;

    matrix[8]  = 0;
    matrix[9]  = 0;
    matrix[10] = q;
    matrix[11] = 1.0;

    matrix[12] = 0;
    matrix[13] = 0;
    matrix[14] = (((far * -2.0f) * near) / (far - near));
    matrix[15] = 0;

    glMultMatrixf(matrix);
}

void Gfx_Translate(float x, float y, float z)
{
    glTranslatef(x, y, z);
}

void Gfx_RotateY(float angle)
{
    glRotatef(angle, 0.0, 1.0, 0.0);
}

void Gfx_RotateZ(float angle)
{
    glRotatef(angle, 0.0, 0.0, 1.0);
}

void Gfx_Scale(float x, float y, float z)
{
    glScalef(x, y, z);
}

// GPU mem functions

void* Gfx_LinearAlloc(size_t size)
{
    return _aligned_malloc(size, 0x80);
}

void Gfx_LinearFree(void* mem)
{
    _aligned_free(mem);
}

#include "platform/Graphics.hpp"
#include <cstdlib>
#include <cmath>
#include <cassert>

// Context functions

typedef struct GfxContext {
    // TODO
} GfxContext;

GfxContext* Gfx_Initialize(int width, int height, const char* gameTitle)
{
    GfxContext *ret = (GfxContext*)malloc(sizeof(GfxContext));

    // TODO

    return ret;
}

bool Gfx_IsQuitTriggered(GfxContext* ctx)
{
    return false;
}

bool Gfx_IsDevMenuTriggered(GfxContext* ctx)
{
    return false;
}

void Gfx_SwapBuffers(GfxContext* ctx)
{
    // TODO
}

void Gfx_Finalize(GfxContext* ctx)
{
    // TODO

    free(ctx);
}

// Renderstate functions

void Gfx_Clear()
{
    // TODO
}

void Gfx_SetViewport(int x, int y, int width, int height)
{
    // TODO
}

void Gfx_SetBlend(bool enable)
{
    // TODO
}

void Gfx_SetVertexBufs(int stride, void* verts)
{
    // TODO
}

void Gfx_DrawElements(int count, void* indices)
{
    // TODO
}

// Texture functions

typedef struct GfxTexture {
    // TODO
} GfxTexture;

GfxTexture* Gfx_TextureCreate(int width, int height, bool isRGB5A1)
{
    GfxTexture *ret = (GfxTexture*)malloc(sizeof(GfxTexture));
    
    // TODO

    return ret;
}

void Gfx_TextureBind(GfxTexture* tex)
{
    // TODO
}

void Gfx_TextureUpload(GfxTexture* tex, void* pixels)
{
    // TODO
}

void Gfx_TextureSetFilter(GfxTexture* tex, bool isLinear)
{
    // TODO
}

void Gfx_TextureDestroy(GfxTexture* tex)
{
    // TODO

    free(tex);
}

// Framebuffer functions

typedef struct GfxRenderTarget {
    // TODO
} GfxRenderTarget;

GfxRenderTarget* Gfx_RenderTargetCreateFromTexture(GfxTexture* tex)
{
    GfxRenderTarget *ret = (GfxRenderTarget*)malloc(sizeof(GfxRenderTarget));

    // TODO

    return ret;
}

void Gfx_RenderTargetBind(GfxRenderTarget* rt)
{
    // TODO
}

void Gfx_RenderTargetDestroy(GfxRenderTarget* rt)
{
    // TODO

    free(rt);
}

// Matrix functions

void Gfx_MatrixMode(GfxMatrixMode mode)
{
    // TODO
}

void Gfx_LoadIdentity(void)
{
    // TODO
}

void Gfx_PushMatrix(void)
{
    // TODO
}

void Gfx_PopMatrix(void)
{
    // TODO
}

void Gfx_Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
{
    // TODO
}

void Gfx_PerspStereo(float fovy, float aspect, float near, float far, float iod, float screen)
{
    // TODO
}

void Gfx_Translate(float x, float y, float z)
{
    // TODO
}

void Gfx_RotateY(float angle)
{
    // TODO
}

void Gfx_RotateZ(float angle)
{
    // TODO
}

void Gfx_Scale(float x, float y, float z)
{
    // TODO
}

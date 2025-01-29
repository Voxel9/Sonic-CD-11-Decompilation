#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <cstddef>

typedef enum GfxMatrixMode {
    MTX_MODE_MODELVIEW,
    MTX_MODE_PROJECTION,
    MTX_MODE_TEXTURE,
    MAX_MTX_MODES,
} GfxMatrixMode;

struct GfxContext;
struct GfxTexture;
struct GfxRenderTarget;

// Context functions
GfxContext* Gfx_Initialize(int width, int height, const char* gameTitle);
bool Gfx_MainLoop(GfxContext* ctx);
bool Gfx_IsDevMenuTriggered(GfxContext* ctx);
void Gfx_FrameBegin();
void Gfx_FrameEnd(GfxContext* ctx);
void Gfx_Finalize(GfxContext* ctx);

// Renderstate functions
void Gfx_Clear();
void Gfx_SetViewport(int x, int y, int width, int height);
void Gfx_SetBlend(bool enable);
void Gfx_SetVertexBufs(int stride, void* verts);
void Gfx_DrawElements(int count, void* indices);

// Texture functions
GfxTexture* Gfx_TextureCreate(int width, int height, bool isRGB5A1, bool isVRAM);
void Gfx_TextureBind(GfxTexture* tex);
void Gfx_TextureUpload(GfxTexture* tex, void* pixels);
void Gfx_TextureSetFilter(GfxTexture* tex, bool isLinear);
void Gfx_TextureDestroy(GfxTexture* tex);

// Framebuffer functions
GfxRenderTarget* Gfx_RenderTargetCreateFromTexture(GfxTexture* tex);
void Gfx_RenderTargetBind(GfxRenderTarget* rt);
void Gfx_RenderTargetDestroy(GfxRenderTarget* rt);

// Matrix functions
void Gfx_MatrixMode(GfxMatrixMode mode);
void Gfx_LoadIdentity(void);
void Gfx_PushMatrix(void);
void Gfx_PopMatrix(void);
void Gfx_Ortho(float left, float right, float bottom, float top, float zNear, float zFar);
void Gfx_PerspStereo(float fovy, float aspect, float near, float far, float iod, float screen);
void Gfx_Translate(float x, float y, float z);
void Gfx_RotateY(float angle);
void Gfx_RotateZ(float angle);
void Gfx_Scale(float x, float y, float z);

// GPU mem functions
void* Gfx_LinearAlloc(size_t size);
void Gfx_LinearFree(void* mem);

#endif

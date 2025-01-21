#include "platform/Graphics.hpp"
#include <cstdlib>
#include <cmath>
#include <cassert>

#include <3ds.h>
#include <citro3d.h>

extern const u8 vshader_shbin_end[];
extern const u8 vshader_shbin[];
extern const u32 vshader_shbin_size;

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB5A1) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB565) | \
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static unsigned char swizzleTexBuffer[4 * 1024 * 1024];

static C3D_RenderTarget* mainRT;

static C3D_MtxStack mtxStacks[MAX_MTX_MODES];
static int ulocMtx[MAX_MTX_MODES];

static C3D_MtxStack *curMtxStack;

// Context functions

typedef struct GfxContext {
    DVLB_s* vshaderDVLB;
    shaderProgram_s program;
} GfxContext;

GfxContext* Gfx_Initialize(int width, int height, const char* gameTitle)
{
    GfxContext *ret = (GfxContext*)malloc(sizeof(GfxContext));

    // Init video
    gfxInit(GSP_RGB565_OES, GSP_RGB565_OES, false);
    consoleInit(GFX_BOTTOM, NULL);

    // Init Citro3D
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    // Init render targets
    mainRT = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA5551, -1);
    C3D_RenderTargetSetOutput(mainRT, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    // Init render states
    C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, (GPU_TEVSRC)0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);

    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_ALL);

    // Load the vertex shader, create a shader program and bind it
    ret->vshaderDVLB = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
    shaderProgramInit(&ret->program);
    shaderProgramSetVsh(&ret->program, &ret->vshaderDVLB->DVLE[0]);
    C3D_BindProgram(&ret->program);

    // Init matrix stacks
    const char* mtxNames[MAX_MTX_MODES] = { "mv", "p", "tex", };
    for(int i = 0; i < MAX_MTX_MODES; i++) {
        MtxStack_Init(&mtxStacks[i]);
        ulocMtx[i] = shaderInstanceGetUniformLocation(ret->program.vertexShader, mtxNames[i]);
        MtxStack_Bind(&mtxStacks[i], GPU_VERTEX_SHADER, ulocMtx[i], 4);
    }

    // Default matrix mode
    curMtxStack = &mtxStacks[MTX_MODE_MODELVIEW];

    return ret;
}

bool Gfx_IsQuitTriggered(GfxContext* ctx)
{
    return !aptMainLoop();
}

bool Gfx_IsDevMenuTriggered(GfxContext* ctx)
{
    return (hidKeysDown() & KEY_SELECT) == KEY_SELECT;
}

void Gfx_FrameBegin()
{
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C3D_FrameDrawOn(mainRT);
}

void Gfx_FrameEnd(GfxContext* ctx)
{
    C3D_FrameEnd(0);
}

void Gfx_Finalize(GfxContext* ctx)
{
    // Release shader objects
    shaderProgramFree(&ctx->program);
    DVLB_Free(ctx->vshaderDVLB);

    // Release render targets
    C3D_RenderTargetDelete(mainRT);

    // Cleanup Citro3D
    C3D_Fini();

    free(ctx);
}

// Renderstate functions

void Gfx_Clear()
{
    //C3D_RenderTargetClear(mainRT, C3D_CLEAR_COLOR, 0x0000FFFF, 0);
}

void Gfx_SetViewport(int x, int y, int width, int height)
{
    C3D_SetViewport(y, x, height, width);
}

void Gfx_SetBlend(bool enable)
{
    C3D_AlphaBlend(
        GPU_BLEND_ADD,
        GPU_BLEND_ADD,
        enable ? GPU_SRC_ALPHA : GPU_ONE,
        enable ? GPU_ONE_MINUS_SRC_ALPHA : GPU_ZERO,
        enable ? GPU_SRC_ALPHA : GPU_ONE,
        enable ? GPU_ONE_MINUS_SRC_ALPHA : GPU_ZERO);
}

void Gfx_SetVertexBufs(int stride, void* verts)
{
    bool isVertex3D = (stride == 20);
    GPU_FORMATS posFmt = isVertex3D ? GPU_FLOAT : GPU_SHORT;
    int posCnt = isVertex3D ? 3 : 2;

    // Configure attributes for use with the vertex shader
    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, posFmt, posCnt);        // v0=position
    AttrInfo_AddLoader(attrInfo, 1, GPU_SHORT, 2);          // v1=texcoord
    AttrInfo_AddLoader(attrInfo, 2, GPU_UNSIGNED_BYTE, 4);  // v2=color

    // Configure buffers
    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, verts, stride, 3, 0x210);
}

void Gfx_DrawElements(int count, void* indices)
{
    if(count >= 3) {
        for(int i = 0; i < MAX_MTX_MODES; i++) {
            MtxStack_Update(&mtxStacks[i]);
        }

        C3D_DrawElements(GPU_TRIANGLES, count, C3D_UNSIGNED_SHORT, indices);
    }
}

// Texture functions

typedef struct GfxTexture {
    C3D_Tex c3dTex;
    bool isRGB5A1;
} GfxTexture;

constexpr size_t prevPowerOfTwo(size_t value)
{
    return value > 0 ? 1 << (31 - __builtin_clz(static_cast<unsigned>(value))) : 0;
}

constexpr bool isPowerOfTwo(size_t value)
{
    return value > 0 && (value & (value - 1)) == 0;
}

// Templated nearest-neighbor resize function
template <typename T>
void resizeToPrevPowerOfTwo(const T* in, T* out, size_t w, size_t h)
{
    // Check if dimensions are already powers of two
    size_t newWidth = isPowerOfTwo(w) ? w : prevPowerOfTwo(w);
    size_t newHeight = isPowerOfTwo(h) ? h : prevPowerOfTwo(h);

    // If no resizing is needed, directly copy the input pixels
    if (newWidth == w && newHeight == h) {
        memcpy(out, in, w * h * sizeof(T));
        return;
    }

    // Compute scaling factors (using fixed-point arithmetic)
    const uint32_t xScale = (static_cast<uint32_t>(w) << 16) / newWidth;
    const uint32_t yScale = (static_cast<uint32_t>(h) << 16) / newHeight;

    // Perform nearest-neighbor resizing (optimized loop for ARMv6k)
    for (size_t y = 0; y < newHeight; ++y) {
        const size_t srcY = (y * yScale) >> 16;
        const T* srcRow = in + srcY * w;
        T* destRow = out + y * newWidth;

        size_t x = 0;

        // Process pixels in blocks for better performance
        for (; x + 3 < newWidth; x += 4) {
            destRow[x] = srcRow[(x * xScale) >> 16];
            destRow[x + 1] = srcRow[((x + 1) * xScale) >> 16];
            destRow[x + 2] = srcRow[((x + 2) * xScale) >> 16];
            destRow[x + 3] = srcRow[((x + 3) * xScale) >> 16];
        }

        // Process remaining pixels
        for (; x < newWidth; ++x) {
            destRow[x] = srcRow[(x * xScale) >> 16];
        }
    }
}

// Morton interleaving using bit manipulation, optimized for ARMv6K
static inline u32 MortonInterleave(u32 x, u32 y)
{
    x = (x | (x << 8)) & 0x00FF00FF;
    x = (x | (x << 4)) & 0x0F0F0F0F;
    x = (x | (x << 2)) & 0x33333333;
    x = (x | (x << 1)) & 0x55555555;

    y = (y | (y << 8)) & 0x00FF00FF;
    y = (y | (y << 4)) & 0x0F0F0F0F;
    y = (y | (y << 2)) & 0x33333333;
    y = (y | (y << 1)) & 0x55555555;

    return x | (y << 1);
}

template <typename T>
void SwizzleTexBuffer(T* in, T* out, u32 w, u32 h)
{
    const u32 px_size = sizeof(T);

    // Tile sizes for better cache utilization
    const u32 tile_size = 8;
    const u32 block_size = tile_size * tile_size;

    // Process the texture in 8x8 blocks
    for (u32 by = 0; by < h; by += tile_size) {
        for (u32 bx = 0; bx < w; bx += tile_size) {
            // Prefetch the block (optional but useful for large textures)
            __asm__("pld [%0]" : : "r"(in + by * w + bx));

            // Unroll the 8x8 block processing
            for (u32 y = 0; y < tile_size; ++y) {
                u32 flipped_y = h - 1 - (by + y);
                u32 base_offset_y = (flipped_y & ~7) * w * px_size;

                for (u32 x = 0; x < tile_size; x += 4) { // Process 4 pixels at a time
                    u32 src_x1 = bx + x + 0;
                    u32 src_x2 = bx + x + 1;
                    u32 src_x3 = bx + x + 2;
                    u32 src_x4 = bx + x + 3;

                    u32 morton1 = MortonInterleave(src_x1 & 7, flipped_y & 7);
                    u32 morton2 = MortonInterleave(src_x2 & 7, flipped_y & 7);
                    u32 morton3 = MortonInterleave(src_x3 & 7, flipped_y & 7);
                    u32 morton4 = MortonInterleave(src_x4 & 7, flipped_y & 7);

                    u32 coarse_x = (src_x1 & ~7) * 8 * px_size;

                    u8* dst1 = (u8*)out + base_offset_y + coarse_x + morton1 * px_size;
                    u8* dst2 = (u8*)out + base_offset_y + coarse_x + morton2 * px_size;
                    u8* dst3 = (u8*)out + base_offset_y + coarse_x + morton3 * px_size;
                    u8* dst4 = (u8*)out + base_offset_y + coarse_x + morton4 * px_size;

                    // Copy pixel data
                    *reinterpret_cast<T*>(dst1) = in[(by + y) * w + src_x1];
                    *reinterpret_cast<T*>(dst2) = in[(by + y) * w + src_x2];
                    *reinterpret_cast<T*>(dst3) = in[(by + y) * w + src_x3];
                    *reinterpret_cast<T*>(dst4) = in[(by + y) * w + src_x4];
                }
            }
        }
    }
}

GfxTexture* Gfx_TextureCreate(int width, int height, bool isRGB5A1)
{
    GfxTexture *ret = (GfxTexture*)malloc(sizeof(GfxTexture));
    ret->isRGB5A1 = isRGB5A1;

    C3D_TexInit(&ret->c3dTex, width, height, isRGB5A1 ? GPU_RGBA5551 : GPU_RGBA8);
    C3D_TexSetWrap(&ret->c3dTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);

    return ret;
}

void Gfx_TextureBind(GfxTexture* tex)
{
    C3D_TexBind(0, &tex->c3dTex);
}

void Gfx_TextureUpload(GfxTexture* tex, void* pixels)
{
    if(tex->isRGB5A1)
        SwizzleTexBuffer((u16*)pixels, (u16*)tex->c3dTex.data, tex->c3dTex.width, tex->c3dTex.height);
    else
        SwizzleTexBuffer((u32*)pixels, (u32*)tex->c3dTex.data, tex->c3dTex.width, tex->c3dTex.height);
}

void Gfx_TextureSetFilter(GfxTexture* tex, bool isLinear)
{
    C3D_TexSetFilter(
        &tex->c3dTex,
        isLinear ? GPU_LINEAR : GPU_NEAREST,
        isLinear ? GPU_LINEAR : GPU_NEAREST);
}

void Gfx_TextureDestroy(GfxTexture* tex)
{
    C3D_TexDelete(&tex->c3dTex);
    free(tex);
}

// Framebuffer functions

typedef struct GfxRenderTarget {
    C3D_RenderTarget* rt;
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
    curMtxStack = &mtxStacks[mode];
}

void Gfx_LoadIdentity(void)
{
    Mtx_Identity(MtxStack_Cur(curMtxStack));
}

void Gfx_PushMatrix(void)
{
    MtxStack_Push(curMtxStack);
}

void Gfx_PopMatrix(void)
{
    MtxStack_Pop(curMtxStack);
}

void Gfx_Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
{
    Mtx_OrthoTilt(MtxStack_Cur(curMtxStack), left, right, bottom, top, zFar, zNear, false);
}

void Gfx_PerspStereo(float fovy, float aspect, float near, float far, float iod, float screen)
{
    Mtx_PerspStereoTilt(MtxStack_Cur(curMtxStack), fovy, aspect, near, far, iod, screen, false);
}

void Gfx_Translate(float x, float y, float z)
{
    Mtx_Translate(MtxStack_Cur(curMtxStack), x, y, z, true);
}

void Gfx_RotateY(float angle)
{
    Mtx_RotateY(MtxStack_Cur(curMtxStack), C3D_AngleFromDegrees(angle), true);
}

void Gfx_RotateZ(float angle)
{
    Mtx_RotateZ(MtxStack_Cur(curMtxStack), C3D_AngleFromDegrees(angle), true);
}

void Gfx_Scale(float x, float y, float z)
{
    Mtx_Scale(MtxStack_Cur(curMtxStack), x, y, z);
}

// GPU mem functions

void* Gfx_LinearAlloc(size_t size)
{
    return linearAlloc(size);
}

void Gfx_LinearFree(void* mem)
{
    linearFree(mem);
}

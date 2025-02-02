#include "Video3DS.hpp"
#include "theoraplayer/th_frame.hpp"
#include "Debug.hpp"    // For PrintLog()

#include <3ds.h>
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstring>
#include <cmath>

#define WAVEBUFCOUNT 3

THEORA_Context vidCtx;
THEORA_videoinfo* vinfo;
THEORA_audioinfo* ainfo;
TH3DS_Frame frame;
static size_t buffSize = 8 * 4096;
static ndspWaveBuf waveBuf[WAVEBUFCOUNT];
int16_t* audioBuffer;
LightEvent soundEvent;

int isplaying = false;
bool videodone = false;

// For RetroEngine drawing
extern int videoWidth;
extern int videoHeight;

int InitAudioPlayback();
void ReleaseAudioDevice();

void audioInit(THEORA_audioinfo* ainfo) {
    ndspChnReset(0);
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetInterp(0, ainfo->channels == 2 ? NDSP_INTERP_POLYPHASE : NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, ainfo->rate);
    ndspChnSetFormat(0, ainfo->channels == 2 ? NDSP_FORMAT_STEREO_PCM16 : NDSP_FORMAT_MONO_PCM16);
    audioBuffer = (int16_t*) linearAlloc((buffSize * sizeof(int16_t)) * WAVEBUFCOUNT);

    memset(waveBuf, 0, sizeof(waveBuf));
    for (unsigned i = 0; i < WAVEBUFCOUNT; ++i)
    {
        waveBuf[i].data_vaddr = &audioBuffer[i * buffSize];
        waveBuf[i].nsamples = buffSize;
        waveBuf[i].status = NDSP_WBUF_DONE;
    }
}

void audioClose(void) {
    ndspChnReset(0);
    if (audioBuffer) linearFree(audioBuffer);
}

void audioCallback(void *const arg_)
{
    (void)arg_;

    if (!isplaying)
        return;

    LightEvent_Signal(&soundEvent);
}

static int isOgg(const char* filepath) {
    FILE* fp = fopen(filepath, "r");
    char magic[16];

    if (!fp) {
        PrintLog("Could not open %s. Please make sure file exists.\n", filepath);
        return 0;
    }

    fseek(fp, 0, SEEK_SET);
    fread(magic, 1, 16, fp);
    fclose(fp);

    if (!strncmp(magic, "OggS", 4))
        return 1;

    return 0;
}

static void changeFile(const char* filepath) {
    int ret = 0;

    if (!isOgg(filepath)) {
        PrintLog("The file is not an ogg file.\n");
        return;
    }

    if ((ret = THEORA_Create(&vidCtx, filepath))) {
        PrintLog("THEORA_Create exited with error, %d.\n", ret);
        return;
    }

    if (!THEORA_HasVideo(&vidCtx) && !THEORA_HasAudio(&vidCtx)) {
        PrintLog("No audio or video stream could be found.\n");
        return;
    }

    vinfo = THEORA_vidinfo(&vidCtx);
    ainfo = THEORA_audinfo(&vidCtx);

    videoWidth = vinfo->width;
    videoHeight = vinfo->height;

    audioInit(ainfo);

    if (THEORA_HasVideo(&vidCtx))
        frameInit(&frame, vinfo);

    isplaying = true;
}

void PlayVideo3DS(const char* fileName) {
    // de-init Retro Engine audio
    ReleaseAudioDevice();

    ndspInit();
    ndspSetCallback(audioCallback, NULL);
    PrintLog("Loading from %s\n", fileName);

    changeFile(fileName);
    videodone = false;
}

void ProcessVideo3DS() {
    if (THEORA_eos(&vidCtx))
        videodone = true;

    if (THEORA_HasAudio(&vidCtx)) {
        for (int cur_wvbuf = 0; cur_wvbuf < WAVEBUFCOUNT; cur_wvbuf++) {
            ndspWaveBuf *buf = &waveBuf[cur_wvbuf];

            if(buf->status == NDSP_WBUF_DONE) {
                size_t read = THEORA_readaudio(&vidCtx, (char*) buf->data_pcm16, buffSize);

                if(read <= 0)
                    break;
                else if(read <= buffSize)
                    buf->nsamples = read / ainfo->channels;

                ndspChnWaveBufAdd(0, buf);
            }
            DSP_FlushDataCache(buf->data_pcm16, buffSize * sizeof(int16_t));
        }
    }

    if (THEORA_HasVideo(&vidCtx)) {
        th_ycbcr_buffer ybr;
        if (THEORA_getvideo(&vidCtx, ybr)) {
            frameWrite(&frame, vinfo, ybr);
        }
    }
}

void BindVideoTex3DS() {
    C3D_TexBind(0, frame.img_tex);
}

void CloseVideo3DS() {
    PrintLog("video done, attempting to de-init");

    videodone = true;

    if (THEORA_HasVideo(&vidCtx))
        frameDelete(&frame);

    audioClose();

    THEORA_Close(&vidCtx);

    ndspExit();

    // re-init Retro Engine audio
    InitAudioPlayback();
}

// Original file: source/video.h
// Original author: oreo639
// https://github.com/oreo639/3ds-theoraplayer
// Licensed under the zlib license. See "COPYING" for full information.

#pragma once

#include <errno.h>

#include <theora/theoradec.h>
#include <tremor/ivorbiscodec.h>

typedef struct tf_callbacks {
	size_t (*read_func)  (void *ptr, size_t size, size_t nmemb, void *datasource);
	int    (*seek_func)  (void *datasource, ogg_int64_t offset, int whence);
	int    (*close_func) (void *datasource);
	long   (*tell_func)  (void *datasource);
} THEORA_callbacks;

typedef struct {
	int width;
	int height;
	double fps;
	th_pixel_fmt fmt;
	th_colorspace colorspace;
} THEORA_videoinfo;

typedef struct {
	int channels;
	int rate;
} THEORA_audioinfo;

typedef struct {
	/* Current State */
	ogg_sync_state sync;
	ogg_page page;
	int eos;

	/* Stream Data */
	int tpackets;
	int vpackets;
	ogg_stream_state tstream;
	ogg_stream_state vstream;

	/* Metadata */
	th_info tinfo;
	th_comment tcomment;

	vorbis_info vinfo;
	vorbis_comment vcomment;

	/* Theora Data */
	th_dec_ctx *tdec;
	int pp_level_max;
	int pp_level;
	int pp_inc;

	/* Vorbis Data */
	int vdsp_init;
	vorbis_dsp_state vdsp;
	int vblock_init;
	vorbis_block vblock;

	/* I/O Data */
	THEORA_callbacks io;
	void *datasource;

	/* user info */
	THEORA_videoinfo videoinfo;
	THEORA_audioinfo audioinfo;

	/* Usage info */
	int frames;
	int dropped;
	double videobuf_time;
	ogg_int64_t timer_calibrate;
	int vstate;
} THEORA_Context;

int THEORA_Create(THEORA_Context* ctx, const char* filepath);
int THEORA_CallbackCreate(THEORA_Context* ctx, void* datasource, THEORA_callbacks io);
void THEORA_Close(THEORA_Context *ctx);
bool THEORA_HasVideo(THEORA_Context *ctx);
bool THEORA_HasAudio(THEORA_Context *ctx);
THEORA_videoinfo* THEORA_vidinfo(THEORA_Context *ctx);
THEORA_audioinfo* THEORA_audinfo(THEORA_Context *ctx);
int THEORA_eos(THEORA_Context *ctx);
void THEORA_reset(THEORA_Context *ctx);
int THEORA_getvideo(THEORA_Context *ctx, th_ycbcr_buffer ybr);
int THEORA_readaudio(THEORA_Context *ctx, char *bufferOut, int buffSize);

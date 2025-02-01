// Original file: source/frame.h
// Original author: oreo639
// https://github.com/oreo639/3ds-theoraplayer
// Licensed under the zlib license. See "COPYING" for full information.

#pragma once

#include <citro3d.h>

#include "th_video.hpp"

typedef struct theora_3ds_vframe {
	C3D_Tex* img_tex;
	C3D_Tex buff[2];
	bool curbuf;
} TH3DS_Frame;

int frameInit(TH3DS_Frame* vframe, THEORA_videoinfo* info);
void frameDelete(TH3DS_Frame* vframe);
void frameWrite(TH3DS_Frame* vframe, THEORA_videoinfo* info, th_ycbcr_buffer ybr);

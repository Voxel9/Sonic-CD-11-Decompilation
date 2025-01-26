#ifndef VIDEO_3DS_H
#define VIDEO_3DS_H

extern int isplaying;
extern bool videodone;

void PlayVideo3DS(const char* fileName);
void ProcessVideo3DS();
void BindVideoTex3DS();
void CloseVideo3DS();

#endif

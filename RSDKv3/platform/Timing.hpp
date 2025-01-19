#ifndef TIMING_H
#define TIMING_H

void SDL_StartTicks();
unsigned int SDL_GetTicks();

unsigned long long SDL_GetPerformanceFrequency();
unsigned long long SDL_GetPerformanceCounter();

#endif

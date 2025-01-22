#ifndef TIMING_H
#define TIMING_H

void Time_StartTicks();
unsigned int Time_GetTicks();

unsigned long long Time_GetPerformanceFrequency();
unsigned long long Time_GetPerformanceCounter();

#endif

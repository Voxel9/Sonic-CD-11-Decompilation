#include "platform/Timing.hpp"
#include <cassert>

#define WIN32_MEAN_AND_LEAN
#include <windows.h>

#define TIME_WRAP_VALUE	(~(DWORD)0)

static DWORD start;

void SDL_StartTicks()
{
    timeBeginPeriod(1); // use 1 ms timer precision
    start = timeGetTime();
}

unsigned int SDL_GetTicks()
{
    DWORD now, ticks;

    now = timeGetTime();

    if (now < start)
        ticks = (TIME_WRAP_VALUE - start) + now;
    else
        ticks = (now - start);

    return (ticks);
}

unsigned long long SDL_GetPerformanceFrequency()
{
    LARGE_INTEGER frequency;

    if (!QueryPerformanceFrequency(&frequency))
    {
        assert(false);
    }
    return frequency.QuadPart;
}

unsigned long long SDL_GetPerformanceCounter()
{
    LARGE_INTEGER counter;

    if (!QueryPerformanceCounter(&counter))
    {
        assert(false);
    }
    return counter.QuadPart;
}

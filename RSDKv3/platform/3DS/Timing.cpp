#include "platform/Timing.hpp"

#include <3ds.h>

static u64 g_startTicks;

void SDL_StartTicks()
{
    g_startTicks = svcGetSystemTick();
}

unsigned int SDL_GetTicks()
{
    u64 elapsed = svcGetSystemTick() - g_startTicks;
    return elapsed * 1000 / SYSCLOCK_ARM11;
}

unsigned long long SDL_GetPerformanceFrequency()
{
    return osGetTimeRef().sysclock_hz;
}

unsigned long long SDL_GetPerformanceCounter()
{
    return osGetTimeRef().value_tick;
}

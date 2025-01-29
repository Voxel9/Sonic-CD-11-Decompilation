#include "platform/Timing.hpp"

#include <sys/time.h>
#include <3ds.h>

static struct timeval start;

void Time_StartTicks()
{
    bool isN3DS = false;
    APT_CheckNew3DS(&isN3DS);

	if(isN3DS) {
        osSetSpeedupEnable(true);
    }

    gettimeofday(&start, NULL);
}

unsigned int Time_GetTicks()
{
    unsigned int ticks;
	struct timeval now;

	gettimeofday(&now, NULL);
	ticks = (now.tv_sec - start.tv_sec) * 1000 + (now.tv_usec - start.tv_usec) / 1000;

	return (ticks);
}

unsigned long long Time_GetPerformanceFrequency()
{
    return osGetTimeRef().sysclock_hz;
}

unsigned long long Time_GetPerformanceCounter()
{
    return osGetTimeRef().value_tick;
}

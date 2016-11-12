#include <3ds.h>

#define TICKS_PER_MSEC 268123.480

u64 osGetMicros() {
	return svcGetSystemTick() / (TICKS_PER_MSEC / 1000);
}

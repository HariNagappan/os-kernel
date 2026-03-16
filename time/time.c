#include "time.h"

static volatile uint64_t ticks = 0;

uint64_t time_get_ticks()
{
    return ticks;
}

void time_set_ticks(uint64_t t)
{
    ticks = t;
}
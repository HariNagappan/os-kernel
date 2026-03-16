#ifndef TIME_H
#define TIME_H

#include <stdint.h>

uint64_t time_get_ticks();
void time_set_ticks(uint64_t t);

#endif
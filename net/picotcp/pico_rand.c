#include "pico_stack.h"

static uint32_t _rand_seed = 0;
static pico_time global_pico_tick = 0;

static void pico_rand_feed(uint32_t feed)
{
    if (_rand_seed == 0) {
        _rand_seed = (uint32_t)global_pico_tick - 1;
    }
    if (!feed)
        return;

    _rand_seed *= 1664525;
    _rand_seed += 1013904223;
    _rand_seed ^= ~(feed);
}

uint32_t pico_rand(void)
{
    pico_rand_feed((uint32_t)global_pico_tick);
    return _rand_seed;
}

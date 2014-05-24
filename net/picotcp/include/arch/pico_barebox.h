#ifndef PICO_SUPPORT_BAREBOX
#define PICO_SUPPORT_BAREBOX

#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <clock.h>
#include <linux/math64.h>

/*
   #define MEMORY_MEASURE
 */
#define dbg printf

#define stack_fill_pattern(...) do {} while(0)
#define stack_count_free_words(...) do {} while(0)
#define stack_get_free_words() (0)

/* measure allocated memory */
#ifdef MEMORY_MEASURE
extern uint32_t max_mem;
extern uint32_t cur_mem;

static inline void *pico_zalloc(int x)
{
    uint32_t *ptr;
    if ((cur_mem + x) > (10 * 1024))
        return NULL;

    ptr = (uint32_t *)calloc(x + 4, 1);
    *ptr = (uint32_t)x;
    cur_mem += x;
    if (cur_mem > max_mem) {
        max_mem = cur_mem;
    }

    return (void*)(ptr + 1);
}

static inline void pico_free(void *x)
{
    uint32_t *ptr = (uint32_t*)(((uint8_t *)x) - 4);
    cur_mem -= *ptr;
    free(ptr);
}
#else

#define pico_zalloc(x) calloc(x, 1)
#define pico_free(x) free(x)
#endif

static inline uint32_t PICO_TIME(void)
{
	uint64_t time;

	time = get_time_ns();
	do_div(time, 1000000000);

	return (uint32_t) time;
}

static inline uint32_t PICO_TIME_MS(void)
{
	uint64_t time;

	time = get_time_ns();
	do_div(time, 1000000);

	return (uint32_t) time;
}

static inline void PICO_IDLE(void)
{
//    usleep(5000);
}

#endif  /* PICO_SUPPORT_BAREBOX */

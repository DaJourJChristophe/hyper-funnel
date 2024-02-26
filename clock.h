#ifndef SM_CLOCK_H
#define SM_CLOCK_H

#include <stdbool.h>
#include <stdint.h>

struct sm_clock
{
  int64_t a;
};

typedef struct sm_clock sm_clock_t;

extern void sm_clock_init(sm_clock_t *self);

extern int64_t sm_clock_tick(sm_clock_t *self);

#endif/*SM_CLOCK_H*/

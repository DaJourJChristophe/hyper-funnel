#include "clock.h"
#include "sequence.h"

void im_seq_init(im_seq_t *self)
{
  sm_clock_init(&self->clock);
}

// recur must be > than 0
// repeat must be > than 0
// if repeat >= recur than error
int64_t im_seq_tick(im_seq_t *self, const int64_t recur,
                                    const int64_t repeat)
{
  return (sm_clock_tick(&self->clock) % recur) / repeat;
}

/*   00 0000 00   11 1111 11   22 2222 22   33 3333 33  */

/*   0000000000   00000000000000000000000   0000000000  */

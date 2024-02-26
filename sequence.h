#ifndef IM_SEQ_H
#define IM_SEQ_H

#include "clock.h"

struct im_seq
{
  sm_clock_t clock;
};

typedef struct im_seq im_seq_t;

void im_seq_init(im_seq_t *self);

int64_t im_seq_tick(im_seq_t *self, const int64_t recur,
                                    const int64_t repeat);

#endif/*IM_SEQ_H*/

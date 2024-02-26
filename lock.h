#ifndef MC_LOCK_H
#define MC_LOCK_H

#include "sequence.h"

#include <stdint.h>

struct mc_lock
{
  im_seq_t seq;
  int64_t j;
  int64_t k;
};

typedef struct mc_lock mc_lock_t;

extern void mc_lock_init(mc_lock_t *self);

extern void mc_lock(mc_lock_t *self, const int64_t id);

#endif/*MC_LOCK_H*/

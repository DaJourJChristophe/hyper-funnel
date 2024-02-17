#ifndef HYPER_FUNNEL__LOAD_BALANCER_H
#define HYPER_FUNNEL__LOAD_BALANCER_H

#include "channel.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

struct load_balancer
{
  uint64_t *distribution;
  size_t cap;
  uint64_t i;
};

typedef struct load_balancer load_balancer_t;

load_balancer_t *load_balancer_new(const size_t cap);

void load_balancer_destroy(load_balancer_t *self);

bool load_balancer_publish(load_balancer_t *self, bidirectional_channel_t **channels, const int item);

#endif/*HYPER_FUNNEL__LOAD_BALANCER_H*/

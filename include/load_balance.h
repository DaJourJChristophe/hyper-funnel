#ifndef HYPER_FUNNEL__LOAD_BALANCER_H
#define HYPER_FUNNEL__LOAD_BALANCER_H

#include "channel.h"
#include "scheduler.h"

#include <turnpike/queue.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

struct load_balancer
{
  queue_t *inbound_queue;
  queue_t *outbound_queue;
  uint64_t *distribution;
  size_t cap;
  uint64_t i;
};

typedef struct load_balancer load_balancer_t;

load_balancer_t *load_balancer_new(const size_t max_queue, const size_t cap);

void load_balancer_destroy(load_balancer_t *self);

void load_balancer_wait(load_balancer_t *self, void *observable, scheduler_t *scheduler);

bool load_balancer_publish(load_balancer_t *self, void *observable, scheduler_t *scheduler,
  bidirectional_channel_t **channels, const void *data);

#endif/*HYPER_FUNNEL__LOAD_BALANCER_H*/

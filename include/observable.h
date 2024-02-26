#ifndef HYPER_FUNNEL__OBSERVABLE_H
#define HYPER_FUNNEL__OBSERVABLE_H

#include "channel.h"
#include "load_balance.h"
#include "observer.h"
#include "scheduler.h"

#include <turnpike/bipartite.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

struct observable
{
  bipartite_queue_t *queue;
  size_t cap;
  bidirectional_channel_t **channels;
  observer_t **observers;
  size_t max_observers;
  load_balancer_t *lb;
  uint64_t count;
  size_t max_threads;
  atomic_bool done;
  scheduler_t *scheduler;
};

typedef struct observable observable_t;

observable_t *observable_new(const size_t cap, const size_t max_observers, const size_t max_threads);

void observable_destroy(observable_t *self);

void observable_select(observable_t *self, observer_t *observer);

void observable_shutdown(observable_t *self);

bool observable_cleanup(observable_t *self);

bool observable_publish(observable_t *self, const void *data);

bool observable_subscribe(observable_t *self, observer_t *observer);

#endif/*HYPER_FUNNEL__OBSERVABLE_H*/

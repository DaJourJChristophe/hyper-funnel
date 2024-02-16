#ifndef HYPER_FUNNEL__OBSERVABLE_H
#define HYPER_FUNNEL__OBSERVABLE_H

#include "observer.h"

#include <turnpike/tsqueue.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

struct observable
{
  ts_queue_t *queue;
  size_t cap;
  observer_t **observers;
  size_t max_observers;
  uint64_t count;
  size_t max_threads;
  atomic_bool done;
  atomic_bool ready;
};

typedef struct observable observable_t;

observable_t *observable_new(const size_t cap, const size_t max_observers, const size_t max_threads);

void observable_destroy(observable_t *self);

void observable_select(observable_t *self);

void observable_clear(observable_t *self);

void observable_shutdown(observable_t *self);

bool observable_publish(observable_t *self, const int item);

bool observable_subscribe(observable_t *self, observer_t *observer);

#endif/*HYPER_FUNNEL__OBSERVABLE_H*/

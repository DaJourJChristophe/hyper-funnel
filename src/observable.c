#include "common.h"
#include "observable.h"
#include "observer.h"
#include "scheduler.h"

#include <turnpike/bipartite.h>

#include <stdatomic.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define COMMAND_QUEUE_CAPACITY       4096
#define DATA_QUEUE_CAPACITY          4096
#define DATA_QUEUE_SEGMENT_LENGTH    sizeof(int)

observable_t *observable_new(const size_t cap, const size_t max_observers, const size_t max_threads)
{
  observable_t *self = NULL;
  self = (observable_t *)_calloc(1, sizeof(*self));
  self->observers = (observer_t **)_calloc(max_observers, sizeof(*self->observers));
  self->channels = (bidirectional_channel_t **)_calloc(max_observers, sizeof(*self->channels));

  uint64_t i;

  for (i = 0; i < max_observers; i++)
  {
    self->channels[i] = bidirectional_channel_new(cap, cap);
  }

  self->queue = bipartite_queue_new(cap, 0);
  self->lb = load_balancer_new(cap, max_observers);
  self->scheduler = scheduler_new((2 * max_observers), COMMAND_QUEUE_CAPACITY, DATA_QUEUE_CAPACITY, NULL);

  for (i = 0; i < max_observers; i++)
  {
    scheduler_add(self->scheduler, self->channels[i]->downstream);
  }

  for (i = 0; i < max_observers; i++)
  {
    scheduler_add(self->scheduler, self->channels[i]->upstream);
  }

  atomic_init(&self->done, false);

  self->max_observers = max_observers;
  self->max_threads = max_threads;
  self->cap = cap;

  return self;
}

void observable_destroy(observable_t *self)
{
  if (self != NULL)
  {
    if (self->queue != NULL)
    {
      bipartite_queue_destroy(self->queue);
    }

    if (self->channels != NULL)
    {
      uint64_t i;

      for (i = 0; i < self->max_observers; i++)
      {
        bidirectional_channel_destroy(self->channels[i]);
      }

      free(self->channels);
      self->channels = NULL;
    }

    if (self->lb != NULL)
    {
      load_balancer_destroy(self->lb);
    }

    if (self->observers != NULL)
    {
      observer_t *observer = NULL;
      uint64_t i;

      for (i = 0; i < self->count; i++)
      {
        observer = self->observers[i];
        observer_destroy(observer);
      }

      __free(self->observers);
    }

    __free(self);
  }
}

void observable_select(observable_t *self, observer_t *observer)
{
  if (self == NULL)
  {
    return;
  }
loop:
  if (true == atomic_load(&observer->ready) ||
      true == atomic_load(&self->done))
  {
    goto done;
  }
  goto loop;
done:
  return;
}

void observable_shutdown(observable_t *self)
{
  if (self == NULL)
  {
    return;
  }

  bool expected = false;
  uint64_t i;

  for (i = 0; i < self->max_observers; i++)
  {
loop:
    if (false == scheduler_empty(self->scheduler, i))
    {
      goto loop;
    }
  }

  atomic_compare_exchange_strong(&self->done, &expected, true);
}

bool observable_cleanup(observable_t *self)
{
  if (self == NULL)
  {
    return false;
  }

  load_balancer_wait(self->lb, self, self->scheduler);

  return true;
}

bool observable_publish(observable_t *self, const void *data)
{
  if (self == NULL)
  {
    return false;
  }

  if (false == load_balancer_publish(self->lb, self, self->scheduler, self->channels, data))
  {
    fprintf(stderr, "%s(): %s\n", __func__, "could not enqueue item");
    return false;
  }

  return true;
}

bool observable_subscribe(observable_t *self, observer_t *observer)
{
  if (self == NULL || observer == NULL)
  {
    return false;
  }

  if (self->count >= self->max_observers)
  {
    return false;
  }

  self->observers[self->count++] = observer;

  return true;
}

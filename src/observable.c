#include "common.h"
#include "observable.h"
#include "observer.h"

#include <turnpike/tsqueue.h>

#include <stdatomic.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

observable_t *observable_new(const size_t cap, const size_t max_observers, const size_t max_threads)
{
  observable_t *self = NULL;
  self = (observable_t *)_calloc(1, sizeof(*self));
  self->observers = (observer_t **)_calloc(max_observers, sizeof(*self->observers));

  self->queue = ts_queue_new(cap);

  atomic_init(&self->done, false);
  atomic_init(&self->ready, false);

  self->max_observers = max_observers;
  self->max_threads;
  self->cap;

  return self;
}

void observable_destroy(observable_t *self)
{
  if (self != NULL)
  {
    if (self->queue != NULL)
    {
      ts_queue_destroy(self->queue);
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

void observable_select(observable_t *self)
{
  if (self == NULL)
  {
    return;
  }
loop:
  if (true == atomic_load(&self->ready) ||
      true == atomic_load(&self->done))
  {
    goto done;
  }
  goto loop;
done:
  return;
}

void observable_clear(observable_t *self)
{
  if (self == NULL)
  {
    return;
  }

  atomic_store(&self->ready, false);
}

void observable_shutdown(observable_t *self)
{
  if (self == NULL)
  {
    return;
  }

  atomic_store(&self->done, true);
}

bool observable_publish(observable_t *self, const int item)
{
  if (self == NULL)
  {
    return false;
  }

  if (false == ts_queue_enqueue(self->queue, item))
  {
    fprintf(stderr, "%s(): %s\n", __func__, "could not enqueue item");
    return false;
  }

  atomic_store(&self->ready, true);

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

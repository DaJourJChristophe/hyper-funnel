#include <turnpike/tsqueue.h>

#include <stdatomic.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct observer;

typedef void *(*observer_callback_t)(void *);

struct observable;

struct observer
{
  struct observable *observable;
  observer_callback_t notify;
};

typedef struct observer observer_t;

observer_t *observer_new(struct observable *observable, observer_callback_t notify)
{
  observer_t *self = NULL;
  self = (observer_t *)calloc(1, sizeof(*self));
  self->observable = observable;
  self->notify = notify;
  return self;
}

void observer_destroy(observer_t *self)
{
  if (self != NULL)
  {
    free(self);
    self = NULL;
  }
}

struct observable
{
  ts_queue_t *queue;
  size_t cap;
  observer_t **observers;
  size_t max_observers;
  uint64_t count;
  pthread_t *tids;
  size_t max_threads;
  atomic_bool done;
  atomic_bool ready;
};

typedef struct observable observable_t;

observable_t *observable_new(const size_t cap, const size_t max_observers, const size_t max_threads)
{
  observable_t *self = NULL;
  self = (observable_t *)calloc(1, sizeof(*self));
  self->queue = ts_queue_new(cap);
  self->tids = (pthread_t *)calloc(max_threads, sizeof(*self->tids));
  self->observers = (observer_t **)calloc(max_observers, sizeof(*self->observers));

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

    if (self->tids != NULL)
    {
      free(self->tids);
      self->tids = NULL;
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

      free(self->observers);
      self->observers = NULL;
    }

    free(self);
    self = NULL;
  }
}

void observable_init(observable_t *self)
{
  if (self == NULL)
  {
    return;
  }

  observer_t *observer = NULL;
  uint64_t i;

  for (i = 0; i < self->count; i++)
  {
    observer = self->observers[i];

    if (pthread_create(&self->tids[i], NULL, observer->notify, observer) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "could not create thread");
      exit(EXIT_FAILURE);
    }
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

void observable_wait(observable_t *self)
{
  if (self == NULL)
  {
    return;
  }

  uint64_t i;

  for (i = 0; i < self->count; i++)
  {
    if (pthread_join(self->tids[i], NULL) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "could not join thread");
      exit(EXIT_FAILURE);
    }
  }
}

void observable_shutdown(observable_t *self)
{
  if (self == NULL)
  {
    return;
  }

  uint64_t i;

  atomic_store(&self->done, true);

  for (i = 0; i < self->count; i++)
  {
    if (pthread_join(self->tids[i], NULL) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "could not join thread");
      exit(EXIT_FAILURE);
    }
  }
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

void *notifier1(void *args)
{
  observer_t *observer = (observer_t *)args;
  int *item = NULL;

  while (false == atomic_load(&observer->observable->done) ||
         false == ts_queue_empty(observer->observable->queue))
  {
    observable_select(observer->observable);

    while (NULL != (item = ts_queue_dequeue(observer->observable->queue)))
    {
      printf("%d\n", *item);
    }

    observable_clear(observer->observable);
  }

  return NULL;
}

void *notifier2(void *args)
{
  observer_t *observer = (observer_t *)args;
  int *item = NULL;

  while (false == atomic_load(&observer->observable->done) ||
         false == ts_queue_empty(observer->observable->queue))
  {
    observable_select(observer->observable);

    while (NULL != (item = ts_queue_dequeue(observer->observable->queue)))
    {
      printf("%d\n", *item);
    }

    observable_clear(observer->observable);
  }

  return NULL;
}

#define QUEUE_CAPACITY 10
#define MAX_OBSERVERS   5
#define MAX_THREADS     2

int main(void)
{
  observer_t *observer1 = NULL;
  observer_t *observer2 = NULL;

  observable_t *observable = NULL;
  observable = observable_new(QUEUE_CAPACITY, MAX_OBSERVERS, MAX_THREADS);

  observer1 = observer_new(observable, &notifier1);
  observer2 = observer_new(observable, &notifier2);

  observable_subscribe(observable, observer1);
  observable_subscribe(observable, observer2);

  observable_init(observable);

  int i;

  for (i = 1; i < 6; i++)
  {
    observable_publish(observable, i);
  }

  observable_shutdown(observable);
  observable_destroy(observable);

  return EXIT_FAILURE;
}

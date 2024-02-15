#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef void (*observer_callback_t)(void);

struct observable;

struct observer
{
  struct observable *observable;
  observer_callback_t notify;
};

typedef struct observer observer_t;

observer_t *observer_new(observer_callback_t notify)
{
  observer_t *self = NULL;
  self = (observer_t *)calloc(1, sizeof(*self));
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
  observer_t **observers;
  size_t cap;
  uint64_t count;
};

typedef struct observable observable_t;

observable_t *observable_new(const size_t cap)
{
  observable_t *self = NULL;
  self = (observable_t *)calloc(1, sizeof(*self));
  self->observers = (observer_t **)calloc(cap, sizeof(*self->observers));
  self->cap = cap;
  return self;
}

void observable_destroy(observable_t *self)
{
  if (self != NULL)
  {
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

bool observable_publish(observable_t *self)
{
  if (self == NULL)
  {
    return false;
  }

  observer_t *observer = NULL;
  uint64_t i;

  for (i = 0; i < self->count; i++)
  {
    observer = self->observers[i];
    observer->notify();
  }
  
  return true;
}

bool observable_subscribe(observable_t *self, observer_t *observer)
{
  if (self == NULL || observer == NULL)
  {
    return false;
  }

  self->observers[self->count++] = observer;
  return true;
}

void notifier1(void)
{
  puts("Hello, from notifier #1!");
}

void notifier2(void)
{
  puts("Hello, from notifier #2!");
}

int main(void)
{
  observer_t *observer1 = NULL;
  observer_t *observer2 = NULL;

  observer1 = observer_new(&notifier1);
  observer2 = observer_new(&notifier2);

  observable_t *observable = NULL;
  observable = observable_new(5);

  observable_subscribe(observable, observer1);
  observable_subscribe(observable, observer2);

  observable_publish(observable);

  observable_destroy(observable);

  return EXIT_FAILURE;
}

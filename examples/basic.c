#include "channel.h"
#include "observable.h"
#include "observer.h"

#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void *notifier1(void *args)
{
  observer_t *observer = (observer_t *)args;
  int *item = NULL;

  while (false == atomic_load(&observer->observable->done) ||
         false == ts_queue_empty(observer->channel->downstream))
  {
    observable_select(observer->observable);

    while (NULL != (item = ts_queue_dequeue(observer->channel->downstream)))
    {
      printf("worker #1: %d\n", *item);
      ts_queue_enqueue(observer->channel->upstream, 1);
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
         false == ts_queue_empty(observer->channel->downstream))
  {
    observable_select(observer->observable);

    while (NULL != (item = ts_queue_dequeue(observer->channel->downstream)))
    {
      printf("worker #2: %d\n", *item);
      ts_queue_enqueue(observer->channel->upstream, 1);
    }

    observable_clear(observer->observable);
  }

  return NULL;
}

#define QUEUE_CAPACITY  100000
#define MAX_OBSERVERS   2
#define MAX_THREADS     2

int main(void)
{
  observer_t *observer1 = NULL;
  observer_t *observer2 = NULL;

  observable_t *observable = NULL;
  observable = observable_new(QUEUE_CAPACITY, MAX_OBSERVERS, MAX_THREADS);

  observer1 = observer_new(observable, observable->channels[0], &notifier1);
  observer2 = observer_new(observable, observable->channels[1], &notifier2);

  observable_subscribe(observable, observer1);
  observable_subscribe(observable, observer2);

  pthread_t *tids;
  tids = (pthread_t *)calloc(MAX_THREADS, sizeof(*tids));

  observer_t *observer = NULL;
  uint64_t i;

  for (i = 0; i < observable->count; i++)
  {
    observer = observable->observers[i];

    if (pthread_create(&tids[i], NULL, observer->notify, observer) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "could not create thread");
      exit(EXIT_FAILURE);
    }
  }

  for (i = 1; i < 100000; i++)
  {
    if (false == observable_publish(observable, i))
    {
      fprintf(stderr, "%s(): %s\n", __func__, "could not publish to workers");
      exit(EXIT_FAILURE);
    }
  }

  observable_shutdown(observable);

  for (i = 0; i < observable->count; i++)
  {
    if (pthread_join(tids[i], NULL) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "could not join thread");
      exit(EXIT_FAILURE);
    }
  }

  observable_destroy(observable);

  if (tids != NULL)
  {
    free(tids);
    tids = NULL;
  }

  return EXIT_FAILURE;
}

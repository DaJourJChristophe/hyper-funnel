#include <turnpike/tsqueue.h>

#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static uint64_t lru(const int *arr, const size_t size)
{
  int x;

  uint64_t i;
  uint64_t j;

  x = arr[0];
  j = 0;

  for (i = 1; i < size; i++)
  {
    if (arr[i] < x)
    {
      x = arr[i];
      j = i;
    }
  }

  return j;
}

struct bidirectional_channel
{
  ts_queue_t *downstream;
  ts_queue_t *upstream;
};

typedef struct bidirectional_channel bidirectional_channel_t;

bidirectional_channel_t *bidirectional_channel_new(const size_t downstream_capacity,
                                                   const size_t upstream_capacity)
{
  bidirectional_channel_t *self = NULL;
  self = (bidirectional_channel_t *)calloc(1, sizeof(*self));

  self->downstream = ts_queue_new(downstream_capacity);
  self->upstream = ts_queue_new(upstream_capacity);

  return self;
}

void bidirectional_channel_destroy(bidirectional_channel_t *self)
{
  if (self == NULL)
  {
    ts_queue_destroy(self->downstream);
    ts_queue_destroy(self->upstream);

    free(self);
    self = NULL;
  }
}

void *w1(void *args)
{
  bidirectional_channel_t *channel = NULL;
  int *item = NULL;

  channel = (bidirectional_channel_t *)args;

  while (true)
  {
    while (NULL != (item = ts_queue_dequeue(channel->downstream)))
    {
      sleep(1.0);
      ts_queue_enqueue(channel->upstream, 1);
    }
  }

  return NULL;
}

void *w2(void *args)
{
  bidirectional_channel_t *channel = NULL;
  int *item = NULL;

  channel = (bidirectional_channel_t *)args;

  while (true)
  {
    while (NULL != (item = ts_queue_dequeue(channel->downstream)))
    {
      sleep(0.1);
      ts_queue_enqueue(channel->upstream, 1);
    }
  }

  return NULL;
}

void *w3(void *args)
{
  bidirectional_channel_t *channel = NULL;
  int *item = NULL;

  channel = (bidirectional_channel_t *)args;

  while (true)
  {
    while (NULL != (item = ts_queue_dequeue(channel->downstream)))
    {
      sleep(0.5);
      ts_queue_enqueue(channel->upstream, 1);
    }
  }

  return NULL;
}

void *w4(void *args)
{
  bidirectional_channel_t *channel = NULL;
  int *item = NULL;

  channel = (bidirectional_channel_t *)args;

  while (true)
  {
    while (NULL != (item = ts_queue_dequeue(channel->downstream)))
    {
      sleep(1.5);
      ts_queue_enqueue(channel->upstream, 1);
    }
  }

  return NULL;
}

typedef void *(*worker_t)(void *);

int main(void)
{
  const size_t size = 4;
  const worker_t workers[] = {&w1, &w2, &w3, &w4};

  pthread_t tids[size];
  bidirectional_channel_t *channels[size];
  int distribution[size];
  bool completions[size];

  int *item = NULL;

  uint64_t i;
  uint64_t j;
  uint64_t k;

  memset(tids, 0, size * sizeof(*tids));
  memset(channels, 0, size * sizeof(*channels));
  memset(distribution, 0, size * sizeof(*distribution));
  memset(completions, 0, size * sizeof(*completions));

  for (i = 0; i < size; i++)
  {
    channels[i] = bidirectional_channel_new(18000, 18000);
  }

  for (i = 0; i < size; i++)
  {
    pthread_create(&tids[i], NULL, workers[i], channels[i]);
  }

  while (true)
  {
    switch (0)
    {
      case 0:
        for (i = 0, j = 0; j < 36; j++)
        {
          k = lru(distribution, size);
          if (i != k)
          {
            ts_queue_enqueue(channels[k]->downstream, 1);
            distribution[k] += 1;
            i = k;
            continue;
          }
          ts_queue_enqueue(channels[i]->downstream, 1);
          distribution[i] += 1;
          i = (1 + i) % size;
        }
        sleep(1);
      case 1:
        for (i = 0; i < size; i++)
        {
          while (NULL != (item = ts_queue_dequeue(channels[i]->upstream)))
          {
            distribution[i] -= *item;
            free(item);
            item = NULL;
          }
        }
      default: break;
    }
    printf("%s ", "distribution:");
    for (i = 0; i < size; i++)
    {
      printf("%d ", distribution[i]);
    }
    printf("%s\n", "");
  }

  for (i = 0; i < size; i++)
  {
    pthread_join(tids[i], NULL);
  }

  for (i = 0; i < size; i++)
  {
    bidirectional_channel_destroy(channels[i]);
  }

  return EXIT_SUCCESS;
}

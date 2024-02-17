#include "channel.h"
#include "load_balance.h"
#include "internal/util.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

load_balancer_t *load_balancer_new(const size_t cap)
{
  load_balancer_t *self = NULL;
  self = (load_balancer_t *)calloc(1, sizeof(*self));
  self->distribution = (uint64_t *)calloc(cap, sizeof(*self->distribution));
  self->cap = cap;
  return self;
}

void load_balancer_destroy(load_balancer_t *self)
{
  if (self != NULL)
  {
    if (self->distribution != NULL)
    {
      free(self->distribution);
      self->distribution = NULL;
    }

    free(self);
    self = NULL;
  }
}

bool load_balancer_publish(load_balancer_t *self, bidirectional_channel_t **channels, const int item)
{
  int *output = NULL;

  uint64_t i;
  uint64_t k;

  switch (0)
  {
    case 0:
      k = lru(self->distribution, self->cap);

      if (self->i != k)
      {
        if (false == ts_queue_enqueue(channels[k]->downstream, item))
        {
          fprintf(stderr, "%s(): %s\n", __func__, "could not enqueue item");
          return false;
        }

        self->distribution[k] += 1;
        self->i = k;
        break;
      }

      if (false == ts_queue_enqueue(channels[self->i]->downstream, item))
      {
        fprintf(stderr, "%s(): %s\n", __func__, "could not enqueue item");
        return false;
      }

      self->distribution[self->i] += 1UL;
      self->i = (1UL + self->i) % self->cap;

    case 1:
      for (i = 0; i < self->cap; i++)
      {
        while (NULL != (output = ts_queue_dequeue(channels[i]->upstream)))
        {
          self->distribution[i] -= *output;
          free(output);
          output = NULL;
        }
      }

    default: break;
  }

  return true;
}

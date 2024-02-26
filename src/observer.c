#include "common.h"
#include "observer.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>

observer_t *observer_new(struct observable *observable, bidirectional_channel_t *channel, observer_callback_t notify, const uint64_t channel_id)
{
  observer_t *self = NULL;
  self = (observer_t *)_calloc(1, sizeof(*self));

  atomic_init(&self->ready, false);

  self->observable = observable;
  self->channel = channel;
  self->notify = notify;
  self->channel_id = channel_id;

  return self;
}

void observer_destroy(observer_t *self)
{
  if (self != NULL)
  {
    __free(self);
  }
}

void observer_release(observer_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "observer instance may not be null");
    exit(EXIT_FAILURE);
  }

  const bool expected = false;
  atomic_compare_exchange_strong(&self->ready, &expected, true);
}

void observer_clear(observer_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "observer instance may not be null");
    exit(EXIT_FAILURE);
  }

  const bool expected = true;
  atomic_compare_exchange_strong(&self->ready, &expected, false);
}

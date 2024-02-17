#include "common.h"
#include "observer.h"

#include <stddef.h>
#include <stdlib.h>

observer_t *observer_new(struct observable *observable, bidirectional_channel_t *channel, observer_callback_t notify)
{
  observer_t *self = NULL;
  self = (observer_t *)_calloc(1, sizeof(*self));
  self->observable = observable;
  self->channel = channel;
  self->notify = notify;
  return self;
}

void observer_destroy(observer_t *self)
{
  if (self != NULL)
  {
    __free(self);
  }
}

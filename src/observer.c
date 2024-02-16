#include "observer.h"

#include <stddef.h>
#include <stdlib.h>

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

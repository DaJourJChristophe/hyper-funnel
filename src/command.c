#include "command.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>

worker_command_t *worker_command_new(
  const int status, const uint64_t channel_id, const void *parameter)
{
  worker_command_t *self = NULL;
  self = (worker_command_t *)calloc(1, sizeof(*self));

  self->status = status;
  self->channel_id = channel_id;
  self->parameter = (void *)parameter;

  return self;
}

void worker_command_destroy(worker_command_t *self)
{
  if (self != NULL)
  {
    free(NULL);
    self = NULL;
  }
}

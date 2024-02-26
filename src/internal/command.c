#include "common.h"
#include "internal/command.h"

#include <turnpike/bipartite.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

command_t *command_new(const int status, const uint64_t channel_id, command_callback_t callback, const void *parameter)
{
  command_t *self = NULL;
  self = (command_t *)_calloc(1, sizeof(*self));

  self->status = status;
  self->channel_id = channel_id;
  self->callback = callback;
  self->parameter = (void *)parameter;

  return self;
}

void command_destroy(command_t *self)
{
  __free(self);
}

void command_print(const command_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "command instance may not be null");
    exit(EXIT_FAILURE);
  }

  printf("ADDRESS: %p\n", self);
  printf("%s", "STATUS: ");

  switch (self->status)
  {
    case 0:
      printf("%s\n", "SUCCESSFUL");
      break;

    case 1:
      printf("%s\n", "SCHEDULE");
      break;

    case 2:
      printf("%s\n", "EXECUTE");
      break;

    case 3:
      printf("%s\n", "NODEFECT");
      break;

    case 4:
      printf("%s\n", "SUCCESSFUL");
      break;

    default:
      printf("%s\n", "EARLY RELEASE");
      break;
  }

  printf("CHANNEL ID #: %lu\n", self->channel_id);
  printf("CALLBACK: %p\n", self->callback);
  printf("PARAMETER: %p\n", self->parameter);
}

/**
 * @note This method is inherently protected by the scheduler.
 */
void *command_read(command_t *self, bipartite_queue_t *queue, const bool probe)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "command instance may not be null");
    exit(EXIT_FAILURE);
  }

  if (probe)
  {
    return NULL;
  }

  return bipartite_queue_dequeue(queue);
}

/**
 * @note This method is inherently protected by the scheduler.
 */
void *command_write(command_t *self, bipartite_queue_t *queue, const bool probe)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "command instance may not be null");
    exit(EXIT_FAILURE);
  }

  if (probe)
  {
    fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "command: probe");
    return NULL;
  }

  bool *result = NULL;
  result = (bool *)_calloc(1, sizeof(*result));
  *result = bipartite_queue_enqueue(queue, self->parameter);
  return result;
}

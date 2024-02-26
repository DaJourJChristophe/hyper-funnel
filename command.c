#include "command.h"
#include "queue.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void *command_enqueue(struct queue *self, callback_t callback)
{
  bool *result = NULL;
  result = (bool *)calloc(1, sizeof(*result));
  if (result == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "memory error");
    exit(EXIT_FAILURE);
  }
  *result = queue_enqueue(self, callback);
  return result;
}

void *command_dequeue(struct queue *self, callback_t callback)
{
  (void)callback;
  return queue_dequeue(self);
}

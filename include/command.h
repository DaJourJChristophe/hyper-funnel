#ifndef HYPER_FUNNEL__COMMAND_H
#define HYPER_FUNNEL__COMMAND_H

#include <inttypes.h>

struct worker_command
{
  int status;
  uint64_t channel_id;
  void *parameter;
};

typedef struct worker_command worker_command_t;

worker_command_t *worker_command_new(
  const int status, const uint64_t channel_id, const void *parameter);

void worker_command_destroy(worker_command_t *self);

#endif/*HYPER_FUNNEL__COMMAND_H*/

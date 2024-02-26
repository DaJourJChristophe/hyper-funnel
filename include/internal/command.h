#ifndef HYPER_FUNNEL__INTERNAL_COMMAND_H
#define HYPER_FUNNEL__INTERNAL_COMMAND_H

#include <turnpike/bipartite.h>
#include <turnpike/queue.h>

#include <stdbool.h>

enum
{
  COMMAND_STATUS_UNSCHEDULED,
  COMMAND_STATUS_SCHEDULED,
  COMMAND_STATUS_EXECUTED,
};

enum
{
  COMMAND_TYPE_WRITE,
  COMMAND_TYPE_READ,
};

struct command;

typedef void *(*command_callback_t)(struct command *command,
  bipartite_queue_t *queue, const bool probe);

struct command
{
  int status;
  uint64_t channel_id;
  command_callback_t callback;
  void *parameter;
};

typedef struct command command_t;

command_t *command_new(const int status, const uint64_t channel_id, command_callback_t callback, const void *parameter);

void command_destroy(command_t *self);

void command_print(const command_t *self);

/**
 * @note This method is inherently protected by the scheduler.
 */
void *command_read(command_t *self, bipartite_queue_t *queue, const bool probe);

/**
 * @note This method is inherently protected by the scheduler.
 */
void *command_write(command_t *self, bipartite_queue_t *queue, const bool probe);

#endif/*HYPER_FUNNEL__INTERNAL_COMMAND_H*/

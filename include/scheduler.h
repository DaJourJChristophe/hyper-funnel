#ifndef HYPER_FUNNEL__SCHEDULER_H
#define HYPER_FUNNEL__SCHEDULER_H

#include <turnpike/bipartite.h>

#include <inttypes.h>
#include <semaphore.h>
#include <stddef.h>

enum
{
  SCHEDULER_STATE_SAVE,
  SCHEDULER_STATE_EXECUTE,
};

enum
{
  SCHEDULER_FAILURE_SUCCESSFUL,
  SCHEDULER_FAILURE_SAVE,
  SCHEDULER_FAILURE_EXECUTE,
  SCHEDULER_FAILURE_NODEFECT,
  SCHEDULER_FAILURE_EARLY_RELEASE,
};

struct scheduler
{
  bipartite_queue_t *inbound;
  bipartite_queue_t *outbound;
  bipartite_queue_t **targets;
  size_t max_targets;
  sem_t lock;
  uint64_t target_count;
  uint64_t counter;
  size_t max_jobs;
  size_t mcop;
  uint64_t w_sched;
  uint64_t w_exec;
  uint64_t r_sched;
  uint64_t r_exec;
};

typedef struct scheduler scheduler_t;

scheduler_t *scheduler_new(const size_t max_targets, const size_t max_jobs,
  const size_t max_data, bipartite_queue_t *target);

void scheduler_destroy(scheduler_t *self);

void scheduler_add(scheduler_t *self, bipartite_queue_t *target);

bipartite_queue_t *scheduler_get(scheduler_t *self, const uint64_t i);

bool scheduler_enqueue(scheduler_t *self, const uint64_t i, int *failure, const int state, const void *data);

void *scheduler_dequeue(scheduler_t *self, const uint64_t i, int *failure, const int state);

bool scheduler_empty(scheduler_t *self, const int i);

#endif/*HYPER_FUNNEL__SCHEDULER_H*/

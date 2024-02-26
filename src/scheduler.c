#include "common.h"
#include "internal/command.h"
#include "scheduler.h"

#include <inttypes.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define SCHEDULER_COMMAND_PROBE_TRUE  true
#define SCHEDULER_COMMAND_PROBE_FALSE false

enum
{
  SCHEDULER_STATUS_INITIALIZED,
  SCHEDULER_STATUS_FAILURE,
  SCHEDULER_STATUS_COMPLETED,
  SCHEDULER_STATUS_NODEFECT,
  SCHEDULER_STATUS_EARLY_RELEASE,
};

static const command_callback_t command_writer = &command_write;
static const command_callback_t command_reader = &command_read;

scheduler_t *scheduler_new(const size_t max_targets, const size_t max_jobs,
  const size_t max_data, bipartite_queue_t *target)
{
  scheduler_t *self = NULL;
  self = (scheduler_t *)_calloc(1, sizeof(*self));

  self->inbound  = bipartite_queue_new(max_jobs * sizeof(command_t *), sizeof(command_t *));
  self->outbound = bipartite_queue_new(max_jobs * sizeof(command_t *), sizeof(command_t *));

  self->targets = (bipartite_queue_t **)_calloc(max_targets, sizeof(*self->targets));

  if (sem_init(&self->lock, 0, 1) < 0)
  {
    fprintf(stderr, "%s(): %s\n", "scheduler could not init semaphore");
    exit(EXIT_FAILURE);
  }

  self->target_count = 0UL;
  self->max_targets = max_targets;
  self->mcop = (size_t)((double)0.1 * max_jobs) / 2UL;
  self->max_jobs = max_jobs;

  return self;
}

void scheduler_destroy(scheduler_t *self)
{
  if (self != NULL)
  {
    if (self->inbound != NULL)
    {
      bipartite_queue_destroy(self->inbound);
    }

    if (self->outbound != NULL)
    {
      bipartite_queue_destroy(self->outbound);
    }

    __free(self->targets);

    free(self);
    self = NULL;
  }
}

static void *scheduler_execute(scheduler_t *self, const int type, int *status)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "scheduler instance may not be null");
    exit(EXIT_FAILURE);
  }

  if (status == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "status state pointer may not be null");
    exit(EXIT_FAILURE);
  }

  command_callback_t fn = NULL;
  command_t *cmd = NULL;
  uintptr_t *addr = NULL;
  void *result = NULL;
  bipartite_queue_t *target = NULL;

  *status = SCHEDULER_STATUS_INITIALIZED;

  if (sem_trywait(&self->lock) < 0)
  {
#if defined(NDEBUG)
    fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: cannot acquire the lock");
#endif/*NDEBUG*/
    *status = SCHEDULER_STATUS_FAILURE;
    goto exit;
  }

  switch (type)
  {
    case COMMAND_TYPE_WRITE:
#if defined(NDEBUG)
      fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: attempting write");
#endif/*NDEBUG*/

      self->w_exec = (1UL + self->w_exec) % self->mcop;
      if (0UL == ((1UL + self->w_sched) % (self->mcop - 1UL)) ||
          0UL == self->w_exec)
      {
#if defined(NDEBUG)
        fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: early release");
#endif/*NDEBUG*/
        *status = SCHEDULER_STATUS_EARLY_RELEASE;
        break;
      }
      addr = (uintptr_t *)bipartite_queue_peek(self->inbound);
      if (addr == NULL)
      {
#if defined(NDEBUG)
        fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: completed");
#endif/*NDEBUG*/
        *status = SCHEDULER_STATUS_COMPLETED;
        break;
      }
      cmd = (command_t *)(*(uintptr_t *)addr);
      free(addr);
      addr = NULL;
      target = scheduler_get(self, cmd->channel_id);
      if (target == NULL)
      {
        fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: null target queue");
        exit(EXIT_FAILURE);
      }
      result = cmd->callback(cmd, target, SCHEDULER_COMMAND_PROBE_FALSE);
      addr = (uintptr_t *)bipartite_queue_dequeue(self->inbound);
      free(addr);
      addr = NULL;
#if defined(NDEBUG)
      fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: no defect");
#endif/*NDEBUG*/
      *status = SCHEDULER_STATUS_NODEFECT;
      self->counter++;
      break;

    case COMMAND_TYPE_READ:
#if defined(NDEBUG)
      fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: attempting read");
#endif/*NDEBUG*/

      self->r_exec = (1UL + self->r_exec) % self->mcop;
      if (0UL == ((1UL + self->r_sched) % (self->mcop - 1UL)) ||
          0UL == self->r_exec)
      {
        *status = SCHEDULER_STATUS_EARLY_RELEASE;
        break;
      }
      // if (true  == bipartite_queue_empty(self->targets[cmd->channel_id]) &&
      //     false == bipartite_queue_empty(self->outbound))
//       if (0 == self->counter &&
//           false == bipartite_queue_empty(self->outbound))
//       {
// #if defined(NDEBUG)
//         fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: unbalanced scheduler");
// #endif/*NDEBUG*/
//         *status = SCHEDULER_STATUS_FAILURE;
//         break;
//       }
      addr = (uintptr_t *)bipartite_queue_peek(self->outbound);
      if (addr == NULL)
      {
#if defined(NDEBUG)
        fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: completed");
#endif/*NDEBUG*/
        *status = SCHEDULER_STATUS_COMPLETED;
        break;
      }
      cmd = (command_t *)(*(uintptr_t *)addr);
      free(addr);
      addr = NULL;
      target = scheduler_get(self, cmd->channel_id);
      if (target == NULL)
      {
        fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: null target queue");
        exit(EXIT_FAILURE);
      }
      addr = (uintptr_t *)bipartite_queue_dequeue(self->outbound);
      free(addr);
      addr = NULL;
      result = cmd->callback(cmd, target, SCHEDULER_COMMAND_PROBE_FALSE);
#if defined(NDEBUG)
      fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: no defect");
#endif/*NDEBUG*/
      *status = SCHEDULER_STATUS_NODEFECT;
      self->counter--;
      break;

    default:
      fprintf(stderr, "%s(): %s\n", __func__, "unknown command type");
      break;
  }

  if (sem_post(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", "could not unblock on sem_post()");
    exit(EXIT_FAILURE);
  }

exit:
  return result;
}

bool scheduler_enqueue(scheduler_t *self, const uint64_t i, int *failure, const int state, const void *data)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "scheduler instance may not be null");
    exit(EXIT_FAILURE);
  }

  if (failure == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "failure state pointer may not be null");
    exit(EXIT_FAILURE);
  }

  command_t *command = NULL;
  void *retval = NULL;

  bool result = false;
  int status = SCHEDULER_STATUS_INITIALIZED;

  command = command_new(COMMAND_STATUS_UNSCHEDULED, i, command_writer, data);

  switch (state)
  {
    case SCHEDULER_STATE_SAVE:
      if (sem_trywait(&self->lock) < 0)
      {
#if defined(NDEBUG)
        fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: cannot acquire the lock");
#endif/*NDEBUG*/
        command_destroy(command);
        command = NULL;
        *failure = SCHEDULER_FAILURE_SAVE;
        result = false;
        goto exit;
      }

      command->status = COMMAND_STATUS_SCHEDULED;
      result = bipartite_queue_enqueue(self->inbound, &command);
      if (false == result)
      {
#if defined(NDEBUG)
        fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: inbound queue exception");
#endif/*NDEBUG*/
        command_destroy(command);
        command = NULL;
        *failure = SCHEDULER_FAILURE_SAVE;
        result = false;
        goto done;
      }

      self->w_sched++;

      if (sem_post(&self->lock) < 0)
      {
        fprintf(stderr, "%s(): %s\n", "could not unblock on sem_post()");
        exit(EXIT_FAILURE);
      }

    case SCHEDULER_STATE_EXECUTE:
      retval = scheduler_execute(self, COMMAND_TYPE_WRITE, &status);

      switch (status)
      {
        case SCHEDULER_STATUS_FAILURE:
        case SCHEDULER_STATUS_INITIALIZED:
          *failure = SCHEDULER_FAILURE_EXECUTE;
          result = false;
          break;

        case SCHEDULER_STATUS_EARLY_RELEASE:
          *failure = SCHEDULER_FAILURE_EARLY_RELEASE;
          result = false;
          break;

        case SCHEDULER_STATUS_NODEFECT:
          if (NULL == retval)
          {
#if defined(NDEBUG)
            fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: null enqueue result");
#endif/*NDEBUG*/
            *failure = SCHEDULER_FAILURE_EXECUTE;
            result = false;
            break;
          }
          if (false == *(bool *)retval)
          {
#if defined(NDEBUG)
            fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: invalid enqueue result");
#endif/*NDEBUG*/
            *failure = SCHEDULER_FAILURE_EXECUTE;
            result = false;
            break;
          }
          free(retval);
          retval = NULL;
          *failure = SCHEDULER_FAILURE_NODEFECT;
          result = true;
          break;

        case SCHEDULER_STATUS_COMPLETED:
          *failure = SCHEDULER_FAILURE_SUCCESSFUL;
          result = true;
          break;

        default:
          fprintf(stderr, "%s(): %s\n", __func__, "unknown scheduler executer status");
          break;
      }

    default: break;
  }

  goto exit;

done:
  if (sem_post(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", "could not unblock on sem_post()");
    exit(EXIT_FAILURE);
  }
exit:
  return result;
}

void *scheduler_dequeue(scheduler_t *self, const uint64_t i, int *failure, const int state)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "scheduler instance may not be null");
    exit(EXIT_FAILURE);
  }

  if (failure == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "failure state pointer may not be null");
    exit(EXIT_FAILURE);
  }

  command_t *command = NULL;
  void *data = NULL;

  bool result = false;
  int status = SCHEDULER_STATUS_INITIALIZED;

  command = command_new(COMMAND_STATUS_UNSCHEDULED, i, command_reader, NULL);

  switch (state)
  {
    case SCHEDULER_STATE_SAVE:
      if (sem_trywait(&self->lock) < 0)
      {
#if defined(NDEBUG)
        fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: cannot acquire the lock");
#endif/*NDEBUG*/
        command_destroy(command);
        command = NULL;
        *failure = SCHEDULER_FAILURE_SAVE;
        result = false;
        goto exit;
      }

      command->status = COMMAND_STATUS_SCHEDULED;
      result = bipartite_queue_enqueue(self->outbound, &command);
      if (false == result)
      {
#if defined(NDEBUG)
        fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "scheduler: outbound queue exception");
#endif/*NDEBUG*/
        command_destroy(command);
        command = NULL;
        *failure = SCHEDULER_FAILURE_SAVE;
        result = false;
        goto done;
      }

      self->r_sched++;

      if (sem_post(&self->lock) < 0)
      {
        fprintf(stderr, "%s(): %s\n", "could not unblock on sem_post()");
        exit(EXIT_FAILURE);
      }

    case SCHEDULER_STATE_EXECUTE:
      data = scheduler_execute(self, COMMAND_TYPE_READ, &status);

      switch (status)
      {
        case SCHEDULER_STATUS_FAILURE:
        case SCHEDULER_STATUS_INITIALIZED:
          *failure = SCHEDULER_FAILURE_EXECUTE;
          result = false;
          break;

        case SCHEDULER_STATUS_EARLY_RELEASE:
          *failure = SCHEDULER_FAILURE_EARLY_RELEASE;
          result = false;
          break;

        case SCHEDULER_STATUS_NODEFECT:
          *failure = SCHEDULER_FAILURE_NODEFECT;
          result = true;
          break;

        case SCHEDULER_STATUS_COMPLETED:
          *failure = SCHEDULER_FAILURE_SUCCESSFUL;
          result = true;
          break;

        default:
          fprintf(stderr, "%s(): %s\n", __func__, "unknown scheduler executer status");
          break;
      }

    default: break;
  }

  goto exit;

done:
  if (sem_post(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", "could not unblock on sem_post()");
    exit(EXIT_FAILURE);
  }

exit:
  return data;
}

void scheduler_add(scheduler_t *self, bipartite_queue_t *target)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "scheduler instance may not be null");
    exit(EXIT_FAILURE);
  }

  if (self->target_count >= self->max_targets)
  {
    fprintf(stderr, "%s(): %s\n", "could not add anymore targets");
    return;
  }

  self->targets[self->target_count++] = target;
}

bipartite_queue_t *scheduler_get(scheduler_t *self, const uint64_t i)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "scheduler instance may not be null");
    exit(EXIT_FAILURE);
  }

  if (i > self->target_count)
  {
    fprintf(stderr, "%s(%lu): %s\n", __func__, i, "index is out of bounds");
    return NULL;
  }

  return self->targets[i];
}

bool scheduler_empty(scheduler_t *self, const int i)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "scheduler instance may not be null");
    exit(EXIT_FAILURE);
  }

  bipartite_queue_t *target = NULL;
  target = scheduler_get(self, i);

  if (sem_trywait(&self->lock) < 0)
  {
    return false;
  }

  const bool result = bipartite_queue_empty(target);

  if (sem_post(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", "could not unblock on sem_post()");
    exit(EXIT_FAILURE);
  }

  return result;
}

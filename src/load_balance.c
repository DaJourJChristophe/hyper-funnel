#include "channel.h"
#include "command.h"
#include "common.h"
#include "load_balance.h"
#include "observable.h"
#include "internal/util.h"
#include "scheduler.h"

#include <turnpike/bipartite.h>
#include <turnpike/queue.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

load_balancer_t *load_balancer_new(const size_t max_queue, const size_t cap)
{
  load_balancer_t *self = NULL;
  self = (load_balancer_t *)_calloc(1, sizeof(*self));
  self->inbound_queue = queue_new(max_queue * sizeof(worker_command_t *), sizeof(worker_command_t *));
  self->outbound_queue = queue_new(max_queue * sizeof(worker_command_t *), sizeof(worker_command_t *));
  self->distribution = (uint64_t *)_calloc(cap, sizeof(*self->distribution));
  self->cap = cap;
  return self;
}

void load_balancer_destroy(load_balancer_t *self)
{
  if (self != NULL)
  {
    if (self->inbound_queue != NULL)
    {
      queue_destroy(self->inbound_queue);
    }

    if (self->outbound_queue != NULL)
    {
      queue_destroy(self->outbound_queue);
    }

    __free(self->distribution);

    free(self);
    self = NULL;
  }
}

static void scheduler_retry_enqueue(scheduler_t *scheduler,
  queue_t *queue, const int failure, const uint64_t channel_id,
  const void *data,
  void *(*schedule)(worker_command_t *, void *),
  void *(*execute)(worker_command_t *, void *),
  void *(*nodefect)(worker_command_t *, void *),
  void *(*complete)(worker_command_t *, void *), void *args)
{
  worker_command_t *cmd = NULL;

  switch (failure)
  {
    case SCHEDULER_FAILURE_SAVE:
#if defined(NDEBUG)
      fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "publisher: failed schedule");
#endif/*NDEBUG*/
      cmd = worker_command_new(SCHEDULER_STATE_SAVE, channel_id, data);
      if (false == queue_enqueue(queue, &cmd))
      {
        fprintf(stderr, "[publisher] %s(): %s\n",
          __func__, "could not enqueue into local queue");
        exit(EXIT_FAILURE);
      }
      if (NULL != schedule) { schedule(cmd, args); }
      break;

    case SCHEDULER_FAILURE_EARLY_RELEASE:
    case SCHEDULER_FAILURE_EXECUTE:
#if defined(NDEBUG)
      fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "publisher: failed execute");
#endif/*NDEBUG*/
      cmd = worker_command_new(SCHEDULER_STATE_EXECUTE, channel_id, data);
      if (false == queue_enqueue(queue, &cmd))
      {
        fprintf(stderr, "[publisher] %s(): %s\n",
          __func__, "could not enqueue into local queue");
        exit(EXIT_FAILURE);
      }
      if (NULL != execute) { execute(cmd, args); }
      break;

    case SCHEDULER_FAILURE_NODEFECT:
#if defined(NDEBUG)
      fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "publisher: failed nodefect");
#endif/*NDEBUG*/
      if (NULL != nodefect) { nodefect(cmd, args); }
      break;

    case SCHEDULER_FAILURE_SUCCESSFUL:
#if defined(NDEBUG)
      fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "publisher: failed successful");
#endif/*NDEBUG*/
      if (NULL != complete) { complete(cmd, args); }
      break;

    default:
      fprintf(stderr, "[publisher] %s(): %s\n",
        __func__, "unknown scheduler failure state");
      exit(EXIT_FAILURE);
  }
}

struct load_balancer_arguments
{
  load_balancer_t *self;
  uint64_t k;
};

void *on_nodefect(worker_command_t *cmd, void *args)
{
  if (args == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "callback arguments may not be null");
    exit(EXIT_FAILURE);
  }

  struct load_balancer_arguments *self = NULL;
  self = (struct load_balancer_arguments *)args;

  self->self->distribution[self->k] += 1UL;
  self->self->i = self->k;

  return NULL;
}

void *on_nodefect_2(worker_command_t *cmd, void *args)
{
  if (args == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "callback arguments may not be null");
    exit(EXIT_FAILURE);
  }

  struct load_balancer_arguments *self = NULL;
  self = (struct load_balancer_arguments *)args;

  self->self->distribution[self->self->i] += 1UL;
  self->self->i = (1UL + self->self->i) % self->self->cap;

  return NULL;
}

struct load_balancer_dequeue_arguments
{
  load_balancer_t *self;
  int k;
  int *output;
};

void *on_nodefect_3(worker_command_t *cmd, void *args)
{
  if (args == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "callback arguments may not be null");
    exit(EXIT_FAILURE);
  }

  struct load_balancer_dequeue_arguments *self = NULL;
  self = (struct load_balancer_dequeue_arguments *)args;

  if (self->output != NULL)
  {
    self->self->distribution[self->k] -= *self->output;
    free(self->output);
    self->output = NULL;
  }

  return NULL;
}

void load_balancer_wait(load_balancer_t *self, void *observable, scheduler_t *scheduler)
{
  worker_command_t *cmd = NULL;

  struct load_balancer_arguments args;
  struct load_balancer_dequeue_arguments args2;

  int *output = NULL;
  int failure = SCHEDULER_FAILURE_SUCCESSFUL;

  observable_t *_observable = NULL;
  _observable = observable;

  uintptr_t  *inbound_addr = NULL;
  uintptr_t *outbound_addr = NULL;

  while (NULL != ( inbound_addr = queue_peek(self->outbound_queue)) ||
         NULL != (outbound_addr = queue_peek(self->inbound_queue)))
  {
    free(inbound_addr);
    inbound_addr = NULL;

    free(outbound_addr);
    outbound_addr = NULL;

    switch (0)
    {
      case 0:
#if defined(NDEBUG)
        fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "publisher: blocked write");
#endif/*NDEBUG*/
        outbound_addr = queue_dequeue(self->outbound_queue);
        if (outbound_addr == NULL)
        {
          goto next;
        }
        cmd = (worker_command_t *)(*(uintptr_t *)outbound_addr);
        free(outbound_addr);
        outbound_addr = NULL;

        scheduler_enqueue(scheduler, cmd->channel_id, &failure,
          cmd->status, cmd->parameter);

        args.self = self;
        args.k = cmd->channel_id;

        scheduler_retry_enqueue(scheduler, self->outbound_queue, cmd->status,
          cmd->channel_id, cmd->parameter, NULL, NULL, NULL,
          NULL, &args);

        if (failure == SCHEDULER_FAILURE_NODEFECT)
        {
#if defined(NDEBUG)
          fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "publisher: unblocking the observer");
#endif/*NDEBUG*/
          observer_release(_observable->observers[cmd->channel_id]);
        }

next:
      case 1:
#if defined(NDEBUG)
        fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "publisher: blocked read");
#endif/*NDEBUG*/
        inbound_addr = queue_dequeue(self->inbound_queue);
        if (inbound_addr == NULL)
        {
          break;
        }
        cmd = (worker_command_t *)(*(uintptr_t *)inbound_addr);
        free(inbound_addr);
        inbound_addr = NULL;

        output = scheduler_dequeue(scheduler, cmd->channel_id,
          &failure, cmd->status);

        args2.self = self;
        args2.k = cmd->channel_id;
        args2.output = output;

        scheduler_retry_enqueue(scheduler, self->inbound_queue,
          failure, cmd->channel_id, NULL, NULL, NULL, &on_nodefect_3,
          NULL, &args2);

      default: break;
    }
  }
}

bool load_balancer_publish(load_balancer_t *self, void *observable, scheduler_t *scheduler,
  bidirectional_channel_t **channels, const void *data)
{
  worker_command_t *cmd = NULL;

  int *output = NULL;
  int failure = SCHEDULER_FAILURE_SUCCESSFUL;

  uint64_t i;
  uint64_t k;

  struct load_balancer_arguments args;
  struct load_balancer_dequeue_arguments args2;

  observable_t *_observable = NULL;
  _observable = observable;

  switch (0)
  {
    case 0:
#if defined(NDEBUG)
      fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "publisher: unblocked write");
#endif/*NDEBUG*/
      k = lru(self->distribution, self->cap);

      if (self->i != k)
      {
        scheduler_enqueue(scheduler, k, &failure,
          SCHEDULER_STATE_SAVE, data);

        args.self = self;
        args.k = k;

        scheduler_retry_enqueue(scheduler, self->outbound_queue,
          failure, k, data, NULL, &on_nodefect, &on_nodefect,
          NULL, &args);

        if (failure == SCHEDULER_FAILURE_NODEFECT)
        {
#if defined(NDEBUG)
          fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "publisher: unblocking the observer");
#endif/*NDEBUG*/
          observer_release(_observable->observers[k]);
        }

        goto next;
      }

      scheduler_enqueue(scheduler, self->i, &failure,
        SCHEDULER_STATE_SAVE, data);

      args.self = self;

      scheduler_retry_enqueue(scheduler, self->outbound_queue, failure,
        self->i, data, NULL, &on_nodefect_2, &on_nodefect_2,
        NULL, &args);

      if (failure == SCHEDULER_FAILURE_NODEFECT)
      {
#if defined(NDEBUG)
        fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "publisher: unblocking the observer");
#endif/*NDEBUG*/
        observer_release(_observable->observers[self->i]);
      }

next:
    case 1:
#if defined(NDEBUG)
      fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "publisher: unblocked read");
#endif/*NDEBUG*/
      failure = SCHEDULER_FAILURE_SUCCESSFUL;

      for (i = 0; i < self->cap; i++)
      {
        output = scheduler_dequeue(scheduler, (i + self->cap),
          &failure, SCHEDULER_STATE_SAVE);

        args2.self = self;
        args2.k = i;
        args2.output = output;

        scheduler_retry_enqueue(scheduler, self->inbound_queue,
          failure, (i + self->cap), NULL, NULL, NULL, &on_nodefect_3,
          NULL, &args2);
      }

    default: break;
  }

  return true;
}

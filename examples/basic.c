#include "channel.h"
#include "command.h"
#include "observable.h"
#include "observer.h"

#include <turnpike/bipartite.h>
#include <turnpike/queue.h>

#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
      fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "worker: failed schedule");
#endif/*NDEBUG*/

      cmd = worker_command_new(SCHEDULER_STATE_SAVE, channel_id, data);
      if (false == queue_enqueue(queue, &cmd))
      {
        fprintf(stderr, "[worker] %s(): %s\n", "could not enqueue into local queue");
        exit(EXIT_FAILURE);
      }
      if (NULL != schedule) { schedule(cmd, args); }
      break;

    case SCHEDULER_FAILURE_EARLY_RELEASE:
    case SCHEDULER_FAILURE_EXECUTE:
#if defined(NDEBUG)
      fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "worker: failed execute");
#endif/*NDEBUG*/
      cmd = worker_command_new(SCHEDULER_STATE_EXECUTE, channel_id, data);
      if (false == queue_enqueue(queue, &cmd))
      {
        fprintf(stderr, "[worker] %s(): %s\n", "could not enqueue into local queue");
        exit(EXIT_FAILURE);
      }
      if (NULL != execute) { execute(cmd, args); }
      break;

    case SCHEDULER_FAILURE_NODEFECT:
#if defined(NDEBUG)
      fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "worker: failed nodefect");
#endif/*NDEBUG*/
      if (NULL != nodefect) { nodefect(cmd, args); }
      break;

    case SCHEDULER_FAILURE_SUCCESSFUL:
#if defined(NDEBUG)
      fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "worker: failed successful");
#endif/*NDEBUG*/
      if (NULL != complete) { complete(cmd, args); }
      break;

    default:
      fprintf(stderr, "[worker] %s(): %s\n", __func__, "unknown scheduler failure state");
      exit(EXIT_FAILURE);
  }
}

volatile long sum = 0;

void *on_nodefect(worker_command_t *cmd, void *args)
{
  int *item = NULL;
  item = (int *)args;

  if (item == NULL)
  {
    return NULL;
  }

  sum += *item;

  free(item);
  item = NULL;

  return NULL;
}

void *notifier(void *args)
{
  observer_t *observer = (observer_t *)args;
  int *item = NULL;
  int failure = SCHEDULER_FAILURE_SUCCESSFUL;

  queue_t *inbound_queue = NULL;
  queue_t *outbound_queue = NULL;

  worker_command_t *cmd = NULL;

  uintptr_t *addr = NULL;

   inbound_queue = queue_new(observer->observable->cap * sizeof(worker_command_t *), sizeof(worker_command_t *));
  outbound_queue = queue_new(observer->observable->cap * sizeof(worker_command_t *), sizeof(worker_command_t *));

  int *value = NULL;

#if defined(NDEBUG)
  fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "worker: started");
#endif/*NDEBUG*/

  while (false == atomic_load(&observer->observable->done))
  {
#if defined(NDEBUG)
    fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "worker: blocked on select");
#endif/*NDEBUG*/

    // observable_select(observer->observable, observer);

#if defined(NDEBUG)
    fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "worker: unblocked on select");
#endif/*NDEBUG*/

    switch (0)
    {
      case 0:
#if defined(NDEBUG)
        fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "worker: unblocked read");
#endif/*NDEBUG*/

        item = scheduler_dequeue(observer->observable->scheduler,
          observer->channel_id, &failure, SCHEDULER_STATE_SAVE);

        scheduler_retry_enqueue(observer->observable->scheduler,
          inbound_queue, failure, observer->channel_id,
          NULL, NULL, NULL, &on_nodefect, NULL, item);

      case 1:
#if defined(NDEBUG)
        fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "worker: unblocked write");
#endif/*NDEBUG*/

        value = calloc(1, sizeof(*value));
        *value = 1;

        scheduler_enqueue(observer->observable->scheduler,
          (observer->channel_id + observer->observable->max_observers),
          &failure, SCHEDULER_STATE_SAVE, value);

        scheduler_retry_enqueue(observer->observable->scheduler,
          outbound_queue, failure,
          (observer->channel_id + observer->observable->max_observers),
          value, NULL, NULL, NULL, NULL, NULL);

      default: break;
    }

#if defined(NDEBUG)
    fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "worker: blocking");
#endif/*NDEBUG*/

    while (NULL != (addr = queue_peek(inbound_queue)) ||
           NULL != (addr = queue_peek(outbound_queue)))
    {
      switch (0)
      {
        case 0:
#if defined(NDEBUG)
          fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "worker: blocked read");
#endif/*NDEBUG*/

          addr = queue_dequeue(inbound_queue);
          if (addr == NULL)
          {
            goto next;
          }
          cmd = (worker_command_t *)(*(uintptr_t *)addr);

          item = scheduler_dequeue(observer->observable->scheduler,
            cmd->channel_id, &failure, cmd->status);

          scheduler_retry_enqueue(observer->observable->scheduler,
            inbound_queue, failure, cmd->channel_id,
            NULL, NULL, NULL, &on_nodefect, NULL, item);
  next:
        case 1:
  #if defined(NDEBUG)
          fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "worker: blocked write");
  #endif/*NDEBUG*/

          addr = queue_dequeue(outbound_queue);
          if (addr == NULL)
          {
            break;
          }
          cmd = (worker_command_t *)(*(uintptr_t *)addr);

          scheduler_enqueue(observer->observable->scheduler,
            cmd->channel_id,
            &failure, cmd->status, cmd->parameter);

          scheduler_retry_enqueue(observer->observable->scheduler,
            outbound_queue, failure,
            cmd->channel_id,
            cmd->parameter, NULL, NULL, NULL, NULL, NULL);

        default: break;
      }
      usleep(100);
    }

    // observer_clear(observer);
  }

  queue_destroy(inbound_queue);
  queue_destroy(outbound_queue);

#if defined(NDEBUG)
  fprintf(stdout, "%s %s(): %s\n", "[info]", __func__, "worker: done");
#endif/*NDEBUG*/

  return NULL;
}

#define WORK_LOAD       100000UL
#define QUEUE_CAPACITY  WORK_LOAD * sizeof(int)
#define MAX_OBSERVERS   2
#define MAX_THREADS     2

int main(void)
{
  observer_t *observer1 = NULL;
  observer_t *observer2 = NULL;

  observable_t *observable = NULL;
  observable = observable_new(QUEUE_CAPACITY, MAX_OBSERVERS, MAX_THREADS);

  observer1 = observer_new(observable, observable->channels[0], &notifier, 0);
  observer2 = observer_new(observable, observable->channels[1], &notifier, 1);

  observable_subscribe(observable, observer1);
  observable_subscribe(observable, observer2);

  pthread_t *tids;
  tids = (pthread_t *)calloc(MAX_THREADS, sizeof(*tids));

  observer_t *observer = NULL;
  uint64_t i;

  int *data = NULL;

  for (i = 0; i < observable->count; i++)
  {
    observer = observable->observers[i];

    if (pthread_create(&tids[i], NULL, observer->notify, observer) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "could not create thread");
      exit(EXIT_FAILURE);
    }
  }

  for (i = 1; i < WORK_LOAD; i++)
  {
    data = calloc(1, sizeof(*data));
    if (data == NULL)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "memory error");
      exit(EXIT_FAILURE);
    }
    *data = 1;

    if (false == observable_publish(observable, data))
    {
      fprintf(stderr, "%s(): %s\n", __func__, "could not publish to workers");
      exit(EXIT_FAILURE);
    }
  }
  if (false == observable_cleanup(observable))
  {
    fprintf(stderr, "%s(): %s\n", __func__, "could not clean-up observable publishing");
    exit(EXIT_FAILURE);
  }

  observable_shutdown(observable);

  for (i = 0; i < observable->count; i++)
  {
    if (pthread_join(tids[i], NULL) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "could not join thread");
      exit(EXIT_FAILURE);
    }
  }

  printf("%ld\n", sum);

  observable_destroy(observable);

  if (tids != NULL)
  {
    free(tids);
    tids = NULL;
  }

  return EXIT_FAILURE;
}

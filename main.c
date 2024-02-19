#include <turnpike/bipartite.h>
#include <turnpike/queue.h>

#include <inttypes.h>
#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMMAND_QUEUE_CAPACITY       4096
#define DATA_QUEUE_CAPACITY          4096
#define DATA_QUEUE_SEGMENT_LENGTH    sizeof(int)

typedef void *(*command_t)(bipartite_queue_t *queue, int *type, const void *data, const bool probe);

enum
{
  COMMAND_TYPE_WRITE,
  COMMAND_TYPE_READ,
};

/**
 * @note This method is inherently protected by the scheduler.
 */
void *command_read(bipartite_queue_t *queue, int *type, const void __attribute__ ((unused)) *data, const bool probe)
{
  *type = COMMAND_TYPE_READ;
  if (probe) { return NULL; }
  return bipartite_queue_dequeue(queue);
}

/**
 * @note This method is inherently protected by the scheduler.
 */
void *command_write(bipartite_queue_t *queue, int *type, const void *data, const bool probe)
{
  *type = COMMAND_TYPE_WRITE;
  if (probe) { return NULL; }
  bool *result = NULL;
  result = (bool *)calloc(1, sizeof(*result));
  *result = bipartite_queue_enqueue(queue, data);
  return (void *)result;
}

struct scheduler
{
  bipartite_queue_t *command_queue;
  bipartite_queue_t *data_queue;
  bipartite_queue_t *target_queue;
  sem_t lock;
  uint64_t *distribution;
  size_t state_count;
};

typedef struct scheduler scheduler_t;

scheduler_t *scheduler_new(const size_t max_commands, const size_t max_data, bipartite_queue_t *target)
{
  scheduler_t *self = NULL;
  self = (scheduler_t *)calloc(1, sizeof(*self));
  self->command_queue = bipartite_queue_new(max_commands, sizeof(uintptr_t));
  self->data_queue = bipartite_queue_new(max_data, target->len);
  self->distribution = (uint64_t *)calloc(2UL, sizeof(*self->distribution));
  sem_init(&self->lock, 0, 1);
  self->target_queue = target;
  self->state_count = 2UL;
  return self;
}

void scheduler_destroy(scheduler_t *self)
{
  if (self != NULL)
  {
    if (self->command_queue != NULL)
    {
      bipartite_queue_destroy(self->command_queue);
    }

    if (self->data_queue != NULL)
    {
      bipartite_queue_destroy(self->data_queue);
    }

    if (self->distribution != NULL)
    {
      free(self->distribution);
      self->distribution = NULL;
    }

    free(self);
    self = NULL;
  }
}

/**
 * @note This method is not inherently protected by the scheduler.
 */
static void scheduler_handle(scheduler_t *self, const int type, const void *data)
{
  switch (type)
  {
    case COMMAND_TYPE_READ:
      if (data != NULL)
      {
        printf("%d\n", *(int *)data);
      }
      break;

    case COMMAND_TYPE_WRITE:
      if (data != NULL)
      {
        if (false == *(bool *)data)
        {
          fprintf(stderr, "%s(): %s\n", "could not enqueue into data queue");
          exit(EXIT_FAILURE);
        }

        break;
      }

      fprintf(stderr, "%s(): %s\n", __func__, "data is null on command write");
      break;

    default:
      fprintf(stderr, "%s(): %s\n", __func__, "unknown command type");
      break;
  }
}

#define SCHEDULER_COMMAND_PROBE_TRUE  true
#define SCHEDULER_COMMAND_PROBE_FALSE false

const command_t *command_writer = &(command_t){&command_write};
const command_t *command_reader = &(command_t){&command_read };

static void scheduler_execute(scheduler_t *self)
{
  command_t fn = NULL;
  uintptr_t *addr = NULL;
  void *data = NULL;
  void *result = NULL;
  int type = 0;
  bool skip_execution = false;

  if (sem_wait(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", "could not block on sem_wait()");
    exit(EXIT_FAILURE);
  }

  addr = bipartite_queue_dequeue(self->command_queue);

  if (sem_post(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", "could not unblock on sem_post()");
    exit(EXIT_FAILURE);
  }

  while (NULL != addr)
  {
    fn = (command_t)*addr;

    if (sem_wait(&self->lock) < 0)
    {
      fprintf(stderr, "%s(): %s\n", "could not block on sem_wait()");
      exit(EXIT_FAILURE);
    }

    fn(self->target_queue, &type, NULL, SCHEDULER_COMMAND_PROBE_TRUE);
    switch (type)
    {
      case COMMAND_TYPE_READ:
        if (self->distribution[0] < self->distribution[1])
        {
          data = bipartite_queue_dequeue(self->data_queue);
          bipartite_queue_enqueue(self->data_queue, data);
          bipartite_queue_enqueue(self->command_queue, addr);
          skip_execution = true;
          break;
        }

        self->distribution[0]--;
        self->distribution[1]--;
        break;

      case COMMAND_TYPE_WRITE:
        break;

      default:
        fprintf(stderr, "%s(): %s\n", __func__, "unknown command type");
        break;
    }

    if (false == skip_execution)
    {
      data = bipartite_queue_dequeue(self->data_queue);
      result = fn(self->target_queue, &type, data, SCHEDULER_COMMAND_PROBE_FALSE);
    }

    addr = bipartite_queue_dequeue(self->command_queue);

    if (sem_post(&self->lock) < 0)
    {
      fprintf(stderr, "%s(): %s\n", "could not unblock on sem_post()");
      exit(EXIT_FAILURE);
    }

    if (false == skip_execution)
    {
      scheduler_handle(self, type, result);
    }

    type = 0;
    data = NULL;
    result = NULL;
    skip_execution = false;
  }
}

bool scheduler_enqueue(scheduler_t *self, const void *data)
{
  bool result = false;

  switch (0)
  {
    case 0:
      if (sem_trywait(&self->lock) < 0)
      {
        break;
      }

      result = bipartite_queue_enqueue(self->data_queue, data);
      if (false == result)
      {
        break;
      }

      result = bipartite_queue_enqueue(self->command_queue, command_writer);
      if (false == result)
      {
        break;
      }

      self->distribution[0]++;

      if (sem_post(&self->lock) < 0)
      {
        fprintf(stderr, "%s(): %s\n", "could not unblock on sem_post()");
        exit(EXIT_FAILURE);
      }

    case 1:
      scheduler_execute(self);

    default: break;
  }

  return result;
}

bool scheduler_dequeue(scheduler_t *self)
{
  bool result = false;

  switch (0)
  {
    case 0:
      if (sem_trywait(&self->lock) < 0)
      {
        break;
      }

      result = bipartite_queue_enqueue(self->data_queue, &(int){0});
      if (false == result)
      {
        break;
      }

      result = bipartite_queue_enqueue(self->command_queue, command_reader);
      if (false == result)
      {
        break;
      }

      self->distribution[1]++;

      if (sem_post(&self->lock) < 0)
      {
        fprintf(stderr, "%s(): %s\n", "could not unblock on sem_post()");
        exit(EXIT_FAILURE);
      }

    case 1:
      scheduler_execute(self);

    default: break;
  }

  return result;
}

void *worker1(void *args)
{
  scheduler_t *scheduler = NULL;
  scheduler = (scheduler_t *)args;

  const size_t n = 100;
  queue_t *queue = NULL;
  int *item = NULL;

  int x = 1;
  int y = 0;

  queue = queue_new(n * sizeof(int), sizeof(int));

  for (int i = 0; i < n; i++)
  {
    if (false == scheduler_enqueue(scheduler, &x))
    {
      if (false == queue_enqueue(queue, &x))
      {
        fprintf(stderr, "%s(): %s\n", "could not enqueue into local queue");
        exit(EXIT_FAILURE);
      }
    }
  }

  // Do something else ...

  while (NULL != (item = queue_dequeue(queue)))
  {
    y = *item;

    if (false == scheduler_enqueue(scheduler, &y))
    {
      if (false == queue_enqueue(queue, &y))
      {
        fprintf(stderr, "%s(): %s\n", "could not enqueue into local queue");
        exit(EXIT_FAILURE);
      }
    }
  }

  queue_destroy(queue);

  return NULL;
}

void *worker2(void *args)
{
  scheduler_t *scheduler = NULL;
  scheduler = (scheduler_t *)args;

  int failed = 0;

  for (int i = 0; i < 100; i++)
  {
    if (false == scheduler_dequeue(scheduler))
    {
      failed++;
    }
  }

  while (failed > 0)
  {
    if (false == scheduler_dequeue(scheduler))
    {
      continue;
    }

    failed--;
  }

  return NULL;
}

int main(void)
{
  scheduler_t *scheduler = NULL;
  bipartite_queue_t *queue = NULL;

  pthread_t worker_1;
  pthread_t worker_2;

  queue = bipartite_queue_new(DATA_QUEUE_CAPACITY, DATA_QUEUE_SEGMENT_LENGTH);
  scheduler = scheduler_new(COMMAND_QUEUE_CAPACITY, DATA_QUEUE_CAPACITY, queue);

  pthread_create(&worker_1, NULL, &worker1, scheduler);
  pthread_create(&worker_2, NULL, &worker2, scheduler);

  pthread_join(worker_1, NULL);
  pthread_join(worker_2, NULL);

  scheduler_destroy(scheduler);
  bipartite_queue_destroy(queue);

  return EXIT_SUCCESS;
}

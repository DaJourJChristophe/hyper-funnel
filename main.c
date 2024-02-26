#include "command.h"
#include "lock.h"
#include "queue.h"

#include <math.h>

#include <pthread.h>

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_INACTIVE_SPINS 100

#define MAX_ALLOWED_WORKLOAD(cap) (uint64_t)(cap * (double)0.1)

struct scheduler
{
  command_queue_t *inbound;
  command_queue_t *outbound;
  combo_lock_t lock;
};

typedef struct scheduler scheduler_t;

void scheduler_schedule_enqueue(scheduler_t *self, const uint64_t id, command_t cmd)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  command_queue_t *queue = NULL;

  combo_lock(&self->lock, id);
  // combo_lock(&self->lock, id, &combo_lock_cycle, &combo_lock_stepper, self);

  if (false == command_queue_enqueue(self->inbound, cmd))
  {
    fprintf(stderr, "%s(): %s\n", __func__, "could not enqueue into command queue");
    exit(EXIT_FAILURE);
  }
}

void scheduler_schedule_dequeue(scheduler_t *self, const uint64_t id, command_t cmd)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  command_queue_t *queue = NULL;

  combo_lock(&self->lock, id);
  // combo_lock(&self->lock, id, &combo_lock_cycle, &combo_lock_stepper, self);

  if (false == command_queue_enqueue(self->outbound, cmd))
  {
    fprintf(stderr, "%s(): %s\n", __func__, "could not enqueue into command queue");
    exit(EXIT_FAILURE);
  }
}

// #define SCHEDULER_FAILURE_NODEFECT      0
// #define SCHEDULER_FAILURE_EARLY_RELEASE 1

// void scheduler_schedule_dequeue_limit(scheduler_t *self, int *failure, const uint64_t id, const uint64_t bound, command_t cmd)
// {
//   if (self == NULL)
//   {
//     fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
//     exit(EXIT_FAILURE);
//   }

//   command_queue_t *queue = NULL;

//   combo_lock(&self->lock, id);
//   // if (false == combo_lock_limit(&self->lock, id, &combo_lock_cycle, &combo_lock_stepper, bound, self))
//   // {
//   //   *failure = SCHEDULER_FAILURE_EARLY_RELEASE;
//   //   return;
//   // }

//   if (false == command_queue_enqueue(self->outbound, cmd))
//   {
//     fprintf(stderr, "%s(): %s\n", __func__, "could not enqueue into command queue");
//     exit(EXIT_FAILURE);
//   }

//   *failure = SCHEDULER_FAILURE_NODEFECT;
// }

command_t scheduler_execute_enqueue(scheduler_t *self, const uint64_t id)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  command_t cmd = NULL;

  combo_lock(&self->lock, id);
  // combo_lock(&self->lock, id, &combo_lock_cycle, &combo_lock_stepper, self);

  cmd = command_queue_dequeue(self->inbound);

  if (cmd == NULL)
  {
    // combo_try_unlock(&self->lock, id, &combo_lock_stepper, self);
    return NULL;
  }

  return cmd;
}

command_t scheduler_execute_dequeue(scheduler_t *self, const uint64_t id)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  command_t cmd = NULL;

  combo_lock(&self->lock, id);
  // combo_lock(&self->lock, id, &combo_lock_cycle, &combo_lock_stepper, self);

  cmd = command_queue_dequeue(self->outbound);

  if (cmd == NULL)
  {
    // combo_try_unlock(&self->lock, id, &combo_lock_stepper, self);
    return NULL;
  }

  return cmd;
}

// command_t scheduler_execute_dequeue_limit(scheduler_t *self, int *failure, const uint64_t id, const uint64_t bound)
// {
//   if (self == NULL)
//   {
//     fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
//     exit(EXIT_FAILURE);
//   }

//   command_t cmd = NULL;

//   combo_lock(&self->lock, id);

//   // if (false == combo_lock_limit(&self->lock, id, &combo_lock_cycle, &combo_lock_stepper, bound, self))
//   // {
//   //   *failure = SCHEDULER_FAILURE_EARLY_RELEASE;
//   //   return NULL;
//   // }

//   cmd = command_queue_dequeue(self->outbound);

//   if (cmd == NULL)
//   {
//     // combo_try_unlock(&self->lock, id, &combo_lock_stepper, self);
//     return NULL;
//   }

//   *failure = SCHEDULER_FAILURE_NODEFECT;
//   return cmd;
// }

struct observable;

struct observer;

struct exchange
{
  struct observer *observer;
  struct observable *observable;
  queue_t *queue;
  scheduler_t *sched;
};

typedef struct exchange exchange_t;

queue_t *observable_access(struct observable *self);

void exchange_enqueue(exchange_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  callback_t callback = NULL;
  command_t cmd = NULL;

  callback = queue_dequeue(observable_access(self->observable));

  /**
   * @warning This is the Critical Moment! This is the moment when
   *          a producer and consumer come together. This is the
   *          moment when this approach will experience the most
   *          idle blocking. In a non-blocking scenario, this is the
   *          moment when the most state failures will occur.
   */

  bool *result = NULL;

  scheduler_schedule_enqueue(self->sched, 1UL, command_enqueue);

  cmd = scheduler_execute_enqueue(self->sched, 1UL);

  if (cmd == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "did not receive a command back fro the scheduler");
    exit(EXIT_FAILURE);
  }

  result = (bool *)cmd(self->queue, callback);

  if (result == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "value error: null pointer exception");
    exit(EXIT_FAILURE);
  }

  if (false == result)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "could not enqueue into target queue");
    exit(EXIT_FAILURE);
  }

  free(result);
  result = NULL;
}

struct observer
{
  uint64_t id;
  queue_t *queue;
  scheduler_t sched;
  exchange_t exchange;
  atomic_int done;
};

typedef struct observer observer_t;

observer_t *observer_new(struct observable *observable, const size_t cap, const uint64_t id)
{
  observer_t *self = NULL;
  self = (observer_t *)calloc(1UL, sizeof(*self));
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "memory error");
    exit(EXIT_FAILURE);
  }

  self->queue = queue_new(cap);

  self->sched.inbound = command_queue_new(cap);
  self->sched.outbound = command_queue_new(cap);

  const uint64_t m = (1UL /*producers*/ + 1UL /*consumers*/);
  combo_lock_init(&self->sched.lock, MAX_INACTIVE_SPINS, MAX_ALLOWED_WORKLOAD(cap), m);

  atomic_init(&self->done, 0);

  self->exchange.observer = self;
  self->exchange.queue = self->queue;
  self->exchange.observable = observable;
  self->exchange.sched = &self->sched;

  self->id = id;
  return self;
}

void observer_destroy(observer_t *self)
{
  if (self != NULL)
  {
    if (self->queue != NULL)
    {
      queue_destroy(self->queue);
      self->queue = NULL;
    }

    if (self->sched.inbound != NULL)
    {
      command_queue_destroy(self->sched.inbound);
      self->sched.inbound = NULL;
    }

    if (self->sched.outbound != NULL)
    {
      command_queue_destroy(self->sched.outbound);
      self->sched.outbound = NULL;
    }

    free(self);
    self = NULL;
  }
}

uint64_t observer_get_id(observer_t *self);

void observer_notify(observer_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  exchange_enqueue(&self->exchange);
}

uint64_t observer_get_id(observer_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  return self->id;
}

struct load_balancer
{
  size_t n;
  uint64_t i;
};

typedef struct load_balancer load_balancer_t;

void load_balancer_next(load_balancer_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  self->i = (1UL + self->i) % self->n;
}

uint64_t load_balancer_get_current_index(load_balancer_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  return self->i;
}

struct publisher
{
  load_balancer_t lb;
};

typedef struct publisher publisher_t;

void publisher_next(publisher_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  load_balancer_next(&self->lb);
}

uint64_t publisher_get_current_index(publisher_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  return load_balancer_get_current_index(&self->lb);
}

struct observable
{
  queue_t *queue;
  observer_t **observers;
  size_t cap;
  uint64_t size;
  publisher_t pub;
};

typedef struct observable observable_t;

observer_t *observable_get_observer(observable_t *self, const int i)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  return self->observers[i];
}

void observable_next(observable_t *self);
uint64_t observable_get_current_index(observable_t *self);

void observable_publish(observable_t *self, callback_t callback)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  queue_enqueue(self->queue, callback);

  observer_t *observer = NULL;

  observer = observable_get_observer(self, observable_get_current_index(self));

  observer_notify(observer);

  observable_next(self);
}

queue_t *observable_access(observable_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  return self->queue;
}

void observable_next(observable_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  publisher_next(&self->pub);
}

uint64_t observable_get_current_index(observable_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  return publisher_get_current_index(&self->pub);
}

void *thread(void *args)
{
  observer_t *observer = NULL;
  observer = (observer_t *)args;

  callback_t callback = NULL;
  command_t cmd = NULL;

  int failure = 0;

  while (true)
  {
    if (1 == atomic_load(&observer->done))
    {
      break;
    }

    while (true)
    {
      // scheduler_schedule_dequeue_limit(&observer->sched, &failure, 0UL, 10UL, command_dequeue);
      scheduler_schedule_dequeue(&observer->sched, 0UL, command_dequeue);

      // if (failure == SCHEDULER_FAILURE_EARLY_RELEASE)
      // {
      //   break;
      // }

      // cmd = scheduler_execute_dequeue_limit(&observer->sched, &failure, 0UL, 10UL);
      cmd = scheduler_execute_dequeue(&observer->sched, 0UL);

      // if (failure == SCHEDULER_FAILURE_EARLY_RELEASE)
      // {
      //   break;
      // }

      if (cmd == NULL)
      {
        fprintf(stderr, "%s(): %s\n", __func__, "did not receive a command back from the scheduler");
        exit(EXIT_FAILURE);
      }

      callback = (callback_t)cmd(observer->queue, NULL);

      if (callback == NULL)
      {
        break;
      }

      callback(1);
    }
  }

  return NULL;
}

// volatile int sum = 0UL;
// atomic_int sum;

void stuff(const int i)
{
  double x = 1.0;
  x = exp(x);
  (void)x;
  // atomic_fetch_add(&sum, i);
  // sum += i;
}

#define MAX_WORKLOAD 1000000

#define MAX_WORKERS 2
#define MAX_THREADS 2

int main(void)
{
  observable_t observable;
  uint64_t i;

  observable.queue = queue_new(MAX_WORKLOAD);

  observable.cap = MAX_WORKERS;
  observable.observers = (observer_t **)calloc(observable.cap, sizeof(*observable.observers));

  for (i = 0; i < observable.cap; i++)
  {
    observable.observers[i] = observer_new(&observable, MAX_WORKLOAD, i);
  }

  observable.pub.lb.n = MAX_WORKERS;
  observable.pub.lb.i = 0UL;

  pthread_t tids[MAX_THREADS];
  memset(tids, 0, MAX_THREADS * sizeof(*tids));

  for (i = 0; i < MAX_THREADS; i++)
  {
    if (pthread_create(&tids[i], NULL, &thread, observable_get_observer(&observable, i)) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "could not create thread");
      exit(EXIT_FAILURE);
    }
  }

  clock_t start;
  clock_t end;

  start = clock();

  for (i = 0; i < MAX_WORKLOAD; i++)
  {
    observable_publish(&observable, &stuff);
  }

  observer_t *observer = NULL;

  for (i = 0; i < MAX_WORKERS; i++)
  {
    observer = observable_get_observer(&observable, i);
    atomic_store(&observer->done, 1);
  }

  for (i = 0; i < MAX_THREADS; i++)
  {
    if (pthread_join(tids[i], NULL) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "could not join thread");
      exit(EXIT_FAILURE);
    }
  }

  end = clock();

  const double duration = (double)(end - start) / (double)CLOCKS_PER_SEC;

  printf("%.15f\n", duration);

  return 0;
}



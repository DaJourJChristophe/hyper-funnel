#ifndef TURNPIKE__COMMON_H
#define TURNPIKE__COMMON_H

#include <stddef.h>
#include <stdlib.h>

static void _free(void **__ptr)
{
  if (NULL != __ptr && NULL != *__ptr)
  {
    free(*__ptr);
    *__ptr = NULL;
  }
}

#define __free(__ptr) _free((void **)&__ptr);

#endif/*TURNPIKE__COMMON_H*/

#ifndef TURNPIKE__THREAD_SAFE_QUEUE_H
#define TURNPIKE__THREAD_SAFE_QUEUE_H

#include <inttypes.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief A safe implementation of a Queue data structure. This data
 *        structure is based upon the Circular Buffer. It has a stateful
 *        capacity specification and read and write pointers. Every
 *        operation is a constant time operation.
 */
struct ts_queue
{
  int *items;
  size_t cap;
  atomic_ulong r;
  atomic_ulong w;
};

/**
 * @brief An alias for the Queue data struct.
 */
typedef struct ts_queue ts_queue_t;

/**
 * @brief Allocate a new Queue data structure to the heap.
 * @param cap The maximum capacity allow in the Queue data structure.
 */
ts_queue_t *ts_queue_new(const size_t cap);

/**
 * @brief Deallocate an existing Queue data structure from the heap.
 * @param self A double pointer to the Queue container.
 */
void __ts_queue_destroy(ts_queue_t **self);

/**
 * @brief Create a stack-pointer and pass it to queue_destroy() so that
 *        the queue pointer in the caller knows the queue no longer exists.
 * @param self A pointer to the Queue container.
 */
#define ts_queue_destroy(self) __ts_queue_destroy(&self)

/**
 * @brief Add an item to the Queue data structure. By default, the Queue
 *        adds items to the beginning of the data structure and removes
 *        items from the end of the data structure.
 *
 *        @note In the future, an argument will be added to this method
 *              so that end-users can change the queue scheme.
 *
 * @param self A pointer to the Queue container.
 * @param item The item to be added to the Queue data structure.
 * @return Whether or not the item was successfully added to the Queue.
 */
bool ts_queue_enqueue(ts_queue_t *self, const int item);

/**
 * @brief Remove an item from the Queue data structure.
 * @param self A pointer to the Queue container.
 * @return The item currently removed from the front of the Queue.
 */
int *ts_queue_dequeue(ts_queue_t *self);

/**
 * @brief Return the item at the front of the Queue data structure.
 * @param self A pointer to the Queue container.
 * @return A copy of the item currently at the front of the Queue.
 */
int *ts_queue_peek(ts_queue_t *self);

/**
 * @brief Return the number of items currently in the Queue data structure.
 * @param self A pointer to the Queue container.
 * @return The current size of the Queue data structure.
 */
size_t ts_queue_size(ts_queue_t *self);

/**
 * @brief Determine of the Queue data structure is empty.
 * @param self A pointer to the Queue container.
 * @return Whether or not the Queue data structure is empty.
 */
bool ts_queue_empty(ts_queue_t *self);

#endif/*TURNPIKE__THREAD_SAFE_QUEUE_H*/

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/**
 * @brief Allocate the Queue container and the queue buffer to the heap.
 * @note Do not implement your calls to the heap here. Instead, please use
 *       the wrapper functions specified in common.h. These functions
 *       deal with common problems like exception handling and fragmentation.
 */
static ts_queue_t *ts_queue_alloc(const size_t cap)
{
  ts_queue_t *self = NULL;
  // TODO: Create a wrapper for this allocation call.
  self = (ts_queue_t *)calloc(1, sizeof(*self));
  // TODO: Create a wrapper for this allocation call.
  self->items = (int *)calloc(cap, sizeof(*self->items));
  return self;
}

/**
 * @brief Allocate a new Queue data structure to the heap. Do not allocate
 *        the queue properties here. Queue properties are allocated in
 *        queue_alloc() refer to it for more information. With that, this
 *        function sets up the Queue for usage.
 * @param cap The maximum capacity allow in the Queue data structure.
 */
ts_queue_t *ts_queue_new(const size_t cap)
{
  ts_queue_t *self = NULL;
  self = ts_queue_alloc(cap);

  atomic_init(&self->r, 0UL);
  atomic_init(&self->w, 0UL);

  self->cap = cap;
  return self;
}

/**
 * @brief Deallocate an existing Queue data structure from the heap.
 * @param self A double pointer to the Queue container.
 */
void __ts_queue_destroy(ts_queue_t **self)
{
  if (self != NULL && *self != NULL)
  {
    __free((*self)->items);
    free(*self);
    *self = NULL;
  }
}

/**
 * @brief Add an item to the Queue data structure. By default, the Queue
 *        adds items to the beginning of the data structure and removes
 *        items from the end of the data structure.
 *
 *        @note In the future, an argument will be added to this method
 *              so that end-users can change the queue scheme.
 *
 * @param self A pointer to the Queue container.
 * @param item The item to be added to the Queue data structure.
 * @return Whether or not the item was successfully added to the Queue.
 */
bool ts_queue_enqueue(ts_queue_t *self, const int item)
{
  if (self == NULL)
  {
    return false;
  }

  const uint64_t r = atomic_load(&self->r);
  const uint64_t w = atomic_fetch_add(&self->w, 1UL);

  if ((w - r) >= self->cap)
  {
    atomic_exchange(&self->w, w);
    return false;
  }

  self->items[w % self->cap] = item;

  return true;
}

/**
 * @brief Remove an item from the Queue data structure.
 * @param self A pointer to the Queue container.
 * @return The item currently removed from the front of the Queue.
 */
int *ts_queue_dequeue(ts_queue_t *self)
{
  if (self == NULL)
  {
    return NULL;
  }

  const uint64_t w = atomic_load(&self->w);
  const uint64_t r = atomic_fetch_add(&self->r, 1UL);

  if (r == w)
  {
    atomic_exchange(&self->r, r);
    return NULL;
  }

  int *item = NULL;
  item = (int *)calloc(1, sizeof(*item));

  *item = self->items[r % self->cap];

  return item;
}

/**
 * @brief Return the item at the front of the Queue data structure.
 * @param self A pointer to the Queue container.
 * @return A copy of the item currently at the front of the Queue.
 */
int *ts_queue_peek(ts_queue_t *self)
{
  if (self == NULL)
  {
    return NULL;
  }

  const uint64_t w = atomic_load(&self->w);
  const uint64_t r = atomic_load(&self->r);

  if (r == w)
  {
    return NULL;
  }

  int *item = NULL;
  item = (int *)calloc(1, sizeof(*item));

  *item = self->items[r % self->cap];

  return item;
}

/**
 * @brief Return the number of items currently in the Queue data structure.
 * @param self A pointer to the Queue container.
 * @return The current size of the Queue data structure.
 */
size_t ts_queue_size(ts_queue_t *self)
{
  if (self == NULL)
  {
    return 0UL;
  }

  const uint64_t w = atomic_load(&self->w);
  const uint64_t r = atomic_load(&self->r);

  return (w - r);
}

/**
 * @brief Determine of the Queue data structure is empty.
 * @param self A pointer to the Queue container.
 * @return Whether or not the Queue data structure is empty.
 */
bool ts_queue_empty(ts_queue_t *self)
{
  if (self == NULL)
  {
    return false;
  }

  const uint64_t w = atomic_load(&self->w);
  const uint64_t r = atomic_load(&self->r);

  // Do not call the forward facing queue_size() method here.
  // That method adds redundant overhead to this method call.
  return 0UL == (w - r);
}

#include <stdatomic.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct observer;

typedef void *(*observer_callback_t)(void *);

struct observable;

struct observer
{
  struct observable *observable;
  observer_callback_t notify;
};

typedef struct observer observer_t;

observer_t *observer_new(struct observable *observable, observer_callback_t notify)
{
  observer_t *self = NULL;
  self = (observer_t *)calloc(1, sizeof(*self));
  self->observable = observable;
  self->notify = notify;
  return self;
}

void observer_destroy(observer_t *self)
{
  if (self != NULL)
  {
    free(self);
    self = NULL;
  }
}

struct observable
{
  ts_queue_t *queue;
  size_t cap;
  observer_t **observers;
  size_t max_observers;
  uint64_t count;
  pthread_t *tids;
  size_t max_threads;
  atomic_bool done;
  atomic_bool ready;
};

typedef struct observable observable_t;

observable_t *observable_new(const size_t cap, const size_t max_observers, const size_t max_threads)
{
  observable_t *self = NULL;
  self = (observable_t *)calloc(1, sizeof(*self));
  self->queue = ts_queue_new(cap);
  self->tids = (pthread_t *)calloc(max_threads, sizeof(*self->tids));
  self->observers = (observer_t **)calloc(max_observers, sizeof(*self->observers));

  atomic_init(&self->done, false);
  atomic_init(&self->ready, false);

  self->max_observers = max_observers;
  self->max_threads;
  self->cap;
  return self;
}

void observable_destroy(observable_t *self)
{
  if (self != NULL)
  {
    if (self->queue != NULL)
    {
      ts_queue_destroy(self->queue);
    }

    if (self->tids != NULL)
    {
      free(self->tids);
      self->tids = NULL;
    }

    if (self->observers != NULL)
    {
      observer_t *observer = NULL;
      uint64_t i;

      for (i = 0; i < self->count; i++)
      {
        observer = self->observers[i];
        observer_destroy(observer);
      }

      free(self->observers);
      self->observers = NULL;
    }

    free(self);
    self = NULL;
  }
}

void observable_init(observable_t *self)
{
  if (self == NULL)
  {
    return;
  }

  observer_t *observer = NULL;
  uint64_t i;

  for (i = 0; i < self->count; i++)
  {
    observer = self->observers[i];

    if (pthread_create(&self->tids[i], NULL, observer->notify, observer) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "could not create thread");
      exit(EXIT_FAILURE);
    }
  }
}

void observable_select(observable_t *self)
{
  if (self == NULL)
  {
    return;
  }
loop:
  if (true == atomic_load(&self->ready) ||
      true == atomic_load(&self->done))
  {
    goto done;
  }
  goto loop;
done:
  return;
}

void observable_clear(observable_t *self)
{
  if (self == NULL)
  {
    return;
  }

  atomic_store(&self->ready, false);
}

void observable_wait(observable_t *self)
{
  if (self == NULL)
  {
    return;
  }

  uint64_t i;

  for (i = 0; i < self->count; i++)
  {
    if (pthread_join(self->tids[i], NULL) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "could not join thread");
      exit(EXIT_FAILURE);
    }
  }
}

void observable_shutdown(observable_t *self)
{
  if (self == NULL)
  {
    return;
  }

  uint64_t i;

  atomic_store(&self->done, true);

  for (i = 0; i < self->count; i++)
  {
    if (pthread_join(self->tids[i], NULL) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "could not join thread");
      exit(EXIT_FAILURE);
    }
  }
}

bool observable_publish(observable_t *self, const int item)
{
  if (self == NULL)
  {
    return false;
  }

  if (false == ts_queue_enqueue(self->queue, item))
  {
    fprintf(stderr, "%s(): %s\n", __func__, "could not enqueue item");
    return false;
  }

  atomic_store(&self->ready, true);

  return true;
}

bool observable_subscribe(observable_t *self, observer_t *observer)
{
  if (self == NULL || observer == NULL)
  {
    return false;
  }

  if (self->count >= self->max_observers)
  {
    return false;
  }

  self->observers[self->count++] = observer;
  return true;
}

void *notifier1(void *args)
{
  observer_t *observer = (observer_t *)args;
  int *item = NULL;

  while (false == atomic_load(&observer->observable->done) ||
         false == ts_queue_empty(observer->observable->queue))
  {
    observable_select(observer->observable);

    while (NULL != (item = ts_queue_dequeue(observer->observable->queue)))
    {
      printf("%d\n", *item);
    }

    observable_clear(observer->observable);
  }

  return NULL;
}

void *notifier2(void *args)
{
  observer_t *observer = (observer_t *)args;
  int *item = NULL;

  while (false == atomic_load(&observer->observable->done) ||
         false == ts_queue_empty(observer->observable->queue))
  {
    observable_select(observer->observable);

    while (NULL != (item = ts_queue_dequeue(observer->observable->queue)))
    {
      printf("%d\n", *item);
    }

    observable_clear(observer->observable);
  }

  return NULL;
}

#define QUEUE_CAPACITY 10
#define MAX_OBSERVERS   5
#define MAX_THREADS     2

int main(void)
{
  observer_t *observer1 = NULL;
  observer_t *observer2 = NULL;

  observable_t *observable = NULL;
  observable = observable_new(QUEUE_CAPACITY, MAX_OBSERVERS, MAX_THREADS);

  observer1 = observer_new(observable, &notifier1);
  observer2 = observer_new(observable, &notifier2);

  observable_subscribe(observable, observer1);
  observable_subscribe(observable, observer2);

  observable_init(observable);

  int i;

  for (i = 1; i < 6; i++)
  {
    observable_publish(observable, i);
  }

  observable_shutdown(observable);
  observable_destroy(observable);

  return EXIT_FAILURE;
}

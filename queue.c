#include "command.h"
#include "queue.h"

#include <immintrin.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

command_queue_t *command_queue_new(const size_t cap)
{
  command_queue_t *self = NULL;
  self = (command_queue_t *)calloc(1, sizeof(*self));
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "memory error");
    exit(EXIT_FAILURE);
  }

  self->items = (command_t *)calloc(cap, sizeof(*self->items));
  if (self->items == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "memory error");
    exit(EXIT_FAILURE);
  }

  if (pthread_mutex_init(&self->lock, NULL) < 0)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "could not init lock");
    exit(EXIT_FAILURE);
  }

  self->cap = cap;
  return self;
}

void command_queue_destroy(command_queue_t *self)
{
  if (self != NULL)
  {
    if (self->items != NULL)
    {
      free(self->items);
      self->items = NULL;
    }

    free(self);
    self = NULL;
  }
}

bool command_queue_enqueue(command_queue_t *self, command_t item)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  if (pthread_mutex_lock(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "acquire lock error");
    exit(EXIT_FAILURE);
  }

  if ((self->w - self->r) >= self->cap)
  {
    if (pthread_mutex_unlock(&self->lock) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "release lock error");
      exit(EXIT_FAILURE);
    }
    return false;
  }

  self->items[self->w++ % self->cap] = item;

  if (pthread_mutex_unlock(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "release lock error");
    exit(EXIT_FAILURE);
  }

  return true;
}

command_t command_queue_dequeue(command_queue_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  if (pthread_mutex_lock(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "acquire lock error");
    exit(EXIT_FAILURE);
  }

  if (self->r == self->w)
  {
    if (pthread_mutex_unlock(&self->lock) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "release lock error");
      exit(EXIT_FAILURE);
    }
    return NULL;
  }

  command_t command = NULL;
  command = self->items[self->r++ % self->cap];

  if (pthread_mutex_unlock(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "release lock error");
    exit(EXIT_FAILURE);
  }

  return command;
}

size_t command_queue_size(command_queue_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  if (pthread_mutex_lock(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "acquire lock error");
    exit(EXIT_FAILURE);
  }

  const size_t size = self->w - self->r;

  if (pthread_mutex_unlock(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "release lock error");
    exit(EXIT_FAILURE);
  }

  return size;
}

queue_t *queue_new(const size_t cap)
{
  queue_t *self = NULL;
  self = (queue_t *)calloc(1, sizeof(*self));
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "memory error");
    exit(EXIT_FAILURE);
  }

  self->items = (callback_t *)calloc(cap, sizeof(*self->items));
  if (self->items == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "memory error");
    exit(EXIT_FAILURE);
  }

  if (pthread_mutex_init(&self->lock, NULL) < 0)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "could not init lock");
    exit(EXIT_FAILURE);
  }

  self->cap = cap;
  return self;
}

void queue_destroy(queue_t *self)
{
  if (self != NULL)
  {
    if (self->items != NULL)
    {
      free(self->items);
      self->items = NULL;
    }

    free(self);
    self = NULL;
  }
}

bool queue_enqueue(queue_t *self, callback_t item)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  if (pthread_mutex_lock(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "acquire lock error");
    exit(EXIT_FAILURE);
  }

  if ((self->w - self->r) >= self->cap)
  {
    if (pthread_mutex_unlock(&self->lock) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "release lock error");
      exit(EXIT_FAILURE);
    }
    return false;
  }

  self->items[self->w++ % self->cap] = item;

  if (pthread_mutex_unlock(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "release lock error");
    exit(EXIT_FAILURE);
  }

  return true;
}

callback_t queue_dequeue(queue_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  if (pthread_mutex_lock(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "acquire lock error");
    exit(EXIT_FAILURE);
  }

  if (self->r == self->w)
  {
    if (pthread_mutex_unlock(&self->lock) < 0)
    {
      fprintf(stderr, "%s(): %s\n", __func__, "release lock error");
      exit(EXIT_FAILURE);
    }
    return NULL;
  }

  callback_t callback = NULL;
  callback = self->items[self->r++ % self->cap];

  if (pthread_mutex_unlock(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "release lock error");
    exit(EXIT_FAILURE);
  }

  return callback;
}

size_t queue_size(queue_t *self)
{
  if (self == NULL)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "null pointer exception");
    exit(EXIT_FAILURE);
  }

  if (pthread_mutex_lock(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "acquire lock error");
    exit(EXIT_FAILURE);
  }

  const size_t size = self->w - self->r;

  if (pthread_mutex_unlock(&self->lock) < 0)
  {
    fprintf(stderr, "%s(): %s\n", __func__, "release lock error");
    exit(EXIT_FAILURE);
  }

  return size;
}

#ifndef QUEUE_H
#define QUEUE_H

#include "command.h"

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct command_queue
{
  command_t *items;
  size_t cap;
  uint64_t w;
  uint64_t r;
  pthread_mutex_t lock;
};

typedef struct command_queue command_queue_t;

command_queue_t *command_queue_new(const size_t cap);

void command_queue_destroy(command_queue_t *self);

bool command_queue_enqueue(command_queue_t *self, command_t item);

command_t command_queue_dequeue(command_queue_t *self);

size_t command_queue_size(command_queue_t *self);

struct queue
{
  callback_t *items;
  size_t cap;
  uint64_t w;
  uint64_t r;
  pthread_mutex_t lock;
};

typedef struct queue queue_t;

queue_t *queue_new(const size_t cap);

void queue_destroy(queue_t *self);

bool queue_enqueue(queue_t *self, callback_t item);

callback_t queue_dequeue(queue_t *self);

size_t queue_size(queue_t *self);

#endif/*QUEUE_H*/

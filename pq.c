#include "pq.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void heapify_down(priority_queue_t *self);

static void heapify_up(priority_queue_t *self);

priority_queue_t *priority_queue_new(void)
{
  priority_queue_t *self = NULL;
  self = (priority_queue_t *)calloc(1, sizeof(*self));
  return self;
}

void priority_queue_destroy(priority_queue_t *self)
{
  if (self != NULL)
  {
    free(self);
    self = NULL;
  }
}

static uint64_t get_left_child_index(const uint64_t parent_index)
{
  return 2 * parent_index + 1;
}

static uint64_t get_right_child_index(const uint64_t parent_index)
{
  return 2 * parent_index + 2;
}

static uint64_t get_parent_index(const uint64_t child_index)
{
  return (child_index - 1) / 2;
}

static uint64_t get_left_child(priority_queue_t *self, const uint64_t parent_index)
{
  return self->values[get_left_child_index(parent_index)].key;
}

static uint64_t get_right_child(priority_queue_t *self, const uint64_t parent_index)
{
  return self->values[get_right_child_index(parent_index)].key;
}

static uint64_t get_parent(priority_queue_t *self, const uint64_t child_index)
{
  return self->values[get_parent_index(child_index)].key;
}

static bool has_left_child(priority_queue_t *self, const uint64_t parent_index)
{
  return get_left_child_index(parent_index) < self->size;
}

static bool has_right_child(priority_queue_t *self, const uint64_t parent_index)
{
  return get_right_child_index(parent_index) < self->size;
}

static bool has_parent(priority_queue_t *self, const uint64_t child_index)
{
  return get_parent_index(child_index) >= 0;
}

static void swap(priority_queue_t *self, const uint64_t i, const uint64_t j)
{
  priority_queue_node_t temp = self->values[i];
  self->values[i] = self->values[j];
  self->values[j] = temp;
}

void *priority_queue_peek(priority_queue_t *self)
{
  if (self == NULL)
  {
    perror("priority queue pointer is null");
    exit(EXIT_FAILURE);
  }

  if (self->size == 0)
  {
    return NULL;
  }

  void *data = NULL;
  data = calloc(PQ_NODE_SIZE, sizeof(uint8_t));
  memcpy(data, self->values[0].data, PQ_NODE_SIZE * sizeof(uint8_t));
  return data;
}

void *priority_queue_poll(priority_queue_t *self)
{
  if (self == NULL)
  {
    perror("priority queue pointer is null");
    exit(EXIT_FAILURE);
  }

  if (self->size == 0)
  {
    return NULL;
  }

  void *data = NULL;
  data = calloc(PQ_NODE_SIZE, sizeof(uint8_t));
  memcpy(data, self->values[0].data, PQ_NODE_SIZE * sizeof(uint8_t));

  memcpy(&self->values[0], &self->values[self->size - 1], PQ_NODE_SIZE * sizeof(uint8_t));
  self->size--;

  heapify_down(self);

  return data;
}

void priority_queue_add(priority_queue_t *self, const uint64_t key, const void *data, const size_t size)
{
  if (self == NULL)
  {
    perror("priority queue pointer is null");
    exit(EXIT_FAILURE);
  }

  if (self->size >= 10)
  {
    return;
  }

  self->values[self->size].key = key;
  memcpy(self->values[self->size].data, data, size * sizeof(uint8_t));
  self->size++;
  heapify_up(self);
}

static void heapify_down(priority_queue_t *self)
{
  if (self == NULL)
  {
    perror("priority queue pointer is null");
    exit(EXIT_FAILURE);
  }

  uint64_t index = 0;

  while (has_left_child(self, index))
  {
    uint64_t smaller_child_index = get_left_child_index(index);

    if (has_right_child(self, index) && get_right_child(self, index) < get_left_child(self, index))
    {
      smaller_child_index = get_right_child_index(index);
    }

    if (self->values[index].key < self->values[smaller_child_index].key)
    {
      break;
    }
    else
    {
      swap(self, index, smaller_child_index);
    }

    index = smaller_child_index;
  }
}

static void heapify_up(priority_queue_t *self)
{
  if (self == NULL)
  {
    perror("priority queue pointer is null");
    exit(EXIT_FAILURE);
  }

  uint64_t index = self->size - 1;

  while (has_parent(self, index) && get_parent(self, index) > self->values[index].key)
  {
    swap(self, get_parent_index(index), index);
    index = get_parent_index(index);
  }
}

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <inttypes.h>
#include <stddef.h>

#define PQ_NODE_SIZE 512

struct priority_queue_node
{
  uint64_t key;
  uint8_t data[PQ_NODE_SIZE];
};

typedef struct priority_queue_node priority_queue_node_t;

#define PQ_MAX_NODES 10

struct priority_queue
{
  priority_queue_node_t values[PQ_MAX_NODES];
  size_t size;
};

typedef struct priority_queue priority_queue_t;

extern priority_queue_t *priority_queue_new(void);

extern void priority_queue_destroy(priority_queue_t *self);

extern void *priority_queue_peek(priority_queue_t *self);

extern void *priority_queue_poll(priority_queue_t *self);

extern void priority_queue_add(priority_queue_t *self, const uint64_t key, const void *data, const size_t size);

#endif/*PRIORITY_QUEUE_H*/

#ifndef COMMAND_H
#define COMMAND_H

struct queue;

typedef void (*callback_t)(const int i);

typedef void *(*command_t)(struct queue *self, callback_t);

void *command_enqueue(struct queue *self, callback_t callback);

void *command_dequeue(struct queue *self, callback_t callback);

#endif/*COMMAND_H*/

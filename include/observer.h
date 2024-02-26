#ifndef HYPER_FUNNEL__OBSERVER_H
#define HYPER_FUNNEL__OBSERVER_H

#include "channel.h"

#include <stdatomic.h>

struct observer;

typedef void *(*observer_callback_t)(void *);

struct observable;

struct observer
{
  uint64_t channel_id;
  struct observable *observable;
  bidirectional_channel_t *channel;
  observer_callback_t notify;
  atomic_bool ready;
};

typedef struct observer observer_t;

observer_t *observer_new(struct observable *observable, bidirectional_channel_t *channel, observer_callback_t notify, const uint64_t channel_id);

void observer_destroy(observer_t *self);

void observer_release(observer_t *self);

void observer_clear(observer_t *self);

#endif/*HYPER_FUNNEL__OBSERVER_H*/

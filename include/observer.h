#ifndef HYPER_FUNNEL__OBSERVER_H
#define HYPER_FUNNEL__OBSERVER_H

#include "channel.h"

struct observer;

typedef void *(*observer_callback_t)(void *);

struct observable;

struct observer
{
  struct observable *observable;
  bidirectional_channel_t *channel;
  observer_callback_t notify;
};

typedef struct observer observer_t;

observer_t *observer_new(struct observable *observable, bidirectional_channel_t *channel, observer_callback_t notify);

void observer_destroy(observer_t *self);

#endif/*HYPER_FUNNEL__OBSERVER_H*/

#ifndef HYPER_FUNNEL__CHANNEL_H
#define HYPER_FUNNEL__CHANNEL_H

#include <turnpike/tsqueue.h>

struct bidirectional_channel
{
  ts_queue_t *downstream;
  ts_queue_t *upstream;
};

typedef struct bidirectional_channel bidirectional_channel_t;

bidirectional_channel_t *bidirectional_channel_new(const size_t downstream_capacity,
                                                   const size_t upstream_capacity);

void bidirectional_channel_destroy(bidirectional_channel_t *self);

#endif/*HYPER_FUNNEL__CHANNEL_H*/

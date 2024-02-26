#include "channel.h"

#include <turnpike/bipartite.h>

#include <stddef.h>
#include <stdlib.h>

bidirectional_channel_t *bidirectional_channel_new(const size_t downstream_capacity,
                                                   const size_t upstream_capacity)
{
  bidirectional_channel_t *self = NULL;
  self = (bidirectional_channel_t *)calloc(1, sizeof(*self));

  self->downstream = bipartite_queue_new(downstream_capacity, sizeof(int));
  self->upstream = bipartite_queue_new(upstream_capacity, sizeof(int));

  return self;
}

void bidirectional_channel_destroy(bidirectional_channel_t *self)
{
  if (self == NULL)
  {
    bipartite_queue_destroy(self->downstream);
    bipartite_queue_destroy(self->upstream);

    free(self);
    self = NULL;
  }
}

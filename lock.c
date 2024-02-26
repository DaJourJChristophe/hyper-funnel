#include "sequence.h"
#include "lock.h"

#include <immintrin.h>
#include <stdatomic.h>
#include <stdbool.h>

#ifndef always_inline
#define always_inline __attribute__ ((always_inline))
#endif/*always_inline*/

static inline bool always_inline mc_trylock(mc_lock_t *self, const int64_t id)
{
  return id == im_seq_tick(&self->seq, self->j, self->k);
}

void mc_lock_init(mc_lock_t *self)
{
  im_seq_init(&self->seq);
  self->j = 800000UL;
  self->k = (1UL < 2);
}

void mc_lock(mc_lock_t *self, const int64_t id)
{
  while (mc_trylock(self, id))
  {
    _mm_pause();
  }
}

#define WORK 1000000

#include <pthread.h>
#include <stdio.h>

int i = 0;

void *thread_a(void *args)
{
  mc_lock_t *lock = NULL;
  lock = (mc_lock_t *)args;

  for (int j = 0; j < WORK; j++)
  {
    mc_lock(lock, 0L);
    i++;
  }

  return NULL;
}

void *thread_b(void *args)
{
  mc_lock_t *lock = NULL;
  lock = (mc_lock_t *)args;

  for (int j = 0; j < WORK; j++)
  {
    mc_lock(lock, 1L);
    i++;
  }

  return NULL;
}

/*

0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15

*/

int main(void)
{
  mc_lock_t lock;

  pthread_t a;
  pthread_t b;

  mc_lock_init(&lock);

  pthread_create(&a, NULL, &thread_a, &lock);
  pthread_create(&b, NULL, &thread_b, &lock);

  pthread_join(a, NULL);
  pthread_join(b, NULL);

  printf("%d\n", i);

  return 0;
}

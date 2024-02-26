/**
 * @note The kernel handles multiple parallel system calls in parallel.
 */

#include "clock.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/**
 * Range: 0 - 1000000
 * Interval Units: Microseconds
 */
static int64_t sm_clock(sm_clock_t *self)
{
  struct timeval tv;

  tv.tv_sec = 0L; // long int tv_sec
  tv.tv_usec = 0L; // long int tv_usec

  if (gettimeofday(&tv, NULL) < 0)
  {
    fprintf(stderr, "%s(): %s\n", __func__, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return tv.tv_usec;
}

void sm_clock_init(sm_clock_t *self)
{
  self->a = sm_clock(NULL);
}

/**
 * @brief Compute the current clock value by triggering the timeval
 *        interrupt and recording the nanosecond property. Compute the
 *        absolute different between the starting nanosecond value a
 *        and the current nanosecond value k, which is the distance
 *        between the creation of the clock and the current evolution.
 */
int64_t sm_clock_tick(sm_clock_t *self)
{
  return labs(sm_clock(NULL) - self->a);
}

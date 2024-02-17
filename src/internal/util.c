#include <inttypes.h>
#include <stddef.h>

uint64_t lru(const uint64_t *arr, const size_t size)
{
  uint64_t x;
  uint64_t i;
  uint64_t j;

  x = arr[0];
  j = 0;

  for (i = 1; i < size; i++)
  {
    if (arr[i] < x)
    {
      x = arr[i];
      j = i;
    }
  }

  return j;
}

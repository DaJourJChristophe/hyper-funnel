#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

int main(void)
{
  clock_t start;
  clock_t end;

  start = clock();

  uint64_t i;

  for (i = 0; i < 1000000UL; i++)
  {
    double x = 1.0;
    x = exp(x);
    (void)x;
  }

  end = clock();

  const double duration = (double)(end - start) / (double)CLOCKS_PER_SEC;

  printf("%.15f\n", duration);

  return 0;
}

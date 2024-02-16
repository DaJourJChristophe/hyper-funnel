#ifndef HYPER_FUNNEL__COMMON_H
#define HYPER_FUNNEL__COMMON_H

#include <jemalloc/jemalloc.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static void __die(const char *funcname, const char *msg)
{
  const char fmt[] = "%s(): %s\n";
  fprintf(stderr, fmt, funcname, msg);
  exit(EXIT_FAILURE);
}

#define die(__msg) __die(__func__, __msg)

static void *_calloc(const size_t nmemb, const size_t size)
{
  void *__ptr = NULL;
  __ptr = mallocx((nmemb * size), MALLOCX_ZERO);
  if (__ptr == NULL)
  {
    die("a memory error occurred");
  }
  return __ptr;
}

static void _free(void **__ptr)
{
  if (NULL != __ptr && NULL != *__ptr)
  {
    dallocx(*__ptr, 0);
    *__ptr = NULL;
  }
}

#define __free(__ptr) _free((void **)&__ptr);

#endif/*HYPER_FUNNEL__COMMON_H*/

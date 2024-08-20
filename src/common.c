#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

void die(const char *msg) {
  int err = errno;
  fprintf(stderr, "[%d] Error in %s\n", err, msg);
  abort();
}

void msg(const char *message) { fprintf(stderr, "%s\n", message); }

void debug_msg(const char *const msg, ...) {
  printf("DEBUG: ");
  va_list argptr;
  va_start(argptr, msg);
  vfprintf(stdout, msg, argptr);
  va_end(argptr);
  printf("\n");
}

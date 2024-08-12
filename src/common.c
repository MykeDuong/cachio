#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

void die(const char *msg) {
  int err = errno;
  fprintf(stderr, "[%d] Error in %s\n", err, msg);
  abort();
}

void msg(const char *message) { fprintf(stderr, "%s\n", message); }

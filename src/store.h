#ifndef STORE_H
#define STORE_H

#include "command.h"
#include "encoding.h"
#include "map.h"

#include <stdint.h>

static struct {
  Map db;
} g_data;

void execute_keys(Command *command, Output *out);

void execute_get(Command *command, Output *out);

void execute_set(Command *command, Output *out);

void execute_delete(Command *command, Output *out);

#endif /* STORE_H */

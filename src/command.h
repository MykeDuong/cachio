#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  char **strings;
  int count;
  int capacity;
} Command;

void initialize_command(Command *command);

void add_to_command(Command *command, char *string, uint32_t length);

void free_command(Command *command);

bool is_command_type(Command *command, const char *type);

#endif /* COMMAND_H */

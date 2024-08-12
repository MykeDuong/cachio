#include <string.h>

#include "command.h"
#include "stdlib.h"

void initialize_command(Command *command) {
  command->count = 0;
  command->capacity = 0;
  command->strings = NULL;
}

void add_to_command(Command *command, char *string, uint32_t length) {
  if (command->capacity < command->count + 1) {
    // Grow
    if (command->capacity < 8) {
      command->capacity = 8;
      command->strings = realloc(command->strings, 8 * sizeof(char *));
    } else {
      command->capacity = command->capacity * 2;
      command->strings =
          realloc(command->strings, command->capacity * sizeof(char *));
    }
  }

  char *s = malloc((length + 1) * sizeof(char));
  memcpy(s, string, length);
  s[length] = '\0';

  command->strings[command->count] = s;
  command->count++;
}

void free_command(Command *command) {
  for (int i = 0; i < command->count; i++) {
    free(command->strings[i]);
  }
  free(command->strings);
  initialize_command(command);
}

bool is_command_type(Command *command, const char *type) {
  return memcmp(command->strings[0], type, strlen(type)) == 0;
}

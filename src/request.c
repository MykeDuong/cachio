#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command.h"
#include "common.h"
#include "request.h"
#include "store.h"

int32_t parse_request(const uint8_t *data, size_t length, Command *command) {
  if (length < 4) {
    return -1;
  }

  // Number of strings
  uint32_t n = 0;
  memcpy(&n, &data[0], 4);
  if (n > K_MAX_ARGS) {
    return -1;
  }

  size_t position = 4; // Point to currently parsed position

  // Loop through strings/messages
  while (n--) {
    if (position + 4 > length) {
      return -1;
    }

    // Size of string
    uint32_t size = 0;
    memcpy(&size, &data[position], 4);
    if (position + 4 + size > length) {
      return -1;
    }
    add_to_command(command, (char *)&data[position + 4], size);
    position += 4 + size;
  }

  if (position != length) {
    return -1; // Trailing garbage
  }
  return 0;
}

void execute_request(Command *command, Output *out) {
  if (command->count == 1 && is_command_type(command, "keys")) {
    execute_keys(command, out);
  } else if (command->count == 2 && is_command_type(command, "get")) {
    execute_get(command, out);
  } else if (command->count == 3 && is_command_type(command, "set")) {
    execute_set(command, out);
  } else if (command->count == 2 && is_command_type(command, "delete")) {
    execute_delete(command, out);
  } else {
    // Command not recognized
    out_error(out, ERROR_UNKNOWN, "Unknown Command");
  }
}

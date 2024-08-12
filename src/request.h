#ifndef REQUEST_H
#define REQUEST_H

#include "command.h"
#include "encoding.h"

#include <stdint.h>
#include <stdlib.h>

#define K_MAX_ARGS 1024

typedef enum {
  ERROR_TOO_BIG,
  ERROR_UNKNOWN,
} ErrorType;

int32_t parse_request(const uint8_t *data, size_t length, Command *command);

void execute_request(Command *command, Output *out);

#endif /* REQUEST_H */

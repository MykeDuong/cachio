#ifndef ENCODING_H
#define ENCODING_H

#include <stdint.h>
#include <stdlib.h>

typedef struct {
  char *chars;
  size_t size;
  size_t capacity;
} Output;

void initialize_output(Output *out);

void free_output(Output *out);

void out_nil(Output *out);

void out_string(Output *out, char *value, uint32_t length);

void out_integer(Output *out, int64_t value);

void out_error(Output *out, int32_t code, const char *const message);

void out_array(Output *out, uint32_t n);

#endif /* ENCODING_H */

#include "encoding.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>

void initialize_output(Output *out) {
  out->size = 0;
  out->capacity = 0;
  out->chars = NULL;
}

void free_output(Output *out) {
  free(out->chars);
  initialize_output(out);
}

static void resize_output(Output *out) {
  if (out->size >= out->capacity) {
    // Grow capacity
    out->capacity = out->capacity == 0 ? 8 : out->capacity * 2;
    out->chars = (char *)realloc(out->chars, sizeof(char) * out->capacity);
  }
}

static void push_to_output(Output *out, char c) {
  resize_output(out);
  out->chars[out->size] = c;
  out->size++;
}

static void append_to_output(Output *out, const char *const chars,
                             size_t length) {
  for (size_t i = 0; i < length; i++) {
    push_to_output(out, chars[i]);
  }
}

void out_nil(Output *out) { push_to_output(out, SERIAL_NIL); }

void out_string(Output *out, char *value, uint32_t length) {
  push_to_output(out, SERIAL_STRING);
  append_to_output(out, (char *)&length, 4);
  append_to_output(out, value, length);
}

void out_integer(Output *out, int64_t value) {
  push_to_output(out, SERIAL_INTEGER);
  append_to_output(out, (char *)value, 8);
}

void out_error(Output *out, int32_t code, const char *const message) {
  push_to_output(out, SERIAL_ERROR);
  append_to_output(out, (char *)&code, 4);
  uint32_t len = (uint32_t)strlen(message);
  append_to_output(out, (char *)&len, 4);
  append_to_output(out, message, len);
}

void out_array(Output *out, uint32_t n) {
  push_to_output(out, SERIAL_ARRAY);
  append_to_output(out, (char *)&n, 4);
}

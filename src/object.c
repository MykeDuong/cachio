#include <stdlib.h>
#include <string.h>

#include "object.h"

void initialize_object_string(ObjectString *str) {
  str->object.type = OBJECT_STRING;
  str->value = NULL;
  str->length = 0;
}

void create_string(ObjectString *str, char *chars) {
  initialize_object_string(str);
  size_t n = strlen(chars);
  str->value = malloc(sizeof(char) * (n + 1));
  memcpy(str->value, chars, strlen(chars));
  str->value[n] = '\0';
  str->length = n;
}

void replace_string(ObjectString *str, char *chars) {
  free(str->value);
  initialize_object_string(str);
  create_string(str, chars);
}

uint32_t hash_string(const char *key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

void free_string(ObjectString *str) {
  free(str->value);
  initialize_object_string(str);
}

#ifndef OBJECT_H
#define OBJECT_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum {
  OBJECT_NUMBER,
  OBJECT_STRING,
  OBJECT_BOOLEAN,
} ObjectType;

typedef struct {
  ObjectType type;
} Object;

typedef struct {
  Object object;
  char *value;
  size_t length;
} ObjectString;

typedef struct {
  Object object;
  double value;
} ObjectNumber;

typedef struct {
  Object object;
  bool value;
} ObjectBoolean;

void initialize_object_string(ObjectString *str);

void create_string(ObjectString *str, char *chars);

void replace_string(ObjectString *str, char *chars);

uint32_t hash_string(const char *key, int length);

void free_string(ObjectString *str);

#endif /* OBJECT_H */

#ifndef ENTRY_H
#define ENTRY_H

#include <stddef.h>

#include "map.h"
#include "object.h"

#define CONTAINER_OF(ptr, T, member) ((T *)((char *)ptr - offsetof(T, member)))

typedef struct {
  HashNode node;
  ObjectString key;
  ObjectString value;
} Entry;

void free_entry(Entry *entry);

#endif /* ENTRY_H */

#include "entry.h"

void free_entry(Entry *entry) {
  free_string(&entry->key);
  free_string(&entry->value);
}

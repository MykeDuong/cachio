#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "encoding.h"
#include "entry.h"
#include "map.h"
#include "object.h"
#include "store.h"

static bool entry_eq(HashNode *lhs, HashNode *rhs) {
  Entry *le = CONTAINER_OF(lhs, Entry, node);
  Entry *re = CONTAINER_OF(rhs, Entry, node);

  if (le->key.length != re->key.length)
    return false;

  return memcmp(le->key.value, re->key.value, le->key.length) == 0;
}

static void get_key_scan(HashNode *node, void *arg) {
  Output *out = (Output *)arg;
  Entry *entry = CONTAINER_OF(node, Entry, node);
  out_string(out, entry->key.value, entry->key.length);
}

void execute_keys(Command *command, Output *out) {
  (void)command;
  out_array(out, (uint32_t)get_map_size(&g_data.db));
  scan_map(&g_data.db, get_key_scan, out);
}

void execute_get(Command *command, Output *out) {
  Entry key;
  create_string(&key.key, command->strings[1]);
  key.node.hashcode = hash_string(key.key.value, key.key.length);

  HashNode *node = lookup_map(&g_data.db, &key.node, &entry_eq);

  if (!node) {
    return out_nil(out);
  }

  ObjectString *value = &CONTAINER_OF(node, Entry, node)->value;
  assert(value->length < K_MAX_MSG);
  return out_string(out, value->value, value->length);
}

void execute_set(Command *command, Output *out) {
  Entry *key = malloc(sizeof(Entry));
  create_string(&key->key, command->strings[1]);
  key->node.hashcode = hash_string(key->key.value, key->key.length);

  HashNode *node = lookup_map(&g_data.db, &key->node, &entry_eq);

  if (!node) {
    create_string(&key->value, command->strings[2]);
    insert_map(&g_data.db, &key->node);
  } else {
    replace_string(&CONTAINER_OF(node, Entry, node)->value,
                   command->strings[2]);
  }
  out_string(out, key->key.value, key->key.length);
}

void execute_delete(Command *command, Output *out) {
  Entry key;
  create_string(&key.key, command->strings[1]);
  key.node.hashcode = hash_string(key.key.value, key.key.length);

  HashNode *node = detach_map(&g_data.db, &key.node, &entry_eq);

  if (node) {
    Entry *entry = CONTAINER_OF(node, Entry, node);
    free_entry(entry);
    free(entry);
  }

  return out_integer(out, node ? 1 : 0);
}

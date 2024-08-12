#include <assert.h>

#include "map.h"

static void init_table(Table *table, size_t n) {
  assert(n > 0 && ((n - 1) & n) == 0); // n is a power of 2
  table->table = (HashNode **)calloc(sizeof(HashNode *), n);
  table->mask = n - 1;
  table->size = 0;
}

static void insert_table(Table *table, HashNode *node) {
  size_t position =
      node->hashcode & table->mask;        // Get slot from hash table array
  HashNode *next = table->table[position]; // Prepend link list
  node->next = next;
  table->table[position] = node;
  table->size++;
}

static HashNode **lookup_table(Table *table, HashNode *key,
                               bool (*eq)(HashNode *, HashNode *)) {

  if (!table->table)
    return NULL;

  size_t position =
      key->hashcode & table->mask; // Modulo operation on power of 2

  HashNode **from = &table->table[position];

  for (HashNode *cur; (cur = *from) != NULL; from = &cur->next) {
    if (cur->hashcode == key->hashcode && eq(cur, key)) {
      return from;
    }
  }
  return NULL;
}

static HashNode *detach_table(Table *table, HashNode **from) {
  HashNode *node = *from;
  *from = node->next;
  table->size--;
  return node;
}

static void scan_table(Table *table, void (*f)(HashNode *, void *), void *arg) {
  if (table->size == 0)
    return;

  for (size_t i = 0; i < table->mask + 1; ++i) {
    HashNode *node = table->table[i];
    while (node) {
      f(node, arg);
      node = node->next;
    }
  }
}

static void start_resizing_map(Map *map) {
  assert(map->t2.table == NULL);

  // Create a bigger hash table and swap them

  map->t2 = map->t1;
  init_table(&map->t1, (map->t1.mask + 1) * 2);

  map->resizing_position = 0;
}

static void help_resizing_map(Map *map) {
  size_t nwork = 0;

  while (nwork < K_RESIZING_WORK && map->t2.size > 0) {
    // Scan for nodes from t2 and move to t1
    HashNode **from = &map->t2.table[map->resizing_position];
    if (!*from) {
      map->resizing_position++;
      continue;
    }

    insert_table(&map->t1, detach_table(&map->t2, from));
    nwork++;
  }

  if (map->t2.size == 0 && map->t2.table) {
    // Finished
    free(map->t2.table);
    map->t2.size = 0;
    map->t2.mask = 0;
  }
}

void insert_map(Map *map, HashNode *node) {
  if (!map->t1.table) {
    init_table(&map->t1, 4); // Initialize table if empty
  }
  insert_table(&map->t1, node); // Insert key to the new table

  if (!map->t2.table) {
    size_t load_factor = map->t1.size / (map->t1.mask + 1);

    if (load_factor >= K_MAX_LOAD_FACTOR) {
      start_resizing_map(map); // Create a larger table
    }

    help_resizing_map(map);
  }
}

size_t get_map_size(Map *map) { return map->t1.size + map->t2.size; }

HashNode *lookup_map(Map *map, HashNode *key,
                     bool (*eq)(HashNode *, HashNode *)) {
  help_resizing_map(map);

  HashNode **from = lookup_table(&map->t1, key, eq);
  from = from ? from : lookup_table(&map->t2, key, eq);

  return from ? *from : NULL;
}

HashNode *detach_map(Map *map, HashNode *key,
                     bool (*eq)(HashNode *, HashNode *)) {
  help_resizing_map(map);

  // Try to delete in t1
  HashNode **from = lookup_table(&map->t1, key, eq);
  if (from != NULL) {
    return detach_table(&map->t1, from);
  }

  // Try to delete in t2
  from = lookup_table(&map->t2, key, eq);
  if (from != NULL) {
    return detach_table(&map->t2, from);
  }

  return NULL;
}

void scan_map(Map *map, void (*f)(HashNode *, void *), void *arg) {
  scan_table(&map->t1, f, arg);
  scan_table(&map->t2, f, arg);
}

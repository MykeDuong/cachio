#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define K_MAX_LOAD_FACTOR 8
#define K_RESIZING_WORK 128

typedef struct HashNode_t {
  struct HashNode_t *next;
  uint64_t hashcode;
} HashNode;

typedef struct {
  HashNode **table;
  size_t mask;
  size_t size;
} Table;

typedef struct {
  Table t1; // newer
  Table t2; // older
  size_t resizing_position;
} Map;

void insert_map(Map *map, HashNode *node);

size_t get_map_size(Map *map);

HashNode *lookup_map(Map *map, HashNode *key,
                     bool (*eq)(HashNode *, HashNode *));

HashNode *detach_map(Map *map, HashNode *key,
                     bool (*eq)(HashNode *, HashNode *));

void scan_map(Map *map, void (*f)(HashNode *, void *), void *arg);
#endif /* TABLE_H */

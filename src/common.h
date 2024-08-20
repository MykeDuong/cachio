#ifndef COMMON_H
#define COMMON_H

/**
 * This constant defines the maximum message size for communication
 */
#define K_MAX_MSG 4096
#define K_BUF_SIZE (K_MAX_MSG + 4)

#define DEBUG_MODE

typedef enum {
  SERIAL_NIL,
  SERIAL_ERROR,
  SERIAL_STRING,
  SERIAL_INTEGER,
  SERIAL_ARRAY,
} DataTypes;

void debug_msg(const char *const msg, ...);

void die(const char *msg);

void msg(const char *message);

#endif /* COMMON_H */

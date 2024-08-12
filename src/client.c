#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "command.h"
#include "common.h"

/**
 * @brief Read a request from the client with exactly n bytes. The
 * request is read by using the syscall read() and stored in the buffer buf.
 *
 * @param fd File descriptor of the client socket
 * @param buf Buffer to store the request
 * @param n Size of the buffer
 *
 * @return int32_t 0 if the request was read successfully, -1 otherwise
 */
static int32_t read_full(int fd, char *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = read(fd, buf, n);
    if (rv <= 0) {
      // rv == 0 -> nothing in buf, produce error
      return -1; // Error
    }
    assert((size_t)rv <= n);
    n -= (size_t)rv;
    buf += rv;
  }
  return 0;
}

/**
 * @brief Write a response to the client of exactly n bytes. The response
 * is written by using the syscall write() and stored in the buffer buf.
 *
 * @param fd File descriptor of the client socket
 * @param buf Buffer to store the response
 * @param n Size of the buffer
 *
 * @return int32_t 0 if the response was written successfully, -1 otherwise
 */
static int32_t write_full(int fd, const char *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = write(fd, buf, n);
    if (rv <= 0) {
      return -1; // Error
    }
    assert((size_t)rv <= n);
    n -= (size_t)rv;
    buf += rv;
  }
  return 0;
}

static int32_t send_request(int fd, const Command *const command) {
  uint32_t length = 4;
  for (int i = 0; i < command->count; i++) {
    length += 4 + strlen(command->strings[i]);
  }
  if (length > K_MAX_MSG) {
    return -1;
  }

  char wbuf[4 + K_MAX_MSG];
  memcpy(&wbuf[0], &length, 4); // Little endian

  uint32_t n = command->count;
  memcpy(&wbuf[4], &n, 4);

  size_t cur = 8;
  for (int i = 0; i < command->count; i++) {
    uint32_t p = (uint32_t)strlen(command->strings[i]);
    memcpy(&wbuf[cur], &p, 4);
    memcpy(&wbuf[cur + 4], command->strings[i], p);
    cur += 4 + p;
  }
  return write_full(fd, wbuf, 4 + length);
}

static int32_t on_response(const uint8_t *data, size_t size) {
  if (size < 1) {
    msg("Bad Response");
    return -1;
  }

  switch (data[0]) {
  case SERIAL_NIL:
    printf("(nil)\n");
    return 1;
  case SERIAL_ERROR:
    if (size < 1 + 8) {
      msg("Bad Response");
      return -1;
    }
    {
      int32_t code = 0;
      uint32_t len = 0;
      memcpy(&code, &data[1], 4);
      memcpy(&len, &data[1 + 4], 4);
      if (size < 1 + 8 + len) {
        msg("Bad Response");
        return -1;
      }
      printf("(err) %d %.*s\n", code, len, &data[1 + 8]);
      return 1 + 8 + len;
    }
  case SERIAL_STRING:
    if (size < 1 + 4) {
      msg("Bad Response");
      return -1;
    }
    {
      uint32_t len = 0;
      memcpy(&len, &data[1], 4);
      if (size < 1 + 4 + len) {
        msg("Bad Response");
        return -1;
      }
      printf("(string) %.*s\n", len, &data[1 + 4]);
      return 1 + 4 + len;
    }
  case SERIAL_INTEGER:
    if (size < 1 + 8) {
      msg("Bad Response");
      return -1;
    }
    {
      int64_t val = 0;
      memcpy(&val, &data[1], 8);
      printf("(integer) %ld\n", val);
      return 1 + 8;
    }

  case SERIAL_ARRAY:
    if (size < 1 + 4) {
      msg("Bad Response");
      return -1;
    }
    {
      uint32_t len = 0;
      memcpy(&len, &data[1], 4);
      printf("(arr) len=%u\n", len);
      size_t array_bytes = 1 + 4;
      for (uint32_t i = 0; i < len; ++i) {
        // Recursion
        int32_t rv = on_response(&data[array_bytes], size - array_bytes);
        if (rv < 0) {
          return rv;
        }
        array_bytes += (size_t)rv;
      }
      printf("(arr) end \n");
      return (int32_t)array_bytes;
    }
  default:
    msg("Bad Response");
    return -1;
  }
}

static int32_t read_response(int fd) {
  // 4-byte header
  char rbuf[4 + K_MAX_MSG + 1];
  errno = 0;
  int32_t err = read_full(fd, rbuf, 4);
  if (err) {
    if (errno == 0) {
      msg("EOF");
    } else {
      msg("CLIENT ERROR: read() error");
    }
    return err;
  }

  uint32_t length = 0;
  memcpy(&length, rbuf, 4); // Little endian
  if (length > K_MAX_MSG) {
    msg("CLIENT ERROR: Message too long");
    return -1;
  }

  // Reply body
  err = read_full(fd, &rbuf[4], length);
  if (err) {
    msg("CLIENT ERROR: read() error");
    return err;
  }

  // Print result
  int32_t rv = on_response((uint8_t *)&rbuf[4], length);
  if (rv > 0 && (uint32_t)rv != length) {
    msg("Bad Response");
    rv = -1;
  }
  return rv;
}

int main(int argc, char **argv) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0) {
    die("socket()");
  }

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(4413);
  addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1
  int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
  if (rv) {
    die("connect()");
  }

  Command command;
  initialize_command(&command);
  for (int i = 1; i < argc; i++) {
    add_to_command(&command, argv[i], strlen(argv[i]));
  }

  int32_t err = send_request(fd, &command);

  if (err) {
    goto L_DONE;
  }

  err = read_response(fd);
  if (err) {
    goto L_DONE;
  }

L_DONE:
  close(fd);
  free_command(&command);
  return 0;
}

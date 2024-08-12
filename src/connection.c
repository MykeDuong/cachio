#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "command.h"
#include "common.h"
#include "connection.h"
#include "encoding.h"
#include "request.h"

void initialize_connection(Connection *connection) {
  connection->fd = -1;
  connection->state = 0;
  connection->rbuf_size = 0;
  connection->wbuf_size = 0;
  connection->wbuf_sent = 0;
}

void initialize_connection_array(ConnectionArray *array) {
  array->count = 0;
  array->capacity = 0;
  array->connections = NULL;
}

void write_connection_array_with_fd(ConnectionArray *array,
                                    Connection *connection) {
  if (array->capacity < (size_t)connection->fd) {
    // Grow
    if (array->capacity == 0) {
      array->capacity = 8;
    }
    while (array->capacity <= (size_t)connection->fd) {
      array->capacity *= 2;
    }
    array->connections =
        realloc(array->connections, array->capacity * sizeof(Connection *));
  }
  array->connections[connection->fd] = connection;
  array->count = array->count > connection->fd ? array->count : connection->fd;
}

int32_t accept_new_connection(ConnectionArray *fd_to_connection, int fd) {
  // Accept connection
  struct sockaddr_in client_addr = {};
  socklen_t socklen = sizeof(client_addr);
  int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
  if (connfd < 0) {
    msg("accept() error");
    return -1; // Error
  }

  // Set the new connection fd to non-blocking mode
  fd_set_nb(connfd);

  // Creating the Connection struct
  Connection *conn = (Connection *)malloc(sizeof(Connection));
  if (!conn) {
    close(connfd);
    msg("Failed to initialize a Connection struct");
    return -1;
  }
  initialize_connection(conn);
  conn->fd = connfd;
  conn->state = STATE_REQUEST;

  // Transfer ownership
  write_connection_array_with_fd(fd_to_connection, conn);
  return 0;
}

void free_connection_array(ConnectionArray *array) {
  /**
  for (int i = 0; i < array->count; i++) {
    free(array->connections[i]);
  }
  */

  free(array->connections);
  initialize_connection_array(array);
}

void initialize_poll_args(PollArgs *args) {
  args->pfds = NULL;
  args->count = 0;
  args->capacity = 0;
}

void write_poll_args(PollArgs *args, struct pollfd fd) {
  if (args->capacity < args->count + 1) {
    // Grow
    if (args->capacity < 8) {
      args->capacity = 8;
      args->pfds = realloc(args->pfds, 8 * sizeof(struct pollfd));
    } else {
      args->capacity = args->capacity * 2;
      args->pfds = realloc(args->pfds, args->capacity * sizeof(struct pollfd));
    }
  }
  args->pfds[args->count] = fd;
  args->count++;
}

void free_poll_args(PollArgs *args) {
  if (args->pfds != NULL)
    free(args->pfds);
  initialize_poll_args(args);
}

void fd_set_nb(int fd) {
  errno = 0;
  // Get current flags
  int flags = fcntl(fd, F_GETFL, 0);
  if (errno) {
    die("fcntl()");
  }

  // Modify flags to non-blocking mode
  flags |= O_NONBLOCK;

  // Set flags
  errno = 0;
  (void)fcntl(fd, F_SETFL, flags);
  if (errno) {
    die("fcntl()");
  }
}

static bool try_flush_buffer(Connection *conn) {
  ssize_t rv = 0;
  // Try again if interrupted
  do {
    ssize_t remain = conn->wbuf_size - conn->wbuf_sent;
    rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
  } while (rv < 0 && errno == EINTR);

  // Temporary unavailable, should retry later
  if (rv < 0 && errno == EAGAIN) {
    // Got EGAIN, stop
    return false;
  }

  if (rv < 0) {
    msg("write() error");
    conn->state = STATE_END;
    return false;
  }

  conn->wbuf_sent += (size_t)rv;
  assert(conn->wbuf_sent <= conn->wbuf_size);
  if (conn->wbuf_sent == conn->wbuf_size) {
    // Respond was successfully sent, change state back
    conn->state = STATE_REQUEST;
    conn->wbuf_sent = 0;
    conn->wbuf_size = 0;
    return false;
  }

  // There is still some data in write buffer, could try to write again
  return true;
}

static void state_respond(Connection *conn) {
  while (try_flush_buffer(conn)) {
  }
}

static bool try_one_request(Connection *conn) {
  // Try to parse a request from the buffer

  if (conn->rbuf_size < 4) {
    // The read buffer does not have enough data for a request
    // Retry later
    return false;
  }

  uint32_t len = 0;
  memcpy(&len, &conn->rbuf, 4);
  if (len > K_MAX_MSG) {
    msg("Message too long");
    conn->state = STATE_END;
    return false;
  }

  if (4 + len > conn->rbuf_size) {
    // The request is not yet complete
    // Retry in next iteration
    return false;
  }

  Command command;
  initialize_command(&command);

  if (0 != parse_request(&conn->rbuf[4], len, &command)) {
    msg("Bad Request");
    free_command(&command);
    conn->state = STATE_END;
    return false;
  }

  Output out;
  initialize_output(&out);
  execute_request(&command, &out);

  // Pack the response into the buffer
  if (4 + out.size > K_MAX_MSG) {
    free_output(&out);
    out_error(&out, ERROR_TOO_BIG, "Response is too big");
  }

  uint32_t wlen = (uint32_t)out.size;
  memcpy(&conn->wbuf[0], &wlen, 4);
  memcpy(&conn->wbuf[4], out.chars, out.size);
  conn->wbuf_size = 4 + wlen;

  // remove request from buffer
  size_t remain = conn->rbuf_size - 4 - len;
  if (remain) {
    memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
  }
  conn->rbuf_size = remain;

  // Change state
  conn->state = STATE_RESPOND;
  state_respond(conn);
  free_command(&command);
  free_output(&out);

  // Continue the outer loop if the process was fully processed
  return (conn->state == STATE_REQUEST);
}

static bool try_fill_buffer(Connection *conn) {
  assert(conn->rbuf_size < sizeof(conn->rbuf));
  ssize_t rv = 0;

  // Loop if interrupted
  do {
    size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
    rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
  } while (rv < 0 && errno == EINTR);

  // Temporary unavailable, should retry later
  if (rv < 0 && errno == EAGAIN) {
    // Got EAGAIN, stop
    return false;
  }

  if (rv < 0) {
    msg("read() error");
    conn->state = STATE_END;
    return false;
  }

  if (rv == 0) {
    if (conn->rbuf_size > 0) {
      msg("Unexpected EOF");
    } else {
      msg("EOF");
    }
    conn->state = STATE_END;
    return false;
  }

  conn->rbuf_size += (size_t)rv;
  assert(conn->rbuf_size < sizeof(conn->rbuf));

  while (try_one_request(conn)) {
  }
  return (conn->state == STATE_REQUEST);
}

static void state_request(Connection *conn) {
  while (try_fill_buffer(conn)) {
  }
}

void connection_io(Connection *conn) {
  if (conn->state == STATE_REQUEST) {
    state_request(conn);
  } else if (conn->state == STATE_RESPOND) {
    state_respond(conn);
  } else {
    msg("Invalid Connection state");
    assert(0);
  }
}

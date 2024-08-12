#ifndef CONNECTION_H
#define CONNECTION_H

#include <arpa/inet.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <unistd.h>

enum {
  STATE_REQUEST = 0,
  STATE_RESPOND = 1,
  STATE_END = 2, // Mark connnection for deletion
};

/**
 * This structure represents a connection to a client. It contains the file
 * descriptor of the client socket, the state of the connection (either
 * STATE_REQUEST or STATE_RESPOND), the reading buffer, and the writing buffer.
 * The reading buffer is used to store the request from the client, and the
 * writing buffer is used to store the response to the client. This is due to
 * the connections being non-blocking, and the need to read and write in chunks.
 */
typedef struct Connection {
  int fd;
  uint32_t state; // Either STATE_REQ / STATE_RES
  // reading buffer
  size_t rbuf_size;
  uint8_t rbuf[K_BUF_SIZE];
  // writing buffer
  size_t wbuf_size;
  size_t wbuf_sent;
  uint8_t wbuf[K_BUF_SIZE];
} Connection;

/**
 * This structure represents an array of connections. It contains an array of
 * pointers to connections, the capacity of the array, and the number of
 * connections in the array.
 */
typedef struct ConnectionArray {
  Connection **connections;
  int capacity;
  int count;
} ConnectionArray;

typedef struct PollArgs {
  struct pollfd *pfds;
  int capacity;
  int count;
} PollArgs;

/**
 * @brief Initialize a connection. This function sets the file descriptor to -1,
 * the state to STATE_REQUEST, and the reading and writing buffers to zero.
 */
void initialize_connection(Connection *connection);

/**
 * @brief Initialize a connection array. This function sets the capacity and
 * count to 0, and the connections' pointers array to NULL.
 *
 * @param array ConnectionArray to initialize
 */
void initialize_connection_array(ConnectionArray *array);

/**
 * @brief Add a connection to the connection array. This function adds a
 * connection to the array of connections, and increases the count of
 * connections in the array with the index of the inserted connection being its
 * fd. If the array is full, it reallocates the array with double the capacity.
 * If the array is currently NULL, it allocates the array with an initial
 * capacity of 8.
 *
 * @param array ConnectionArray to add the connection to
 * @param connection Connection pointer to add to the array
 */
void write_connection_array_with_fd(ConnectionArray *array,
                                    Connection *connection);

/**
 * @brief Accept a new connection. This function accepts a new connection on the
 * server socket, and adds the new connection to the connection array. If the
 * connection struct is NULL, it will return -1.
 *
 * @param fd_to_connection ConnectionArray to add the new connection to
 * @param fd File descriptor of the server socket
 *
 * @return int32_t 0 if the new connection was accepted successfully, -1
 * otherwise
 */
int32_t accept_new_connection(ConnectionArray *fd_to_connection, int fd);

/**
 * @brief Free the whole ConnectionArray. This function frees all the
 * connections in the array, and then frees the array itself.
 *
 * @param array ConnectionArray to free
 */
void free_connection_array(ConnectionArray *array);

/**
 * @brief Initialize a PollArgs structure. This function sets the capacity and
 * count to 0, and the file descriptor array to NULL.
 *
 * @param args PollArgs to initialize
 */
void initialize_poll_args(PollArgs *args);

/**
 * @brief Add a file descriptor to the PollArgs structure. This function adds a
 * file descriptor to the array of file descriptors, and increases the count of
 * file descriptors in the array. If the array is full, it reallocates the array
 * with double the capacity. If the array is currently NULL, it allocates the
 * array with an initial capacity of 8.
 *
 * @param args PollArgs to add the file descriptor to
 * @param fd File descriptor to add to the array
 */
void write_poll_args(PollArgs *args, struct pollfd fd);

/**
 * @brief Free the whole PollArgs structure. This function frees the file
 * descriptor array.
 *
 * @param args PollArgs to free
 */
void free_poll_args(PollArgs *args);

/**
 * @brief Set a file descriptor (fd) to non-blocking mode.
 *
 * @param fd File descriptor to set to non-blocking mode
 */
void fd_set_nb(int fd);

void connection_io(Connection *connection);

#endif /* CONNECTION_H */

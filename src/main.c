#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "connection.h"

int main() {
  // fd for the TCP socket
  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0) {
    die("socket()");
  }

  int val = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(4413);
  addr.sin_addr.s_addr = ntohl(0);
  int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
  if (rv) {
    die("bind()");
  }

  rv = listen(fd, SOMAXCONN);
  if (rv) {
    die("listen()");
  }

  ConnectionArray fd_to_connections;
  initialize_connection_array(&fd_to_connections);

  // Set the listening fd to non-blocking mode
  fd_set_nb(fd);

  // Event loop
  PollArgs args;
  initialize_poll_args(&args);
  while (true) {
    // Prepare the arguments of poll()
    free_poll_args(&args);

    // Listening fd is in the first position
    struct pollfd pfd = {fd, POLLIN, 0};
    write_poll_args(&args, pfd);

    // Connection fds;
    for (int i = 0; i < fd_to_connections.capacity; i++) {
      Connection *connection = fd_to_connections.connections[i];
      // Skip NULL connections
      if (connection == NULL)
        continue;

      struct pollfd pfd = {};
      pfd.fd = connection->fd;
      pfd.events = (connection->state == STATE_REQUEST) ? POLLIN : POLLOUT;
      pfd.events = pfd.events | POLLERR;
      write_poll_args(&args, pfd);
    }

    // Poll active fds, both listening and client fds
    int rv = poll(args.pfds, (nfds_t)args.count, 1000);
    if (rv < 0) {
      die("poll()");
    }

    // Process client fds
    for (size_t i = 1; i < args.capacity; i++) {
      if (args.pfds[i].revents) {
        Connection *connection = fd_to_connections.connections[args.pfds[i].fd];
        connection_io(connection);
        if (connection->state == STATE_END) {
          // Destroy
          fd_to_connections.connections[connection->fd] = NULL;
          (void)close(connection->fd);
          free(connection);
        }
      }
    }

    // Try accepting new connection if the listening fd is active
    if (args.pfds[0].revents) {
      (void)accept_new_connection(&fd_to_connections, fd);
    }
  }

  free_poll_args(&args);
  free_connection_array(&fd_to_connections);

  return 0;
}

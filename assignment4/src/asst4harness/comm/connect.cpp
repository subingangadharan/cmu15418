// Copyright 2013 15418 Course Staff.

#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "./connect.h"

static void parse_hostport(char* hostport, char** host, char** port) {
  // TODO(awreece) Support IPv6.
  char* state;

  *host = strtok_r(hostport, ":", &state);
  *port = strtok_r(NULL, ":", &state);
}

static int try_connect(int fd, const struct addrinfo* res) {
  return connect(fd, res->ai_addr, res->ai_addrlen);
}

static int try_bind_listen(int fd, const struct addrinfo* res) {
  int optval = 1;
  int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  if (ret < 0) {
    return ret;
  }
  ret = bind(fd, res->ai_addr, res->ai_addrlen);
  if (ret < 0) {
    return ret;
  }

  return listen(fd, MAX_BACKLOG);
}

// I *could* use templates, but I don't understand C++.
static int connect_common(char* hostport,
                          int (*try_socket)(int fd, const struct addrinfo* res),
                          int flags) {
  struct addrinfo *result;
  struct addrinfo *res;
  struct addrinfo hints;
  int error;
  int sfd;
  char* host;
  char* port;

  parse_hostport(hostport, &host, &port);
  if (port == NULL) {
    // They didn't give us a port.
    return -1;
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family= PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_NUMERICSERV | flags;

  error = getaddrinfo(host, port, &hints, &result);
  if (error != 0) {
    return -1;
  }

  for (res = result; res != NULL; res = res->ai_next) {
    sfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sfd == -1) continue;

    if (try_socket(sfd, res) == 0) {
      break;
    }

    close(sfd);
  }

  if (res == NULL) {
    sfd = -1;
  }

  freeaddrinfo(result);
  // TODO(awreece) This is bad and I feel bad, but I'm lazy.
  port[-1] = ':';
  return sfd;
}

int connect_to(const char* hostport) {
  char addr[128];
  strncpy(addr, hostport, 127);
  addr[127] = 0;
  return connect_common(addr, try_connect, 0);
}


int listen_to(const char* hostport) {
  char addr[128];
  strncpy(addr, hostport, 127);
  addr[127] = 0;
  return connect_common(addr, try_bind_listen, AI_PASSIVE);
}

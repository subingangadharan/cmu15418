// Copyright 2013 15418 Course Staff.

#ifndef COMM_CONNECT_H
#define COMM_CONNECT_H

#define MAX_BACKLOG 128

int connect_to(const char* hostport);
int listen_to(const char* hostport);

#endif // COMM_CONNECT_H

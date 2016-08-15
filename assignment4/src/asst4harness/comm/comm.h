// Copyright 2013 15418 Course Staff.

#ifndef COMM_COMM_H_
#define COMM_COMM_H_

#include <string>

#include "types/types.h"

int recv_message(int fd, message_t* message, int* tag);
int send_message(int fd, const message_t message, const int tag);

int recv_work(int fd, work_t* work);
int send_work(int fd, const work_t& work, int tag);

int recv_worker_stats(int fd, worker_stats_t* stats);
int send_worker_stats(int fd, const worker_stats_t& stats);

int recv_resp(int fd, resp_t* resp);
int send_resp(int fd, const resp_t& resp);
int send_resp(int fd, const resp_t& resp, int tag);

int send_string(int fd, const std::string& args);

#endif  // COMM_COMM_H_

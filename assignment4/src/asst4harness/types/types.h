// Copyright 2013 15418 Course Staff.

#ifndef TYPES_H_
#define TYPES_H_

#include <boost/shared_ptr.hpp>

#include <iostream>

typedef enum {
  WORK,
  RESPONSE,
  NEW_WORKER,
  REQUEST_STATS,
  STATS,
  ISREADY,
  SHUTDOWN,
  WORKER_UP_TIME_STATS
} message_t;

typedef struct {
  message_t message;
  int tag;
} tagged_message_t;

typedef struct {
  int cpu_threads;
  int memory_threads;
  int io_threads;
} worker_stats_t;

typedef struct {
  int buf_len;
  boost::shared_ptr<char[]> buf;
} work_t;

typedef struct {
  int buf_len;
  boost::shared_ptr<char[]> buf;
} resp_t;

std::ostream& operator<< (std::ostream &out, const work_t& work);
std::ostream& operator<< (std::ostream &out, const resp_t& resp);
std::ostream& operator<< (std::ostream &out, const message_t& work);
std::ostream& operator<< (std::ostream &out, const worker_stats_t& stats);

#endif  // TYPES_H_

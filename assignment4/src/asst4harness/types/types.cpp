// Copyright 2013 15418 Course Staff.

#include <glog/logging.h>

#include <iostream> // NOLINT

#include "types/types.h"

std::ostream& operator<< (std::ostream &out, const work_t &work) {
  return out << "Work (len=" << work.buf_len << ", buf=" << work.buf << ")";
}

std::ostream& operator<< (std::ostream &out, const resp_t &resp) {
  return out << "Resp(buf_len=" << resp.buf_len << ", buf=" << resp.buf << ")";
}

std::ostream& operator<< (std::ostream &out, const message_t &message) {
  switch (message) {
    case WORK:
      out << "WORK";
      break;
    case RESPONSE:
      out << "RESPONSE";
      break;
    case NEW_WORKER:
      out << "NEW_WORKER";
      break;
    case REQUEST_STATS:
      out << "REQUEST_STATS";
      break;
    case STATS:
      out << "STATS";
      break;
    case ISREADY:
      out << "ISREADY";
      break;
    case SHUTDOWN:
      out << "SHUTDOWN";
      break;
    case WORKER_UP_TIME_STATS:
      out << "WORKER_UP_TIME_STATS";
      break;
    default:
      LOG(FATAL) << "Invalid message " << std::hex << static_cast<int>(message);
  }
  return out;
}

std::ostream& operator<< (std::ostream &out, const worker_stats_t &stats) {
  return out << "Stats(cpu_threads=" << stats.cpu_threads
             << ", memory_threads=" << stats.memory_threads
             << ", io_threads=" << stats.io_threads << ")";
}

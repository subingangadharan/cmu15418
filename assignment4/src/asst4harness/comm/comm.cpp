// Copyright 2013 15418 Course Staff.

#include <assert.h>
#include <boost/make_shared.hpp>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>

#include "comm/comm.h"

static int send_all(int fd, const void* buf, size_t len) {
  const char* cbuf = reinterpret_cast<const char*>(buf);
  size_t sent = 0;
  do {
    ssize_t ret = send(fd, &cbuf[sent], len - sent, 0);
    if (ret == -1 && errno != EINTR) {
      return -1;
    } else if (ret == 0) {
      return -1;
    }
    sent += ret;
  } while (sent < len);

  return 0;
}

static int recv_all(int fd, void* buf, size_t len) {
  char* cbuf = reinterpret_cast<char*>(buf);
  size_t received = 0;
  do {
    ssize_t ret = recv(fd, &cbuf[received], len - received, 0);
    if (ret == -1 && errno != EINTR) {
      return -1;
    } else if (ret == 0) {
      return -1;
    }
    received += ret;
  } while (received < len);

  return 0;
}

int send_message(int fd, const message_t message, const int tag) {
  tagged_message_t to_send;

  to_send.message = message;
  to_send.tag = tag;

  return send_all(fd, &to_send, sizeof(to_send));
}

int recv_message(int fd, message_t* message, int* tag) {
  tagged_message_t to_recv;

  int ret = recv_all(fd, &to_recv, sizeof(to_recv));

  if (ret == 0) {
    *tag = to_recv.tag;
    *message = to_recv.message;
  }

  return ret;
}

int recv_work(int fd, work_t* work) {
  int err = recv_all(fd, &work->buf_len, sizeof(work->buf_len));
  if (err == 0) {
    work->buf = boost::make_shared<char[]>(work->buf_len);
    err = recv_all(fd, work->buf.get(), work->buf_len);
  }
  return err;
}

int send_work(int fd, const work_t& work, int tag) {
  int err = send_message(fd, WORK, tag);
  if (err == 0) {
    err = send_all(fd, &work.buf_len, sizeof(work.buf_len));
    if (err == 0) {
      err = send_all(fd, work.buf.get(), work.buf_len);
    }
  }
  return err;
}

int recv_resp(int fd, resp_t* resp) {
  int err = recv_all(fd, &resp->buf_len, sizeof(resp->buf_len));
  if (err == 0) {
    resp->buf = boost::make_shared<char[]>(resp->buf_len);
    err = recv_all(fd, resp->buf.get(), resp->buf_len);
  }
  return err;
}

int send_resp(int fd, const resp_t& resp, int tag) {
  int err = send_message(fd, RESPONSE, tag);
  if (err == 0) {
    err = send_all(fd, &resp.buf_len, sizeof(resp.buf_len));
    if (err == 0) {
      err = send_all(fd, resp.buf.get(), resp.buf_len);
    }
  }
  return err;
}

int recv_worker_stats(int fd, worker_stats_t* stats) {
  return recv_all(fd, stats, sizeof(*stats));
}

int send_worker_stats(int fd, const worker_stats_t& stats) {
  return send_all(fd, &stats, sizeof(stats));
}

int send_string(int fd, const std::string& s) {
  int len = s.length();
  assert(sizeof(len) == 4);
  int err = send_all(fd, &len, sizeof(len));
  if (err < 0) return err;
  return send_all(fd, s.c_str(), len);
}

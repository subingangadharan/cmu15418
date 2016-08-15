// Copyright 2013 15418 Course Staff.
// This was most helpful: http://eradman.com/posts/kqueue-tcp.html

#include <assert.h>
#include <boost/unordered_set.hpp>
#include <errno.h>
#include <event.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>
#include <netinet/in.h>
#include <boost/make_shared.hpp>

#include "comm/comm.h"
#include "types/types.h"
#include "server/messages.h"
#include "server/master.h"

#include  "tools/cycle_timer.h"

#define MAX_EVENTS 1024

extern int launcher_fd;
extern int accept_fd;

DEFINE_bool(log_network, false, "Log network traffic.");

#define NETLOG(level) DLOG_IF(level, FLAGS_log_network)

static bool is_server_initialized = false;
static int num_instances_booted = 0;
static double total_worker_seconds;

std::map<Worker_handle, double> worker_boot_times;
boost::unordered_set<Worker_handle> workers;

static void close_connection(void* connection_handle) {
  struct event* event = reinterpret_cast<struct event*>(connection_handle);
  CHECK_NE(EVENT_FD(event), accept_fd) << "Critical connection failed\n";
  CHECK_NE(EVENT_FD(event), launcher_fd) << "Critical connection failed\n";

  // We should never call close_connection() on a worker handle, because
  // kill_worker() first removes the worker from the worker set and then
  // we remove it from the event loop here.
  CHECK(workers.find(connection_handle) == workers.end())
    << "Unexpected close of worker handle " << EVENT_FD(event);

  NETLOG(INFO) << "Connection closed " << EVENT_FD(event);

  PLOG_IF(ERROR, close(EVENT_FD(event)))
    << "Error closing fd " << EVENT_FD(event);
  LOG_IF(ERROR, event_del(event) < 0)
    << "Error deleting event " << EVENT_FD(event);
  delete event;
}

unsigned pending_worker_requests = 0;
void request_new_worker_node(const Request_msg& req) {

  // HACK(kayvonf): stick the tag in the dictionary to avoid a lot of
  // extra plumbing
  Request_msg modified(req);

  char tmp_buffer[32];
  sprintf(tmp_buffer, "%d", req.get_tag());
  modified.set_arg("tag", tmp_buffer);

  std::string str = modified.get_request_string();

  DLOG(INFO) << "Requesting worker " << str;
  CHECK_EQ(send_string(launcher_fd, str), 0)
    << "Cannot talk launcher\n";
  pending_worker_requests++;
}

static void accumulate_time(Worker_handle worker_handle) {
  double start_time = worker_boot_times[worker_handle];
  double end_time = CycleTimer::currentSeconds();
  double worker_up_time = end_time - start_time;
  total_worker_seconds += worker_up_time;

  //printf("*** MASTER: accumulating %.2f sec\n", worker_up_time);
}

void kill_worker_node(Worker_handle worker_handle) {

  CHECK_EQ(workers.erase(worker_handle), 1U) << "Attempt to kill non worker";
  close_connection(worker_handle);
  accumulate_time(worker_handle);
  worker_boot_times.erase(worker_handle);
}

void send_request_to_worker(Client_handle worker_handle, const Request_msg& job) {
  work_t comm_work;

  std::string contents = job.get_request_string();
  int allocation_size = contents.size();
  comm_work.buf = boost::make_shared<char[]>(allocation_size);
  comm_work.buf_len = allocation_size;
  strncpy(comm_work.buf.get(), contents.c_str(), allocation_size);

  // now perform the send
  CHECK(workers.find(worker_handle) != workers.end())
    << "Attempt to send work to invalid worker";
  // TODO(awreece) Lock the worker handle!
  struct event* event = reinterpret_cast<struct event*>(worker_handle);
  NETLOG(INFO) << "Sending work (" << job.get_tag() << "," << comm_work << ") to "
               << EVENT_FD(event);
  CHECK_EQ(send_work(EVENT_FD(event), comm_work, job.get_tag()), 0)
    << "Unexpected connection failure with worker " << EVENT_FD(event);
}

void send_client_response(Client_handle client_handle, const Response_msg& resp) {

  resp_t comm_resp;

  std::string resp_str = resp.get_response();
  int allocation_size = resp_str.size();
  comm_resp.buf = boost::make_shared<char[]>(allocation_size);
  comm_resp.buf_len = allocation_size;
  strncpy(comm_resp.buf.get(), resp_str.c_str(), allocation_size);

  // send to comm layer
  struct event* event = reinterpret_cast<struct event*>(client_handle);
  NETLOG(INFO) << "Sending response " << comm_resp << " to " << EVENT_FD(event);
  CHECK_EQ(send_resp(EVENT_FD(event), comm_resp, 0), 0)
    << "Unexpected connection failure with client " << EVENT_FD(event);
}

void server_init_complete() {
  is_server_initialized = true;
}

static void shutdown() {
  LOG(INFO) << "Shutting down";
  exit(0);
}

bool should_shutdown = false;
static void handle_read(int fd, int16_t events, void* arg) {
  assert(events & EV_READ);
  message_t message;
  int tag;
  int err = recv_message(fd, &message, &tag);
  if (err < 0) {
    NETLOG(WARNING) << "Connection closed on " << fd;
    close_connection(arg);
    return;
  }

  NETLOG(INFO) << "Got message (" << message << "," << tag << ")";

  switch (message) {

  case ISREADY: {

    resp_t comm_resp;
    std::string resp_str( is_server_initialized ? "ready" : "not_ready" );

    int allocation_size = resp_str.size();
    comm_resp.buf = boost::make_shared<char[]>(allocation_size);
    comm_resp.buf_len = allocation_size;
    strncpy(comm_resp.buf.get(), resp_str.c_str(), allocation_size);

    // send to comm layer
    struct event* event = reinterpret_cast<struct event*>(arg);
    NETLOG(INFO) << "Sending response " << comm_resp << " to " << EVENT_FD(event);
    CHECK_EQ(send_resp(EVENT_FD(event), comm_resp, 0), 0)
      << "Unexpected connection failure with client " << EVENT_FD(event);

    close_connection(arg);
    break;
  }

  case WORKER_UP_TIME_STATS: {

    // Accumulate time for all the workers that HAVE NOT yet been shut
    // down
    for (std::map<Worker_handle, double>::const_iterator it=worker_boot_times.begin();
         it != worker_boot_times.end(); it++)
      accumulate_time(it->first);

    resp_t comm_resp;

    char tmp_buffer[128];
    sprintf(tmp_buffer,"%d %.2f", num_instances_booted, total_worker_seconds);
    std::string resp_str(tmp_buffer);

    int allocation_size = resp_str.size();
    comm_resp.buf = boost::make_shared<char[]>(allocation_size);
    comm_resp.buf_len = allocation_size;
    strncpy(comm_resp.buf.get(), resp_str.c_str(), allocation_size);

    // send to comm layer
    struct event* event = reinterpret_cast<struct event*>(arg);
    NETLOG(INFO) << "Sending response " << comm_resp << " to " << EVENT_FD(event);
    CHECK_EQ(send_resp(EVENT_FD(event), comm_resp, 0), 0)
      << "Unexpected connection failure with client " << EVENT_FD(event);

    close_connection(arg);
    break;
  }

    case SHUTDOWN: {
      if (pending_worker_requests == 0) {
  shutdown();
      } else {
  should_shutdown = true;
      }
    }
    case WORK: {
      // A new request from a client.
      work_t work;
      // TODO(awreece) This *ought* to be a buffered read.
      if (recv_work(fd, &work) < 0) {
        NETLOG(ERROR) << "Unexpected connection close on " << fd;
        close_connection(arg);
        return;
      }
      NETLOG(INFO) << "Got new work " << work << " from " << fd;

      // HACK(kayvonf): convert a work_t into a Request_msg to pass to student code
      // Since the content in work_t.buf is not null terminated, this is a big mess
      int len = work.buf_len;
      char* tmp_buffer = new char[len+1];
      strncpy(tmp_buffer, work.buf.get(), len);
      tmp_buffer[len] = '\0';

      Request_msg client_req(0, tmp_buffer);
      delete [] tmp_buffer;

      handle_client_request(arg, client_req);
      break;
    }

    case RESPONSE: {
      // Worker job is done response.
      resp_t comm_resp;
      if (recv_resp(fd, &comm_resp) < 0) {
        NETLOG(ERROR) << "Unexpected connection close on " << fd;
        close_connection(arg);
        return;
      }
      NETLOG(INFO) << "Got worker response (" << tag << "," << comm_resp
        << ") from " << fd;

      // HACK(kayvonf): convert a resp_t into a Response_msg to pass to student code
      // Since the content in resp_t.buf is not null terminated, this is a big mess
      int len = comm_resp.buf_len;
      char* tmp_buffer = new char[len+1];
      strncpy(tmp_buffer, comm_resp.buf.get(), len);
      tmp_buffer[len] = '\0';
      Response_msg resp(tag);
      resp.set_response(tmp_buffer);
      delete [] tmp_buffer;

      handle_worker_response(arg, resp);
      break;
    }

    case NEW_WORKER: {
      pending_worker_requests--;
      if (should_shutdown && pending_worker_requests == 0) {
  shutdown();
      }
      // Notification that a worker has booted.
      NETLOG(INFO) << "New worker " << tag << " on " << fd;
      workers.insert(arg);
      worker_boot_times[arg] = CycleTimer::currentSeconds();
      num_instances_booted++;
      handle_new_worker_online(arg, tag);
      break;
    }

    default: {
      NETLOG(ERROR) << "Unexpected message " << message << " from " << fd;
      close_connection(arg);
      return;
    }
  }
}

static void handle_accept(int fd, int16_t events, void* arg) {
  (void)arg;
  assert(events & EV_READ);

  struct sockaddr addr;
  socklen_t addr_len = sizeof(addr);
  fd = accept(fd, &addr, &addr_len);

  PCHECK(fd >= 0) << "Failure accepting new connection!";
  NETLOG(INFO) << "New connection on " << fd;

  // So I *ought* to use bufferevents, but they change the API significantly. I
  // think I'll go for readability here over what I suspect is a negligable
  // improvement in performance.
  // TODO(awreece) Use bufferevents?

  // Send event struct as arg to make it easy to stop the event.
  struct event* event = new struct event;
  // I would really rather use event_self_cbarg().
  event_set(event, fd, EV_READ|EV_PERSIST, handle_read, event);
  event_add(event, NULL);
}

static void handle_timer(int fd, int16_t events, void* arg) {
  (void)fd;
  (void)events;
  (void)arg;

  NETLOG(INFO) << "Timer tick";
  handle_tick();
}

void harness_init() {
  num_instances_booted = 0;
  total_worker_seconds = 0.0;
}

void harness_begin_main_loop(struct timeval* tick_period) {
  event_init();
  struct event accept_event, timer_event;

  // Set up the accept event.
  event_set(&accept_event, accept_fd, EV_READ|EV_PERSIST,
            handle_accept, &accept_event);
  event_add(&accept_event, NULL);

  // Set up the timer event.
  event_set(&timer_event, -1, EV_PERSIST, handle_timer, NULL);
  event_add(&timer_event, tick_period);

  NETLOG(INFO) << "Starting event loop";
  event_dispatch();
}

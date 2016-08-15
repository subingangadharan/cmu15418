// Copyright 2013 15418 Course Staff


#include <getopt.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <boost/make_shared.hpp>

#include <string>

#include "comm/connect.h"
#include "comm/comm.h"
#include "server/messages.h"
#include "server/worker.h"

extern void init_work_engine();


static int master_fd = -1;
DEFINE_int32(cpu_threads, 2, "Number of threads to use");
DEFINE_int32(memory_threads, 2, "Number of threads to use");
DEFINE_int32(io_threads, 2, "Number of threads to use");
DEFINE_int32(tag, 0, "Tag to send when initially connecting to the master");

DEFINE_bool(log_network, false, "Log network traffic.");
DEFINE_bool(force_disk_io, false, "Force diskIO.");
DEFINE_bool(fast_boot, false, "Enable fast booting (don't artificially delay boot time)");

DEFINE_string(workerparams, "", "Student specified commandline args");
//DEFINE_string(assets_dir, "/afs/cs/academic/class/15418-s13/public/data", "Assets directory");
DEFINE_string(assets_dir, "./data", "Assets directory");


// You should probably hold onto this when writing to master_fd.
pthread_mutex_t master_write_lock = PTHREAD_MUTEX_INITIALIZER;

// seconds
const int WORKER_BOOT_LATENCY = 1;

void harness_boot_worker(bool fastBoot) {

  char worker_hostname[1024];
  gethostname(worker_hostname, 1023);

  DLOG(INFO) << "Booting worker. Hostname: " << worker_hostname << std::endl;

  if (!fastBoot) {
    sleep(WORKER_BOOT_LATENCY);
  }

  init_work_engine();
}

void harness_connect_to_master(const std::string& port, int tag) {

  master_fd = connect_to(port.c_str());
  CHECK_GE(master_fd, 0) << "Worker could not connect to master" << port;
  DLOG(INFO) << "Connected to master " << port;

  CHECK_GE(send_message(master_fd, NEW_WORKER, tag), 0)
    << "Couldn't register with master";

}

void harness_begin_main_loop() {

  work_t work;
  int tag;
  message_t message;
  while (recv_message(master_fd, &message, &tag) == 0) {
    if (message == REQUEST_STATS) {
      //  DLOG_IF(INFO, FLAGS_log_network) << "Master requested stats";
      //  CHECK_GE(send_stats(master_fd), 0) << "Error sending to master";
      continue;
    }
    CHECK_EQ(message, WORK) << "Invalid message type " << message;
    CHECK_GE(recv_work(master_fd, &work), 0) << "Error receiving from master";

    DLOG_IF(INFO, FLAGS_log_network) << "Got new work (" << tag << "," << work
                                     << ") from master";

    // convert a work_t into a Request_msg to pass to student code
    // Since the content in work_t.buf is not null terminated, this is
    // a big mess
    int len = work.buf_len;
    char* tmp_buffer = new char[len+1];
    strncpy(tmp_buffer, work.buf.get(), len);
    tmp_buffer[len] = '\0';

    Request_msg req(tag, tmp_buffer);
    delete [] tmp_buffer;

    // student code
    worker_handle_request(req);
  }

  char worker_hostname[1024];
  gethostname(worker_hostname, 1023);
  DLOG(INFO) << "Worker on " << worker_hostname << " is shutting down (master terminated connection)" << std::endl;
}

void worker_send_response(const Response_msg& resp) {

  resp_t comm_resp;
  int tag = resp.get_tag();
  int err;

  // convert student-friendly Response_msg object to the comm layer's
  // resp_t
  std::string resp_str = resp.get_response();
  int allocation_size = resp_str.size();
  comm_resp.buf = boost::make_shared<char[]>(allocation_size);
  comm_resp.buf_len = allocation_size;
  strncpy(comm_resp.buf.get(), resp_str.c_str(), allocation_size);

  // send the reponse to the master node
  //DLOG_IF(INFO, FLAGS_log_network) << work << " => " << comm_resp;
  pthread_mutex_lock(&master_write_lock);
  err = send_resp(master_fd, comm_resp, tag);
  pthread_mutex_unlock(&master_write_lock);
  CHECK_GE(err, 0) << "Error writing to master!";
  DLOG_IF(INFO, FLAGS_log_network) << tag << "," << comm_resp << ") to master";

}

int main(int argc, char** argv) {

  std::string usage("Usage: " + std::string(argv[0]) +
                    " [options] <hostport>\n");
  usage += "  Runs a worker node with master that is running on host:port.";
  google::SetUsageMessage(usage);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  google::ParseCommandLineFlags(&argc, &argv, true);
  if (argc < 2) {
    fprintf(stderr, "Insufficient arguments provided\n%s\n",
             google::ProgramUsage());
    exit(EXIT_FAILURE);
  }

  std::string port = argv[1];

  //harness_boot_worker(FLAGS_fast_boot, FLAGS_force_disk_io, FLAGS_assets_dir);
  harness_boot_worker(FLAGS_fast_boot);

  Request_msg boot_req(0, FLAGS_workerparams);

  //int tag = FLAGS_tag;
  int tag = atoi(boot_req.get_arg("tag").c_str());

  harness_connect_to_master(port, tag);

  // student code
  worker_node_init( boot_req );

  harness_begin_main_loop();

  return 0;
}

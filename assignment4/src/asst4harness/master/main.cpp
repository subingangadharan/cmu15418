// Copyright 2013 15418 Course Staff

#include <getopt.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <string>

#include "comm/connect.h"
#include "comm/comm.h"
#include "server/master.h"

void harness_init();
void harness_begin_main_loop(struct timeval* tick_period);

int launcher_fd = -1;
int accept_fd = -1;

DEFINE_string(address, "localhost:15418", "What address to listen on.");
DECLARE_bool(log_network);
DEFINE_int32(max_workers, 2, "Maximum number of workers the master can request");

int main(int argc, char** argv) {
  int err;
  std::string usage("Usage: " + std::string(argv[0]) +
                    " [options] <hostport>\n");
  usage += "  Runs a master node with launcher that is running on host:port.";
  google::SetUsageMessage(usage);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  google::ParseCommandLineFlags(&argc, &argv, true);
  if (argc != 2) {
    fprintf(stderr, "Invalid number of aruments provided\n%s\n",
            google::ProgramUsage());
    exit(EXIT_FAILURE);
  }

  accept_fd = listen_to(FLAGS_address.c_str());
  CHECK_GE(accept_fd, 0) << "Could not listen on " << FLAGS_address;
  DLOG_IF(INFO, FLAGS_log_network) << "Listening on " << FLAGS_address;

  DLOG_IF(INFO, FLAGS_log_network) << "Waiting for launcher " << argv[1];
  while (launcher_fd < 0) {
    sleep(1);
    launcher_fd = connect_to(argv[1]);
  }
  DLOG_IF(INFO, FLAGS_log_network) << "Connected to launcher at " << argv[1];

  // Tell the launcher what address we are listening on.
  err = send_string(launcher_fd, FLAGS_address);
  CHECK_GE(err, 0) << "Error sending master info";

  harness_init();

  // student code
  int tick_seconds;
  master_node_init(FLAGS_max_workers, tick_seconds);

  struct timeval tick_period;
  tick_period.tv_sec = tick_seconds;
  tick_period.tv_usec = 0;

  harness_begin_main_loop(&tick_period);

  return 0;
}

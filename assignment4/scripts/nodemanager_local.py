#!/usr/bin/env python2.7

import argparse
import comm
import collections
import re
import signal
from subprocess import Popen
import socket
import struct
import sys
import threading
import random
import os

dirname = os.path.dirname(os.path.realpath(__file__))

signal.signal(signal.SIGCHLD, signal.SIG_IGN)

def positive_int(value):
  ivalue = int(value)
  if ivalue <= 0:
      raise argparse.ArgumentTypeError("%s is not a positive int value" % value)
  return ivalue

parser = argparse.ArgumentParser(description="Launch workers")
parser.add_argument("--verbose", help="Verbose output", action='store_true')
parser.add_argument("--host", action='append', default=[],
     help="If present, launch on these hosts. Can be passed multiple times.")
parser.add_argument("--nodefile");
parser.add_argument("port", help="Port to listen on", type=positive_int)
parser.add_argument("worker_args",
    nargs=argparse.REMAINDER,
    help="All excess arguments are passed to the worker.");

args = parser.parse_args()
args.verbose = True;

launch_on_remote_hosts = False;
remote_cwd = '.';

# HACK(kayvonf): two ways to let this script know it is running on latedays.
if args.nodefile is not None or len(args.host) > 0:

  launch_on_remote_hosts = True;
  # this is the configuration run on latedays.andrew.cmu.edu just read
  # the PBS Hostsfile directly. The filename is specified via
  # commandline argument.
  if args.nodefile is not None:
    count = 0;
    master_hostname = '';
    input_file = open(args.nodefile, 'r');
    worker_hostnames = [];
    remote_cwd = os.getcwd();
    for line in input_file:
      host = line.strip('\n\r') + '.local';
      # HACK(kayvonf): skip first line in hostsfile since that is the
      # master node.  This must be kept consistent with the main pbs
      # script
      if count == 0:
        master_hostname = host;
      elif host != master_hostname:
        worker_hostnames.append(host);
      count = count+1;
    if args.verbose:
      print "Launching on remote hosts: ", worker_hostnames;
    hosts = set(worker_hostnames);

  else:
    if args.verbose:
      print "Launching on remote hosts: ", args.host
    hosts = set(args.host)
    
  hosts_lock = threading.Lock()

s = comm.listen_to(args.port)

if args.verbose:
    print "Listening on",  args.port

def reserve_remote_host():
  hosts_lock.acquire()
  if len(hosts) == 0:
    host = None
  else:
    host = hosts.pop()
  hosts_lock.release()
  return host

def release_remote_host(host):
  hosts_lock.acquire()
  hosts.add(host)
  hosts_lock.release()

def local_command(addr, worker_args):
    arg_string = " ".join(args.worker_args)
    
    if len(worker_args) == 0:
      workerparams_str = "";
    else:
      workerparams_str = "--workerparams=\"%s\"" % worker_args;  

    # XXX(awreece) Lol this is hilariously insecure.
    cmd = "./worker %s %s %s" % (arg_string, workerparams_str, addr)
    #print cmd;
    return cmd;
    
def launch_local_worker(addr, wr):
    command = local_command(addr, wr)
    if args.verbose:
        print "Launch worker: " + command

    proc = Popen(command, shell=True)
    ret = proc.wait()

    if args.verbose:
        print "Worker returned with status %d" % (ret)

loopback_patterns = map(re.compile, [
      r"^localhost",
      r"^127.0.0.1",
      r"^0:0:0:0:0:0:0:1",
      r"^::1",
])
def is_loopback_address(addr):
  # This is not intended to be exhaustive: this is merely an attempt to catch
  # silly mistakes early rather than leaving users lost and confused.
  return reduce(lambda a, r: a or r.match(addr), loopback_patterns, None) is not None

def launch_remote_worker(addr, wr):
    if is_loopback_address(addr):
       raise Exception("Using loopback addr " + addr + " for remote node")

    remote_host = reserve_remote_host()
    if remote_host is None:
	if args.verbose:
	    print "No remote available!"
        return

    # command = "ssh -o StrictHostKeyChecking=no %s '%s'" % (remote_host, local_command(addr, wr))
    local_cmd = local_command(addr, wr)
    # HACK(mchoquet): checks if running on latedays.andrew.cmu.edu, and runs a
    #                 different script if so.
    if (args.nodefile is not None):
      worker_cmd = "cd %s; source scripts/load_modules.sh; %s" % (remote_cwd, local_cmd)
    else:
      worker_cmd = "cd %s; %s" % (remote_cwd, local_cmd)
    command = "ssh %s '%s'" % (remote_host, worker_cmd)
    if args.verbose:
        print "Launch worker: " + command

    proc = Popen(command, shell=True)
    ret = proc.wait()

    release_remote_host(remote_host)

    if args.verbose:
        print "Worker returned with status %d" % ret

class WorkerHandler(threading.Thread):
    def __init__(self, launch_worker, addr, worker_args):
        threading.Thread.__init__(self)
        self.addr = addr
        self.worker_args = worker_args
        self.launch_worker = launch_worker

    def run(self):
        self.launch_worker(self.addr, self.worker_args)

class ConnectionHandler(threading.Thread):
    def __init__(self, sock, addr, launch_worker):
        threading.Thread.__init__(self)
        self.sock = sock
        self.launch_worker = launch_worker

    def run(self):
        addr = comm.recv_string(self.sock)
        if args.verbose:
            print "Master is listening on %s" % addr

        threads = []
        worker_args = comm.recv_string(self.sock)
        while worker_args:
	    t = WorkerHandler(self.launch_worker, addr, worker_args)
	    threads.append(t)
	    t.start()
	    try:
	      worker_args = comm.recv_string(self.sock)
	    except comm.SocketClosed:
	      if args.verbose:
	         print "Connection closed with %s" % addr
	      break;

	for t in threads:
	    t.join()

        if args.verbose:
            print "Connection from %s closed" % addr
        self.sock.close()

while True:
        conn, addr = s.accept()
        if args.verbose:
               print "Connection from", addr
	launch = launch_local_worker
	if launch_on_remote_hosts:
	      launch = launch_remote_worker
        ConnectionHandler(conn, addr, launch).start()

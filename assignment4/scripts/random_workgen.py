#!/usr/bin/env python2.7

import argparse
import comm
import os
import random
import socket
import string
import struct
import sys
import threading
import time

def positive_int(value):
  ivalue = int(value)
  if ivalue <= 0:
      raise argparse.ArgumentTypeError("%s is not a positive int value" % value)
  return ivalue

def positive_float(value):
  fvalue = float(value)
  if fvalue <= 0:
      raise argparse.ArgumentTypeError("%s is not a positive int value" % value)
  return fvalue

def hostport(value):
  host, sport = string.split(value, ":", 1)
  return (host, positive_int(sport))

parser = argparse.ArgumentParser(description="Launch workers")
parser.add_argument("--verbose",
    help="Verbose output",
    action='store_true')
parser.add_argument("address",
    help="Address of master",
    type=hostport)
parser.add_argument("--seed",
    help="Random seed for generator",
    type=positive_int)
parser.add_argument("--num_connections",
    help="Number of simultaneous connections to master",
    default=2,
    type=positive_int)
parser.add_argument("--sleep_time",
    help="Average time to pause between requests.",
    default=.2,
    type=positive_float)
parser.add_argument("--work_per_thread",
    help="Total units of work per thread to compute",
    default=10,
    type=positive_int)

args = parser.parse_args()

class WorkGenThread(threading.Thread):
    def __init__(self, tid, seed):
        threading.Thread.__init__(self)
        self.random = random.Random(seed)
	self.tid = tid
	if args.verbose:
	    self.log("Got seed %d" % seed)

    def log(self, msg):
        print "tid: %d %s" % (self.tid, msg)

    def fatal(self, msg=""):
        print "tid: %d FATAL %s" % (self.tid, msg)
	os._exit(1)

    def connect(self):
        s = socket.create_connection(args.address)
        if s is None:
	    self.fatal("Could not open socket!")

        if args.verbose:
	    self.log("Connected to %s,%d" % args.address)

	return s

    def send_work(self, socket, work):
	comm.TaggedMessage(comm.WORK, 0).to_socket(socket)
	work.to_socket(socket)

    def random_work(self):
	return comm.Work(comm.Work.HIGH_CPU, self.random.randrange(400))

    def random_sleep(self):
	sleep_time = self.random.expovariate(1 / args.sleep_time)
	if args.verbose:
	   self.log("Sleeping for %.3f" % sleep_time)
        time.sleep(sleep_time)

    def check_work(self, work, resp):
        if resp.x != 1492:
	    self.fatal("Invalid resp %s for work %s" % (resp, work))

    def run(self):
        try:
	    sock = self.connect()
	    for _ in xrange(args.work_per_thread):
		self.random_sleep()
		work = self.random_work()
		if args.verbose:
		    self.log("Sending %s" % work)
		self.send_work(sock, work)
		resp = comm.Resp.from_socket(sock)
		self.check_work(work, resp)
		if args.verbose:
		    self.log("Got resp %s for work %s" % (resp, work))
	    sock.close()
	except Exception as e:
	    self.fatal(e)

if args.seed:
    random.seed(args.seed)

    if args.verbose:
        print "Seeding with ", args.seed

threads = [WorkGenThread(i, random.randrange(0,25)) for i in xrange(args.num_connections)]
[t.start() for t in threads]
[t.join() for t in threads]

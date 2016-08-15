// Copyright 2013 Course Staff.

#include <boost/make_shared.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <fstream>
#include <map>

#include "server/messages.h"
#include "server/worker.h"
#include "tools/cycle_timer.h"

/*
 * high_compute_job --
 *
 * This function performs a compute-intensive operation (generating a
 * large number of random numbers).  There is essentially no memory
 * traffic.  The working set is very, very small.
 */
void high_compute_job(const Request_msg& req, Response_msg& resp) {

  const char* motivation[16] = {
    "You are going to do a great project",
    "OMG, 418 is so gr8!",
    "Come to lecture, there might be donuts!",
    "Write a great lecture comment on your favorite idea in the class",
    "Bring out all the stops in assignment 4.",
    "Ask questions. Ask questions. Ask questions",
    "Flatter your TAs with compliments",
    "Worse is better. Keep it simple...",
    "You will perform amazingly on exam 2",
    "You will PWN your classmates in the parallelism competition",
    "Exams are all just fun and games",
    "Do as best as you can and just have fun!",
    "Laugh at Kayvon's jokes",
    "Do a great project, and it all works out in the end",
    "Be careful not to optimize prematurely",
    "If all else fails... buy Kayvon donuts",
  };

  int iters = 175 * 1000 * 1000;
  unsigned int seed = atoi(req.get_arg("x").c_str());

  for (int i=0; i<iters; i++) {
    seed = rand_r(&seed);
  }

  int idx = seed % 16;
  resp.set_response(motivation[idx]);
}

/*
 * count_primes_job --
 * 
 * This task has similar workload characteristics as high_compute_job.
 * It is compute intensive, with a tiny working set.  It computes the
 * number of primes up to the input argument N. (We are aware it is
 * not a particularlly intelligent algorithm for doing this.)
 */
void count_primes_job(const Request_msg& req, Response_msg& resp) {

  int N = atoi(req.get_arg("n").c_str());

  int NUM_ITER = 10;
  int count;

  for (int iter = 0; iter < NUM_ITER; iter++) {
    count = (N >= 2) ? 1 : 0; // since 2 is prime

    for (int i = 3; i < N; i+=2) {    // For every odd number

      int prime;
      int div1, div2, rem;

      prime = i;

      // Keep searching for divisor until rem == 0 (i.e. non prime),
      // or we've reached the sqrt of prime (when div1 > div2)

      div1 = 1;
      do {
        div1 += 2;            // Divide by 3, 5, 7, ...
        div2 = prime / div1;  // Find the dividend
        rem = prime % div1;   // Find remainder
      } while (rem != 0 && div1 <= div2);

      if (rem != 0 || div1 == prime) {
        // prime is really a prime
        count++;
      }
    }
  }

  char tmp_buffer[32];
  sprintf(tmp_buffer, "%d", count);
  resp.set_response(tmp_buffer);
}

/*
 * mini_compute_job --
 *
 * This task is a tiny operation.  It has very low compute or
 * bandwidth requirements since all it does is square the input number
 * and add 10.
 */
void mini_compute_job(const Request_msg& req, Response_msg& resp) {

  int number = atoi(req.get_arg("x").c_str());

  // result = x * x + 10
  int result = number * number + 10;
  int idx = result % 10;

const char* responses[10] = {
    "We recommend getting full credit on grading_wisdom.txt first",
    "Re-watch the lecture on scaling a website",
    "There are two of these: http://ark.intel.com/products/83352/Intel-Xeon-Processor-E5-2620-v3-15M-Cache-2_40-GHz",
    "You need good perf out of a node AND the ability to scale-out",
    "Yes, you can optimize for specific traces but you don't have to. A general schedule algorithm works.",
    "Figure out a way to understand the workload characteristics in each trace.",
    "There may be opportunities for caching in this assignment",
    "Are there any other opportunities for parallelism? (other than parallelism across requests?)",
    "The costs of communication between server nodes is likely not significant in this assignment.",
    "The best performance may come from a particular mixture of jobs on a worker node."
  };
  
  resp.set_response(responses[idx]);
}

/*
 * high_bandwidth_job --
 *
 * This function streams over a large chunk of memory.  Therefore it
 * is a bandwidth-intensive task.
 */
void high_bandwidth_job(const Request_msg& req, Response_msg& resp) {

  const int NUM_ITERS = 100;
  const int ALLOCATION_SIZE = 64 * 1000 * 1000;
  const int NUM_ELEMENTS = ALLOCATION_SIZE / sizeof(unsigned int);
  
  // Allocate a buffer that's much larger than the LLC and populate
  // it.
  unsigned int* buffer = new unsigned int[NUM_ELEMENTS];
  if (!buffer) {
    // worth checking for
    resp.set_response("allocation failed: worker likely out of memory");
    return;
  }
  
  for (int i=0; i<NUM_ELEMENTS; i++) {
    buffer[i] = (unsigned int)i;
  }
  
  int index = atoi(req.get_arg("x").c_str()) % NUM_ELEMENTS;
  unsigned int total = 0;
  
  //double startTime = CycleTimer::currentSeconds();

  // loop over the buffer, jumping by a cache line each time.  Simple
  // stride means the prefetcher will probably do reasonably well but
  // we'll be terribly bandwidth bound.
  for (int iter=0; iter<NUM_ITERS; iter++) {
    for (int i=0; i<NUM_ELEMENTS; i++) {
      total += buffer[index]; 
      index += 16;
      if (index >= NUM_ELEMENTS)
	index = 0;
    }
  }
  
  //double endTime = CycleTimer::currentSeconds();

  delete [] buffer;
  
  //double postFreeTime = CycleTimer::currentSeconds();

  //  DLOG(INFO) << req.get_request_string()
  //	     << " scan=" << (endTime - startTime)
  //	     << " free=" << (postFreeTime - endTime) << std::endl;

  char tmp_buffer[128];
  sprintf(tmp_buffer, "%u", total);
  resp.set_response(tmp_buffer);
}

/*
 * cachefootprint_job --
 *
 * This function has a working that just fits within the L3 cache on
 * the CPUs in latedays nodes.  It operates by allocating a buffer of
 * pointers, and then randomly jumping to different elements of the
 * buffer. The performance and bandwidth requirements of this
 * operation are sensitive to this working set staying in the cache.
 * If the working set is in cache, there will be essentially no
 * bandwidth requirement.  If it falls out of cache, the performance
 * of the code will drop substantially.
 */
void cachefootprint_job(const Request_msg& req, Response_msg& resp) {

  // hardcode buffer size to about 14 MB (the LLC on latedays CPUs is
  // 15MB)
  unsigned int L3_SIZE = 14 * 1000 * 1000; 
  unsigned int n = L3_SIZE / sizeof(void *);

  // Make a random permutation of [0 ... n-1].
  unsigned int seed = atoi(req.get_arg("x").c_str());
  unsigned int *scratch = new unsigned int[n];
  for (unsigned int i = 0; i < n; i++) {
    scratch[i] = i;
  }
  for (unsigned int i = n - 1; i > 0; i--) {
    unsigned int j = seed % (i + 1);
    seed = rand_r(&seed);
    unsigned int tmp = scratch[i];
    scratch[i] = scratch[j];
    scratch[j] = tmp;
  }

  // Turn the permutation into a cycle of pointers
  void **arr = new void *[n];
  for (unsigned int i = 0; i < n - 1; i++) {
    arr[scratch[i]] = (void *)&arr[scratch[i + 1]];
  }
  arr[scratch[n - 1]] = (void *)&arr[scratch[0]];
  void **p = &arr[scratch[0]];
  delete scratch;

  //double startTime = CycleTimer::currentSeconds();

  ////////////////////////////////////////////////////////////////
  // Loop through the pointer cycle a few times. Each iteration jump
  // to the location in the buffer indicated by the current
  // element. (This is where all the work in this function is)
  ////////////////////////////////////////////////////////////////

  unsigned int nIterations = 100;
  for (unsigned int i = 0; i < nIterations * n; i++) {
    p = (void **)(*p);
  }
  
  //double endTime = CycleTimer::currentSeconds();

  //DLOG(INFO) << req.get_request_string()
  //	     << " scan=" << (endTime - startTime) << std::endl;

  unsigned int i = p - arr;
  delete arr;

  // now emit a response

  int idx = i % 14;
  
  const char* responses[14] = {
    "Implement a cache simulator that supports invalidation-based coherence.",
    "Parallelize an algorithm you are working on for research.",
    "Try and beat one of the solutions in Guy Blelloch's Problem-Based Benchmark Suite.",
    "Play around with interesting hardware, like FPGAs, Raspberry PIs, Tegra K1, or Oculus Rift",
    "Measure the energy consumption of a device when running an interesting workload",
    "Use a modern parallel programming framework that we didn't teach in class.",
    "Consider large-scale graph algorithms",
    "There are also great projects on parallelizing graphics, comptuer vision, or machine learning.",
    "Implement a parallelizing compiler.",
    "Investigate scale-out parallelism using Amazon web services",
    "Cryptocurrencies!",
    "Parallelize an algorithm you are interested in on Latedays.",
    "Computer vision is ripe for optimization these days",
    "Check out last year's parallelism computation page for more ideas"
  };

  resp.set_response(responses[idx]);
}

/*
 * execute_work --
 *
 * This function generates responses for all the Assignment 4 request
 * types.  You are not allowed to modify this function or this files
 * contents, but you absolutely want to understand the workload
 * characteristics of each type of request.
 */ 
void execute_work(const Request_msg& req, Response_msg& resp) {

  std::string cmd = req.get_arg("cmd");

  if (cmd.compare("418wisdom") == 0) {
    // compute intensive
    high_compute_job(req, resp);
  }
  else if (cmd.compare("countprimes") == 0) {
    // compute intensive
    count_primes_job(req, resp);
  }
  else if (cmd.compare("bandwidth") == 0) {
    // bandwidth intensive
    high_bandwidth_job(req, resp);
  }
  else if (cmd.compare("tellmenow") == 0) {
    // very little compute or bandwidth (lightweight job)
    mini_compute_job(req, resp);
  }
  else if (cmd.compare("projectidea") == 0) {
    // has an L3-cache sized working set
    cachefootprint_job(req, resp);
  }
  else {
    resp.set_response("unknown command");
  }
}


void init_work_engine() {
  // no initialize required at this time
}

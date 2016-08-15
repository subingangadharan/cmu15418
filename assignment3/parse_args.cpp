
#include "parse_args.h"

#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include <argp.h>
#include <math.h>
#include <string>

#include "graph.h"


/* Argument parsing */
const char* argp_program_version = "asst3 1.0";
const char *argp_program_bug_address = "<15418-staff@cs.cmu.edu>";

/* Program documentation. */
static char doc[] = "15-418: Assignment 3\n"
  "APP is one of bfs, pagerank, kbfs, decomp, or grade.";

/* A description of the arguments we accept. */
static char args_doc[] = "APP GRAPH_FILE";
 
/* The options we understand. */
static struct argp_option options[] = {
  {"device",  'd',    "N",      0,  "Device number to run the code on" },
  {"threads", 't',    "N",      0,  "Number of threads to use" },
  {"check",   'c',      0,      1,  "Check for correctness (run once)" },
  {"stu",     's',      0,      1,  "Skip student solution" },
  {"ref",     'r',      0,      1,  "Skip reference solution" },
  { 0 }
};

/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  Arguments* arguments = (Arguments*)(state->input);
  int threads;

  switch (key) {
    case 'd':
      arguments->device = atoi(arg);
      break;
    case 't':
      threads = atoi(arg);
      if (threads <= 0) {
      	std::cerr << "Invalid thread number: " << threads << std::endl;
        argp_usage(state);
      }
      arguments->threads = threads;
      break;
    case 'c':
      arguments->correctness = true;
      break;
    case 's':
      arguments->runStu = false;
      break;
    case 'r':
      arguments->runRef = false;
      break;

    case ARGP_KEY_ARG:
      if (state->arg_num == 0) {
        std::string app(arg);
        if (app == "bfs")
          arguments->app = BFS;
        else if (app == "pagerank")
          arguments->app = PAGERANK;
        else if (app == "kbfs")
          arguments->app = KBFS;
        else if (app == "decomp")
          arguments->app = DECOMP;
        else if (app == "grade")
          arguments->app = GRADE;
        else {
          /* Invalid app */
          std::cerr << "Invalid app " << arg << std::endl;
          argp_usage(state);
        }
      }
      else if (state->arg_num == 1) {
        arguments->graph = arg;
      } else {
        /* Too many arguments. */
        argp_usage(state);
      }

      break;

    case ARGP_KEY_END:
      if (state->arg_num < 2) {
        /* Not enough arguments. */
        argp_usage(state);
      }
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

Arguments parseArgs(int argc, char** argv)
{
  Arguments arguments;
  arguments.app = GRADE;
  arguments.graph = NULL;
  arguments.device = 0;
  arguments.threads = -1;
  arguments.correctness = false;
  // By default run both reference and student solutions.
  arguments.runStu = true;
  arguments.runRef = true;

  argp_parse(&argp, argc, argv, 0, 0, &arguments);
  return arguments;
}


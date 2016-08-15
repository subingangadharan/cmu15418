#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <type_traits>
#include <utility>

#include <float.h>

#include <math.h>
#include <omp.h>
#include "mic.h"

#include "graph.h"
#include "grade.h"

#include "parse_args.h"

/* Apps */
#include "apps/bfs.h"
#include "apps/page_rank.h"
#include "apps/kBFS.h"
#include "apps/graph_decomposition.h"

/* Reference Solutions */
#include "ref/apps/bfs_ref.h"
#include "ref/apps/page_rank_ref.h"
#include "ref/apps/graph_decomposition_ref.h"
#include "ref/apps/kBFS_ref.h"

/* App constants */
#define DecompBeta 2.f
#define PageRankDampening 0.3f
#define PageRankConvergence 0.01f

// Number of trials to run benchmarks
#define NUM_TRIALS 1

/*
 * App wrappers
 */
void pageRankWrapper(Graph g, float* solution)
{
  pageRank(g, solution, PageRankDampening, PageRankConvergence);
}

void pageRankRefWrapper (Graph g, float* solution)
{
  pageRank_ref(g, solution, PageRankDampening, PageRankConvergence);
}

void graphDecompWrapper(Graph g, int* solution) 
{
  int maxVal;
  int maxId;
  int* dus = getDus(g->num_nodes, DecompBeta, &maxVal, &maxId);

  decompose(g, solution, dus, maxVal, maxId);
  free(dus);
}

// returns for every node, the cluster id it belongs to 
void graphDecompRefWrapper(Graph g, int* solution) 
{
  int maxVal;
  int maxId;
  int* dus = getDus_ref(g->num_nodes, DecompBeta, &maxVal, &maxId);

  decompose_ref(g, solution, dus, maxVal, maxId);
  free(dus);
}

/*
 * Timing apps
 */

void gradeApps(int num_nodes, int num_edges,
    int *outgoing_starts, int *outgoing_edges,
    int *incoming_starts, int *incoming_edges,
    int device, int numTrials, int minThreadCount, int maxThreadCount,
    App app, bool runRef, bool runStu) {
  graph g;

  g.num_nodes = num_nodes;
  g.num_edges = num_edges;
  g.outgoing_starts = outgoing_starts;
  g.outgoing_edges = outgoing_edges;
  g.incoming_starts = incoming_starts;
  g.incoming_edges = incoming_edges;

  // Time apps
  std::stringstream timing;

  /* Run tests */
  float possiblePoints = 0;
  float points = 0;
  if (app == BFS || app == GRADE) {
    printTimingApp(timing, "BFS");
    possiblePoints += 4.5;
    points += timeApp<int, BFS>(&g, device, numTrials, 4.5,
        minThreadCount, maxThreadCount, bfs_ref, bfs,
        compareArrays<int>, runRef, runStu, timing);
  }
  if (app == PAGERANK || app == GRADE) {
    printTimingApp(timing, "PageRank");
    possiblePoints += 4.5;
    points += timeApp<float, PAGERANK>(&g, device, numTrials, 4.5,
        minThreadCount, maxThreadCount,
        pageRankRefWrapper, pageRankWrapper,
        compareApprox<float>, runRef, runStu, timing);
  }
  if (app == KBFS || app == GRADE) {
    printTimingApp(timing, "kBFS");
    possiblePoints += 4.5;
    points += timeApp<int, KBFS>(&g, device, numTrials, 4.5,
        minThreadCount, maxThreadCount, kBFS_ref, kBFS,
        compareArraysAndRadiiEst<int>, runRef, runStu, timing);
  }
  if (app == DECOMP || app == GRADE) {
    printTimingApp(timing, "Graph Decomposition");
    // 5 total performance points for graph decomp, split between 4 graphs.
    possiblePoints += 5.0/4;
    points += timeApp<int, DECOMP>(&g, device, numTrials, 5.0/4,
        minThreadCount, maxThreadCount,
        graphDecompRefWrapper, graphDecompWrapper,
        compareArrays<int>, runRef, runStu, timing);
  }

  /* Timing done */
  std::cout << std::endl << std::endl;
  std::cout << "Grading summary:" << std::endl;
  std::cout << timing.str();
  std::cout << std::endl;
  std::cout << "Total Grade: " << points << "/" << possiblePoints << std::endl;
  std::cout << std::endl;
}

/*
 * main
 */

int main(int argc, char** argv)
{
  Arguments arguments = parseArgs(argc, argv);

  sep(std::cout);
  std::cout << "Running on device " << arguments.device << std::endl;
  sep(std::cout);

  int sys_max_threads, max_threads, min_threads;

  // Get the number of available threads on the MIC or host device
  #pragma offload target(mic: arguments.device)
  sys_max_threads = omp_get_max_threads();

  if (arguments.threads > 0) {
    max_threads = std::min(arguments.threads, sys_max_threads);
    min_threads = max_threads;
  } else if (arguments.app == GRADE) {
    // Grading starts from 64 threads
    min_threads = std::min(64, sys_max_threads);
    max_threads = sys_max_threads;
  } else {
    min_threads = 1;
    max_threads = sys_max_threads;
  }

  // Test correctness only.
  int numTrials = NUM_TRIALS;
  if (arguments.correctness) {
    numTrials = 1;
  }

  std::cout << std::endl;
  sep(std::cout);
  std::cout << "Max system threads = " << max_threads << std::endl;
  std::cout << "Running " << min_threads <<  "-" << max_threads << " threads";
  std::cout << std::endl;
  sep(std::cout);

  std::cout << std::endl;
  std::cout << "Loading graph..." << std::endl;
  Graph graph = load_graph_binary(arguments.graph);

  std::cout << std::endl;
  std::cout << "Graph stats:" << std::endl;
  std::cout << "  Nodes: " << num_nodes(graph) << std::endl;
  std::cout << "  Edges: " << num_edges(graph) << std::endl;

  std::cout << std::endl;

  /* Transfer graph data to device */
  int num_nodes = graph->num_nodes;
  int num_edges = graph->num_edges;

  int* outgoing_starts = graph->outgoing_starts;
  int* outgoing_edges = graph->outgoing_edges;

  int* incoming_starts = graph->incoming_starts;
  int* incoming_edges = graph->incoming_edges;

  #pragma offload_transfer target(mic: arguments.device) \
        in(outgoing_starts : length(graph->num_nodes) ALLOC) \
        in(outgoing_edges : length(graph->num_edges) ALLOC)  \
        in(incoming_starts : length(graph->num_nodes) ALLOC) \
        in(incoming_edges : length(graph->num_edges) ALLOC)

  /* Time apps */
  #pragma offload target(mic: arguments.device) \
    in(numTrials) \
    in(min_threads) \
    in(max_threads) \
    in(num_edges) \
    in(num_nodes) \
    in(arguments.app) \
    in(arguments.device) \
    in(arguments.runRef) \
    in(arguments.runStu) \
    nocopy(outgoing_starts : length(graph->num_nodes) REUSE) \
    nocopy(outgoing_edges : length(graph->num_edges) REUSE)  \
    nocopy(incoming_starts : length(graph->num_nodes) REUSE) \
    nocopy(incoming_edges : length(graph->num_edges) REUSE)
  gradeApps(num_nodes, num_edges, outgoing_starts, outgoing_edges,
      incoming_starts, incoming_edges,
      arguments.device, numTrials, min_threads, max_threads, arguments.app,
      arguments.runRef, arguments.runStu);

  /* Free graph data */
  #pragma offload_transfer target(mic: arguments.device) \
        nocopy(outgoing_starts : length(graph->num_nodes) FREE) \
        nocopy(outgoing_edges : length(graph->num_edges) FREE)  \
        nocopy(incoming_starts : length(graph->num_nodes) FREE) \
        nocopy(incoming_edges : length(graph->num_edges) FREE)

  free_graph(graph);
}

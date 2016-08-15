#ifndef __GRADE_H__
#define __GRADE_H__

#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <chrono>

#include <type_traits>
#include <utility>

#include <float.h>

#include <omp.h>
#include "mic.h"

#include "graph.h"
#include "graph_internal.h"

#include "parse_args.h"
#include "contracts.h"

// Epsilon for approximate float comparisons
#define EPSILON 0.001f

// Output column size
#define COL_SIZE 15

// Number of points for each runtime matching the original ref runtime.
#define EXTRA_CREDIT 0.3

/*
 * Reference times on the phi
 * For grading student performance when the reference is not run.
 * Indexed by [APP][graph][number of threads (64, 128, 236)]
 */
// TODO(kku): Ugly hard-coded size

// Adjusted reference time
static double refTimeTable[4][4][3] = {
  // BFS
  {
    // com-orkut_117m.graph
    { 0.2235 , 0.1637 , 0.2415 },
    // soc-livejournal1_68m
    { 0.3734 , 0.3135 , 0.4057 },
    // rmat_200m
    { 2.1021 , 1.6171 , 1.5956 },
    // soc-pokec_30m
    { 0.2081 , 0.1939 , 0.2924 }
  },
  // PAGERANK
  {
    // com-orkut_117m.graph
    { 3.8533 , 2.2470 , 1.5512 },
    // soc-livejournal1_68m
    { 2.2999 , 1.3957 , 1.0575 },
    // rmat_200m
    { 13.5453 , 8.3121 , 6.1407 },
    // soc-pokec_30m
    { 1.2403 , 0.7041 , 0.4984 }
  },
  // KBFS
  {
    // com-orkut_117m.graph
    { 41.7699 , 26.3132 , 19.4586 },
    // soc-livejournal1_68m
    { 10.3742 , 8.3289 , 8.0807 },
    // rmat_200m
    { 67.6248 , 52.2105 , 47.1388 },
    // soc-pokec_30m
    { 4.5007 , 3.5232 , 3.2306 }
  },
  // DECOMP
  {
    // com-orkut_117m.graph
    { 3.0258 , 2.4040 , 2.1483 },
    // soc-livejournal1_68m
    { 3.0444 , 2.7236 , 2.6725 },
    // rmat_200m
    { 21.8837 , 18.8862 , 17.9954 },
    // soc-pokec_30m
    { 1.2287 , 1.0783 , 1.0672 }
  }
};

// Original reference time used for extra credit.
static double refExtraTimeTable[4][4][3] = {
  // BFS
  {
    // com-orkut_117m.graph
    {0.2078, 0.1357, 0.2209}, // adjusted
    // soc-livejournal1_68m
    {0.2347, 0.2123, 0.2704},
    // rmat_200m
    {1.5444, 1.0811, 1.0658},
    // soc-pokec_30m
    {0.1962, 0.1760, 0.2879} // adjusted
  },
  // PAGERANK
  {
    // com-orkut_117m.graph
    {3.5203, 2.0160, 1.3421}, // adjusted
    // soc-livejournal1_68m
    {1.9369, 1.1051, 0.7757},
    // rmat_200m
    {11.3733, 6.3221, 4.3275},
    // soc-pokec_30m
    {1.1019, 0.6059, 0.4016} // adjusted
  },
  // KBFS
  {
    // com-orkut_117m.graph
    {39.9503, 25.5576, 18.8067}, // adjusted
    // soc-livejournal1_68m
    {9.8073, 7.9282, 7.6994}, // adjusted
    // rmat_200m
    {66.0763, 49.6047, 44.6521},
    // soc-pokec_30m
    {4.3053, 3.4722, 3.1474}
  },
  // DECOMP
  {
    // com-orkut_117m.graph
    {2.9623, 2.3680, 2.1731},
    // soc-livejournal1_68m
    {2.9650, 2.6828, 2.6310},
    // rmat_200m
    {20.4276, 18.0206, 17.2282},
    // soc-pokec_30m
    {1.1637, 1.0159, 1.0074}
  }
};

// Get index into refTimeTable for a given number of threads.
static int getThreadIndex(int numThreads) {
  if (numThreads == 64)
    return 0;
  else if (numThreads == 128)
    return 1;
  return 2;
}

// Get index into refTimeTable for a given graph.
static int getGraphIndex(Graph g) {
  // com-orkut_117m.graph
  if (num_nodes(g) == 3072441)
    return 0;
  // soc-livejournal1_68m
  if (num_nodes(g) == 4847571)
    return 1;
  // rmat_200m
  if (num_nodes(g) == 33554432)
    return 2;
  // soc-pokec_30m
  if (num_nodes(g) == 1632803)
    return 3;
  
  // Should not get here.
  ENSURES(false);
  return -1;
}

/*
 * Printing functions
 */

static void sep(std::ostream& out, char separator = '-', int length = 78)
{
    for (int i = 0; i < length; i++)
      out << separator;
    out << std::endl;
}

static void printTimingApp(std::ostream& timing, const char* appName)
{
  std::cout << std::endl;
  std::cout << "Timing results for " << appName << ":" << std::endl;
  sep(std::cout, '=', 75);

  timing << std::endl;
  timing << "Timing results for " << appName << ":" << std::endl;
  sep(timing, '=', 75);
}

/*
 * Correctness checkers
 */

template <class T>
bool compareArrays(Graph graph, T* ref, T* stu)
{
  for (int i = 0; i < graph->num_nodes; i++) {
    if (ref[i] != stu[i]) {
      std::cerr << "*** Results disagree at " << i << " expected " 
        << ref[i] << " found " << stu[i] << std::endl;
      return false;
    }
  }
  return true;
}

template <class T>
bool compareApprox(Graph graph, T* ref, T* stu)
{
  for (int i = 0; i < graph->num_nodes; i++) {
    if (fabs(ref[i] - stu[i]) > EPSILON) {
      std::cerr << "*** Results disagree at " << i << " expected " 
        << ref[i] << " found " << stu[i] << std::endl;
      return false;
    }
  }
  return true;
}

template <class T>
bool compareArraysAndDisplay(Graph graph, T* ref, T*stu) 
{
  printf("\n----------------------------------\n");
  printf("Visualization of student results");
  printf("\n----------------------------------\n\n");

  int grid_dim = (int)sqrt(graph->num_nodes);
  for (int j=0; j<grid_dim; j++) {
    for (int i=0; i<grid_dim; i++) {
      printf("%02d ", stu[j*grid_dim + i]);
    }
    printf("\n");
  }
  printf("\n----------------------------------\n");
  printf("Visualization of reference results");
  printf("\n----------------------------------\n\n");

  grid_dim = (int)sqrt(graph->num_nodes);
  for (int j=0; j<grid_dim; j++) {
    for (int i=0; i<grid_dim; i++) {
      printf("%02d ", ref[j*grid_dim + i]);
    }
    printf("\n");
  }
  
  return compareArrays<T>(graph, ref, stu);
}

template <class T>
bool compareArraysAndRadiiEst(Graph graph, T* ref, T* stu) 
{
  bool isCorrect = true;
  for (int i = 0; i < graph->num_nodes; i++) {
    if (ref[i] != stu[i]) {
      std::cerr << "*** Results disagree at " << i << " expected "
        << ref[i] << " found " << stu[i] << std::endl;
	isCorrect = false;
    }
  }
  int stuMaxVal = -1;
  int refMaxVal = -1;
  #pragma omp parallel for schedule(dynamic, 512) reduction(max: stuMaxVal)
  for (int i = 0; i < graph->num_nodes; i++) {
	if (stu[i] > stuMaxVal)
		stuMaxVal = stu[i];
  }
  #pragma omp parallel for schedule(dynamic, 512) reduction(max: refMaxVal)
  for (int i = 0; i < graph->num_nodes; i++) {
        if (ref[i] > refMaxVal)
                refMaxVal = ref[i];
  }
 
  if (refMaxVal != stuMaxVal) {
	std::cerr << "*** Radius estimates differ. Expected: " << refMaxVal << " Got: " << stuMaxVal << std::endl;
	isCorrect = false;
  }   
  return isCorrect;
}

/*
 * Time and score an app
 */

// Returns score for the app.
template<typename T, App APP>
double timeApp(Graph g, int device, int numTrials, double maxPoints,
    int minThreadCount, int maxThreadCount,
    void (*ref)(Graph, T*), void (*stu)(Graph, T*),
    bool (*check)(Graph, T*, T*),
    bool runRef, bool runStu,
    std::ostream& timing) {

  timing << std::left << std::setw(COL_SIZE) << "Threads";
  timing << std::left << std::setw(COL_SIZE) << "Ref. Time";
  timing << std::left << std::setw(COL_SIZE) << "Ref. Speedup";
  if (runStu) {
    timing << std::left << std::setw(COL_SIZE) << "Your Time";
    timing << std::left << std::setw(COL_SIZE) << "Your Speedup";
  }
  timing << std::endl;
  sep(timing, '-', 75);

  std::cout << std::left << std::setw(COL_SIZE) << "Threads";
  std::cout << std::left << std::setw(COL_SIZE) << "Ref. Time";
  std::cout << std::left << std::setw(COL_SIZE) << "Ref. Speedup";
  if (runStu) {
    std::cout << std::left << std::setw(COL_SIZE) << "Your Time";
    std::cout << std::left << std::setw(COL_SIZE) << "Your Speedup";
  }
  std::cout << std::endl;
  sep(std::cout, '-', 75);

  using namespace std::chrono;
  typedef std::chrono::high_resolution_clock Clock;
  typedef std::chrono::duration<double> dsec;

  int precision = 4;
  T* refSolution = new T [g->num_nodes];
  T* stuSolution = new T [g->num_nodes];

  bool firstTimeDone = false;
  double refOneThreadTime = 0;
  double stuOneThreadTime = 0;

  bool correct = true;
  double refBestTime = DBL_MAX;
  double stuBestTime = DBL_MAX;

  int threads = minThreadCount;
  while (true) {
    double refTime = 0;
    double stuTime = 0;
    for (int i = 0; i < numTrials; i++) {
      #pragma offload target(mic: device)
      omp_set_num_threads(threads);
      
      if (runRef) {
        auto refStart = Clock::now();
        ref(g, refSolution);
        refTime += duration_cast<dsec>(Clock::now() - refStart).count();
      } else {
        refTime += refTimeTable[APP][getGraphIndex(g)][getThreadIndex(threads)];
      }

      if (runStu) {
        auto stuStart = Clock::now();
        stu(g, stuSolution);
        stuTime += duration_cast<dsec>(Clock::now() - stuStart).count();
      } else {
        stuTime += 1;
      }

      if (runRef && runStu) {
        correct = correct && check(g, refSolution, stuSolution);
        if (!correct)
          break;
      }
    }

    refTime /= numTrials;
    stuTime /= numTrials;

    if (!firstTimeDone) {
      firstTimeDone = true;
      refOneThreadTime = refTime;
      stuOneThreadTime = stuTime;
    }

    refBestTime = std::min(refBestTime, refTime);
    stuBestTime = std::min(stuBestTime, stuTime);

    double refSpeedup = refOneThreadTime / refTime;
    double stuSpeedup = stuOneThreadTime / stuTime;

    timing << std::right << std::setw(7) << threads;
    timing << std::left << std::setw(COL_SIZE - 7) << "";
    timing << std::setprecision(precision) << std::fixed;
    timing << std::left << std::setw(COL_SIZE) << refTime;
    timing << std::setprecision(precision) << std::fixed;
    timing << std::left << std::setw(COL_SIZE) << refSpeedup;
    if (runStu) {
      timing << std::setprecision(precision) << std::fixed;
      timing << std::left << std::setw(COL_SIZE) << stuTime;
      timing << std::setprecision(precision) << std::fixed;
      timing << std::left << std::setw(COL_SIZE) << stuSpeedup;
    }
    timing << std::endl;

    std::cout << std::right << std::setw(7) << threads;
    std::cout << std::left << std::setw(COL_SIZE - 7) << "";
    std::cout << std::setprecision(precision) << std::fixed;
    std::cout << std::left << std::setw(COL_SIZE) << refTime;
    std::cout << std::setprecision(precision) << std::fixed;
    std::cout << std::left << std::setw(COL_SIZE) << refSpeedup;
    if (runStu) {
      std::cout << std::setprecision(precision) << std::fixed;
      std::cout << std::left << std::setw(COL_SIZE) << stuTime;
      std::cout << std::setprecision(precision) << std::fixed;
      std::cout << std::left << std::setw(COL_SIZE) << stuSpeedup;
    }
    std::cout << std::endl;

    if (threads == maxThreadCount)
      break;
    
    threads = std::min(maxThreadCount, threads * 2);
  }

  delete[] refSolution;
  delete[] stuSolution;

  /* Print and return grade */
  double fraction = stuBestTime / refBestTime * 100.0;
  double curve = 4.0 / 3.0 * (refBestTime / stuBestTime) - (1.0 / 3.0);
  double points = std::min(maxPoints, std::max(maxPoints * curve, 0.0));
  points = (correct) ? points : 0.0;

  // Calculate extra credit
  double refExtraBestTime = DBL_MAX;
  if (!runRef) {
    for (int i = minThreadCount; i <= maxThreadCount;
        i = std::min(maxThreadCount, i * 2)) {
      refExtraBestTime = std::min(refExtraBestTime,
          refExtraTimeTable[APP][getGraphIndex(g)][getThreadIndex(i)]);
      if (i == maxThreadCount)
        break;
    }
  } else {
    refExtraBestTime = refBestTime;
  }
  if (correct && stuBestTime <= refExtraBestTime)
    points += EXTRA_CREDIT;

  sep(timing, '-', 75);
  timing << "Time: " << std::setprecision(2) << std::fixed << fraction << '%' << " of reference solution.";
  if (correct) {
    timing << " Grade: " << std::fixed << std::setprecision(2) << points << std::endl;
    if (stuBestTime <= refExtraBestTime)
      timing << "EXTRA CREDIT" << std::endl;
  } else
  {
    timing << " Grade: " << std::fixed << std::setprecision(2) << "INCORRECT" << std::endl;
  }
  sep(timing, '-', 75);

  sep(std::cout, '-', 75);
  std::cout << "Time: " << std::setprecision(2) << std::fixed << fraction << '%' << " of reference solution.";
  if (correct) {
    std::cout << " Grade: " << std::fixed << std::setprecision(2) << points << std::endl;
    if (stuBestTime <= refExtraBestTime)
      std::cout << "EXTRA CREDIT" << std::endl;
  } else
  {
    std::cout << " Grade: " << std::fixed << std::setprecision(2) << "INCORRECT" << std::endl;
  }
  sep(std::cout, '-', 75);

#ifndef RUN_MIC
  if (!runRef) {
    timing << "WARNING: Reference time is based on MIC execution." << std::endl;
    std::cout << "WARNING: Reference time is based on MIC execution." << std::endl;
  }
#endif

  return points;
}

#endif /* __GRADE_H__ */

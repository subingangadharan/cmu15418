#ifndef __GRAPH_DECOMPOSITION_H__
#define __GRAPH_DECOMPOSITION_H__

#include "graph.h"
#include <random>
#include <math.h>
#include <mutex>
#include <omp.h>

static float* genExp(int n, float rate, float* maxVal, int* maxId) {
  std::default_random_engine generator; // note this will always generate the same values - which we want - for grading
  std::exponential_distribution<double> distribution(rate);
  float maxdu = -1.f;
  int id = -1;

  std::mutex mtx;

  float* vals = (float*) malloc(sizeof(float) * n);
  for (int i = 0; i < n; i++) {
    float val = distribution(generator);
    if (val > maxdu) {
      mtx.lock();
      if (val > maxdu) {
        maxdu = val;
        id = i;
      }
      mtx.unlock();
    }

    vals[i] = val;
  }

  *maxVal = maxdu;
  *maxId = id;
  return vals;
}


/**
 * Given an array of floats, casts them all into 
 **/
static int* chopToInt(float* fdus, int n) {
  int* dus = (int*)malloc(sizeof(int) * n);
  #pragma omp parallel for schedule(dynamic, 512)
  for (int i = 0; i < n; i++) {
    dus[i] = (int)fdus[i];
  }
  return dus;
}

static int* getDus(int n, float rate, int* maxVal, int* maxId) {
  float fmaxVal;
  float* expVals = genExp(n, rate, &fmaxVal, maxId);
  int* dus = chopToInt(expVals, n);
  free(expVals);
	*maxVal = (int)fmaxVal;
  return dus;
}

void decompose(graph *g, int *decomp, int* dus, int maxVal, int maxId);

#endif

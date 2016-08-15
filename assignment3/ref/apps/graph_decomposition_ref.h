#ifndef __GRAPH_DECOMPOSITION_REF_H__
#define __GRAPH_DECOMPOSITION_REF_H__

#include "graph.h"
#include <mutex>
#include <random>
#include <omp.h>

static float* genExp_ref(int n, float rate, float* maxVal, int* maxId) {
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
static int* chopToInt_ref(float* fdus, int n) {
  int* dus = (int*)malloc(sizeof(int) * n);
  #pragma omp parallel for schedule(dynamic, 512)
  for (int i = 0; i < n; i++) {
    dus[i] = (int)fdus[i];
  }
  return dus;
}

static int* getDus_ref(int n, float rate, int* maxVal, int* maxId) {
  float fmaxVal;
  float* expVals = genExp_ref(n, rate, &fmaxVal, maxId);
  int* dus = chopToInt_ref(expVals, n);
  free(expVals);
  *maxVal = (int)fmaxVal;
  return dus;
}

void decompose_ref(graph *g, int *decomp, int* dus, int maxDu, int maxDuId);


#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <omp.h>
#include <bitset>
#include <random>

#include "paraGraph.h"
#include "mic.h"
#include "graph.h"
#include "graph_internal.h"

#define NA -1
#define K 32
#define WORDSIZE 32
#define NUMWORDS 1 // K / 32 where 32 is the number of bits in a word
#define YESRETURN 1
#define NORETURN 0


class RadiiUpdate 
{
  public:
    int** visited;
    int** nextVisited;
    int* radii;
    int iter;

    RadiiUpdate(int** visited, int** nextVisited, int* radii, int iter) :
      visited(visited), nextVisited(nextVisited), radii(radii), iter(iter) {};

    bool update(Vertex src, Vertex dst) {
      bool changed = false;
      for (int j = 0; j < NUMWORDS; j++) {
        if (visited[dst][j] != visited[src][j]) {
          // word-wide or
          __sync_fetch_and_or(&(nextVisited[dst][j]), visited[dst][j] | visited[src][j]);

          int oldRadius = radii[dst];
          if (radii[dst] != iter) {
            changed = changed || __sync_bool_compare_and_swap(&radii[dst], oldRadius, iter);
          }
        }
      }
      return changed;
    }	

    bool cond(Vertex v) {
      return true;
    }
};


class VisitedCopy 
{
  public:
    int** visited;
    int** nextVisited;
    VisitedCopy(int** visited, int** nextVisited) : 
      visited(visited), nextVisited(nextVisited) {};

    bool operator()(Vertex v) {
      for (int j = 0; j < NUMWORDS; j++) { //parallelize this loop with simd
        visited[v][j] = nextVisited[v][j];
      }
      return true;
    }
};

/**
 * Given a set of K source vertices, set them as visited
 * and their radii to 0.
 **/
class Init
{
  public:
    int* S;
    int** visited;
    int** nextVisited;
    int* radii;
    Init(int* S, int** visited, int** nextVisited, int* radii) :
      S(S), visited(visited), nextVisited(nextVisited), radii(radii) {};

    bool operator()(Vertex k) {
      // the kth vertex
      int v = S[k];
      int bit = k % WORDSIZE;
      int word = k/WORDSIZE;

      int oldWord = visited[v][word];
      int newWord = oldWord | 1 << bit;
      bool success = false;

      while (!success) {
        success = __sync_bool_compare_and_swap(&visited[v][word], oldWord, newWord);
	oldWord = visited[v][word];
	newWord = oldWord | 1 << bit;
      }

      oldWord = nextVisited[v][word];
      newWord = oldWord | 1 << bit;
      success = false;

      while (!success) {
	success = __sync_bool_compare_and_swap(&nextVisited[v][word], oldWord, newWord);
    	oldWord = nextVisited[v][word];
	newWord = oldWord | 1 << bit;
      }

      radii[v] = 0;
      return false;
    }

};



/**
  Given a graph, select k random start vertices. Call these 
  k vertices set S. For all vertices v in the graph, find the farthest
  distance between v and any of the vertices in S. Store this is distField.
  
  Note that this implementation is faster than running K separate BFSs in 
  parallel because all k BFSs share a frontier. This means that for each 
  node we must store a bit vector of size K bits, indicating whether or not
  a node has been visited yet by the kth BFS. 
  
  Note that a node v will continue to be added to the frontier as long as its
  source vertex u has a different bit vector than it. This means that v is 
  being visited for the first time by at least one of the K BFSs. Thus we must
  also increment our estimate of its radius since the radius is the FARTHEST
  distance of node v to any of the u source vertices.   

  At the end of the algorithm distField will contain the maximum distance from 
  each node v in the graph to any of the K source nodes. The final radius 
  estimate of the graph is obtained by taking the max radius obtained from all 
  nodes in the graph. Note that this is an estimate because we only ran a BFS 
  from K nodes. If we wanted an exact radius we would have had to run a BFS from
  every single node in the graph.
 **/
void kBFS(graph *g, int *distField) {

  int** visited;
  int** nextVisited;
  int* radii;
  int iter = 0;

  // set up globals
  #pragma omp parallel for schedule(static)
  for (int i = 0; i < g->num_nodes; i++)
    distField[i] = NA;
  radii = distField;

  visited = (int**) malloc(sizeof(int*) * g->num_nodes);
  nextVisited = (int**) malloc(sizeof(int*) * g->num_nodes);

  for (int i = 0; i < g->num_nodes; i++) {
    visited[i] = (int*) malloc(sizeof(int) * NUMWORDS);
    nextVisited[i] = (int*) malloc(sizeof(int) * NUMWORDS);
    memset(visited[i], 0, sizeof(int) * NUMWORDS);
    memset(nextVisited[i], 0, sizeof(int) * NUMWORDS);
  }


  // initialize the frontier with K random nodes
  srand(0);
  int numSources = std::min(K, g->num_nodes);
  int S[numSources]; // the set of source nodes
  for (int i = 0; i < numSources; i++) 
    S[i] = (std::rand()/(float)RAND_MAX) * g->num_nodes;

  VertexSet* frontier = newVertexSet(SPARSE, numSources, g->num_nodes);
  for (int i = 0; i < numSources; i++) {
    addVertex(frontier, S[i]);
  }

  // iterate over values 1 thru k to do initialization
  VertexSet* ks = newVertexSet(SPARSE, numSources, g->num_nodes);
  for (int i = 0; i < numSources; i++) 
    addVertex(ks, i);

  Init i(S, visited, nextVisited, radii);
  vertexMap(ks, i, NORETURN);

  freeVertexSet(ks);

  VertexSet *newFrontier;

  while (frontier->size > 0) {
    iter = iter + 1;
    RadiiUpdate ru(visited, nextVisited, radii, iter);
    newFrontier = edgeMap(g, frontier, ru);

    freeVertexSet(frontier);
    frontier = newFrontier;

    VisitedCopy vc(visited, nextVisited);
    vertexMap(frontier, vc, NORETURN);
  }

  for (int i = 0; i < g->num_nodes; i++) {
    free(visited[i]);
    free(nextVisited[i]);
  }

  freeVertexSet(frontier);
  free(visited);
  free(nextVisited);
}

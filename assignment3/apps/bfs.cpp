#include <stdlib.h>
#include <omp.h>

#include "paraGraph.h"
#include "mic.h"
#include "graph.h"
#include "graph_internal.h"

#define NA -1

class Bfs
{
  public:
    Bfs(Graph g, int* solution)
      : currentDistance(1), distances_(solution)
    {
      for (int i = 0; i < num_nodes(g); i++) {
        distances_[i] = NA;
      }
      distances_[0] = 0;
    }

    bool update(Vertex src, Vertex dst) {
      if (distances_[dst] == NA)
        return __sync_bool_compare_and_swap(&distances_[dst], NA, currentDistance);
      return false;
    }

    bool cond(Vertex v) {
      return distances_[v] == NA;
    }

    int currentDistance;

  private:
    int* distances_;
};


// Finds the BFS distance to each node starting from node 0.
void bfs(graph *g, int *solution) {
  Bfs f(g, solution);

  // Initialize frontier.
  VertexSet* frontier = newVertexSet(SPARSE, 1, num_nodes(g));
  addVertex(frontier, 0);

  VertexSet *newFrontier;

  while (frontier->size != 0) {
    newFrontier = edgeMap<Bfs>(g, frontier, f);
    freeVertexSet(frontier);
    frontier = newFrontier;
    f.currentDistance++;
  }

  freeVertexSet(frontier);
}

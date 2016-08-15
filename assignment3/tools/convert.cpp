#include <stdlib.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>


#include "graph_internal.h"

using namespace std;

typedef unsigned long long int uint64;

struct Edge {
  Edge(uint64 argv0, uint64 argv1) : v0(argv0), v1(argv1) {}
  uint64 v0;
  uint64 v1;
};

struct GroupedNode {
  GroupedNode(int argOrigId, int argGroupId) : groupId(argGroupId), origId(argOrigId) {}
  int groupId;
  int origId;
};

// sort by first node
struct SortEdgesPred {
  bool operator()(const Edge& a, const Edge& b) {
    if (a.v0 == b.v0)
      return a.v1 < b.v1;
    return a.v0 < b.v0;
  }
};

// sort nodes by group
struct SortNodeByGroupPred {
  bool operator()(const GroupedNode& a, const GroupedNode& b) {
    if (a.groupId == b.groupId)
      return a.origId < b.origId;
    return a.groupId < b.groupId;
  }
};


string
trim(const string& str) {
  size_t pos1 = str.find_first_not_of(" \t\n");
  size_t pos2 = str.find_last_not_of(" \t\n");

  if (pos1 == string::npos)
    return "";
  else if (pos2 == string::npos)
    return str.substr(pos1);
  else
    return str.substr(pos1, pos2-pos1+1);
}

bool isComment(const string& str) {
  return trim(str)[0] == '#';
}


void make_graph(graph* g, int num_nodes, const vector<Edge>& edges) {

  g->num_edges = edges.size();
  g->num_nodes = num_nodes;
  
  g->outgoing_starts = new int[g->num_nodes];
  g->outgoing_edges = new Vertex[g->num_edges];
  g->incoming_starts = NULL;
  g->incoming_edges = NULL;

  int next_vertex = 1;

  // scan through the edges, filling in outgoing_edges and also
  // computing output_starts.  The only tricky part is compute starts
  // when there are consecutive notes with no outgoing edges.
  //
  // NOTE(kayvonf): this code assumes that the edges are sorted by
  // source node.
  g->outgoing_starts[0] = 0;
  g->outgoing_edges[0] = edges[0].v1;
  for (size_t i=1; i<g->num_edges; i++) {
    g->outgoing_edges[i] = edges[i].v1;

    if (edges[i].v0 != edges[i-1].v0) {
      while (next_vertex <= edges[i].v0) {
	g->outgoing_starts[next_vertex++] = i;
      }
    }
  }
  while (next_vertex < g->num_nodes)
    g->outgoing_starts[next_vertex++] = g->num_edges;

  

} 

void resort_graph_nodes(graph* g, int* node_groups) {
 
  // given a group id per graph node, resort the nodes in the graph so
  // that all nodes are consecutive
  
  vector<GroupedNode> nodes;
  for (int i=0; i<g->num_nodes; i++)
    nodes.push_back(GroupedNode(i, node_groups[i]));
  
  // sort the nodes according to their group
  sort(nodes.begin(), nodes.end(), SortNodeByGroupPred());

  /*
  printf("New to old mapping:\n");
  for (size_t i=0; i<nodes.size(); i++)
    printf("%lu <-- %d\n", i, nodes[i].origId);
  */

  // now need to make new edges that refer to the new node ids. This
  // requires a mapping from old node id to new node id.
  unordered_map<int,int> oldToNew;
  for (size_t i=0; i<nodes.size(); i++)
    oldToNew.insert({nodes[i].origId, i});
  
  // make a new edges array
  vector<Edge> edges;
  for (int i=0; i<g->num_nodes; i++) {
    int start_edge = g->outgoing_starts[i];
    int end_edge = (i == g->num_nodes-1) ? g->num_edges : g->outgoing_starts[i+1];
    int newNodeId = oldToNew[i];
    for (int e=start_edge; e<end_edge; e++)
      edges.push_back(Edge(newNodeId, oldToNew[g->outgoing_edges[e]]));
  }
  
  std::sort(edges.begin(), edges.end(), SortEdgesPred());

  // clean out the old graph structure
  delete [] g->outgoing_starts;
  delete [] g->outgoing_edges;
  if (g->incoming_starts)
    delete [] g->incoming_starts;
  if (g->incoming_edges)
    delete [] g->incoming_edges;

  // and finally build a new structure
  make_graph(g, g->num_nodes, edges);
  
}

// makes a graph that corresponds to an n x n grid of nodes, with
// edges between nodes adjacent to each other in the cardinal
// directions
void make_grid_graph(const char* filename, int n) {

  vector<Edge> edges;

  for (int j=0; j<n; j++) {
    for (int i=0; i<n; i++) {

      int current = j*n + i;
      int left = current - 1;
      int right = current + 1;
      int up = current - n;
      int down = current + n;
      
      if (j>0)
	edges.push_back(Edge(current, up));
      if (j<n-1)
	edges.push_back(Edge(current, down));
      if (i>0)
	edges.push_back(Edge(current, left));
      if (i<n-1)
	edges.push_back(Edge(current, right));
    }
  }

  std::sort(edges.begin(), edges.end(), SortEdgesPred());

  graph g;
  make_graph(&g, n * n, edges);

  /*
  printf("Before grouped sort:\n");
  for (int i=0; i<g.num_nodes; i++) {
    int start_edge = g.outgoing_starts[i];
    int end_edge = (i == g.num_nodes-1) ? g.num_edges : g.outgoing_starts[i+1];
    printf("%2d: ", i);
    for (int e=start_edge; e<end_edge; e++)
      printf("%d ", g.outgoing_edges[e]);
    printf("\n");
  }
  */

  /*
  int* groups = new int[g.num_nodes];
  for (int i=0; i<g.num_nodes; i++)
    if (i > g.num_nodes /2)
      groups[i] = 0;
    else
      groups[i] = 1;

  resort_graph_nodes(&g, groups);

  printf("After grouped sort:\n");
  for (int i=0; i<g.num_nodes; i++) {
    int start_edge = g.outgoing_starts[i];
    int end_edge = (i == g.num_nodes-1) ? g.num_edges : g.outgoing_starts[i+1];
    printf("%2d: ", i);
    for (int e=start_edge; e<end_edge; e++)
      printf("%d ", g.outgoing_edges[e]);
    printf("\n");
  }
  */

  store_graph_binary(filename, &g);
  
  delete [] g.outgoing_starts;
  delete [] g.outgoing_edges;
  
  printf("Saved graph.\n");
  printf("Now loading graph as a sanity check.\n");

  Graph g2 = load_graph_binary(filename);

  printf("Passed sanity check.\n");
}

// NOTE(kayvonf): these sanity checked are now are deprecated. Can
// delete this function.
void sanity_check(uint64 num_verts, const vector<Edge>& edges) {

  vector<bool> hasOutgoing(num_verts);
  vector<bool> hasIncoming(num_verts);
  
  for (size_t i=0; i<hasOutgoing.size(); i++) {
    hasOutgoing[i] = false;
    hasIncoming[i] = false;
  }
  
  bool badGraph = false;

  // sanity check, confirm valid vertex indices
  for (size_t i=0; i<edges.size(); i++) {
    if (edges[i].v0 >= num_verts) {
      printf("Out of bounds v0 vertex index: %llu at edges[%llu].v0\n", edges[i].v0, i);
      badGraph = true;
    }
    hasOutgoing[edges[i].v0] = true;

    if (edges[i].v1 >= num_verts) {
      printf("Out of bounds v1 vertex index: %llu at edges[%llu].v1\n", edges[i].v1, i);
      //exit(1);
      badGraph = true;
    }
    hasIncoming[edges[i].v1] = true;
  }

  // find vertices with no edges
  int singletons = 0;
  for (size_t i=0; i<hasIncoming.size(); i++)
    if (! (hasIncoming[i] || hasOutgoing[i]) ) {
      if (singletons < 10)
	printf("Vertex %llu has no edges\n", i);
      singletons++;
    }

  if (badGraph)
    printf("**** WARNING: BAD GRAPH ****\n");
}


int main(int argc, char** argv) {

  if (argc < 3) {
    printf("Usage: %s inputfile outputfile\n", argv[0]);
    return 1;
  }

  string inputFilename(argv[1]);
  string outputFilename(argv[2]);
  string line;

  make_grid_graph(outputFilename.c_str(), 4);
  exit(1);

  ifstream file(inputFilename.c_str());
  
  vector<Edge> edges;
  unordered_map<uint64, uint64> nameToId;
  
  uint64 nextUniqueId = 0;

  while (getline(file, line)) {
    line = trim(line);
    if (isComment(line) || line.size() == 0) {
      printf("COMMENT: %s\n", line.c_str());
      continue;
    }

    istringstream iss(line);
    
    uint64 v0, v1;
    iss >> v0 >> v1;

    // remap vertex id in the file to a unique vertex id
    auto it = nameToId.find(v0);
    if (it == nameToId.end()) {
      nameToId.insert({v0,nextUniqueId});
      v0 = nextUniqueId;
      nextUniqueId++;
    } else {
      v0 = it->second;
    }

    // remap vertex id in the file to a unique vertex id
    it = nameToId.find(v1);
    if (it == nameToId.end()) {
      nameToId.insert({v1,nextUniqueId});
      v1 = nextUniqueId;
      nextUniqueId++;
    } else {
      v1 = it->second;
    }

    // add edge.  Note that the new edge refers to the "remapped vertex ids"
    edges.push_back(Edge(v0, v1));

    // remove what's going on every 5M edges
    if (edges.size() % 5000000 == 0) {
      printf("Loaded %.1fM edges\n", static_cast<float>(edges.size()) / 1000000);
    }
  }

  file.close();

  printf("Done loading graph\n");

  // sort edges by the first vertex, this puts all outgoing edges from
  // the same vertex next to each other in the array.
  sort(edges.begin(), edges.end(), SortEdgesPred());

  // find max vertex id in the graph.
  //
  // FIXME(kayvonf): Originally, this search was necessary to find the
  // maximum vertex id.  Now it's unecessary due to vertex id
  // remapping -- all vertices in the graph will be connected to at
  // least one edge, and nextUniqueId tells us the number of vertices
  // in the graph
  uint64 max_vert_index = 0;
  for (size_t i=0; i<edges.size(); i++) {
    max_vert_index = std::max(max_vert_index, std::max(edges[i].v0, edges[i].v1));
  }
  uint64 num_verts = max_vert_index+1;

  if (num_verts != nextUniqueId) {
    printf("Something bad has happened. nextUniqueId (%llu) != num_verts (%llu)\n", nextUniqueId, num_verts);
    exit(1);
  }

  // NOTE(kayvonf): should probably remove this since vertex mapping
  // prevents anything bad getting through now.
  sanity_check(num_verts, edges);

  printf("num_nodes: %llu\n", num_verts);
  printf("num_edges: %lu\n", edges.size());

  graph g;
  make_graph(&g, num_verts, edges);

  store_graph_binary(outputFilename.c_str(), &g);
  delete [] g.outgoing_starts;
  delete [] g.outgoing_edges;
  
  printf("Saved graph.\n");
  printf("Now loading graph as a sanity check.\n");
  
  Graph g2 = load_graph_binary(outputFilename.c_str());

  printf("Passed sanity check.\n");

  return 0;
}

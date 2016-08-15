#!/usr/bin/env python

# Script for grading performance.
# Performance is graded by comparing the student's best wall-clock time
# (not speedup) after running the code in 64, 128, and 240 thread
# configurations for bfs, kbfs, pagerank, and graph decomp against the
# reference solution.

import re
import subprocess
import sys

GRAPHS = [
  "/home/15-418/asst3_graphs/soc-pokec_30m.graph",
  "/home/15-418/asst3_graphs/soc-livejournal1_68m.graph",
  "/home/15-418/asst3_graphs/com-orkut_117m.graph",
  "/home/15-418/asst3_graphs/rmat_200m.graph"
]

# runGraph returns the student's score and total possible score for runinng 3
# algorithms on the given graph.
def runGraph(paraGraph, g):
  args = [
    paraGraph,
    "grade",
    g,
    "-r" # Don't run ref
  ]
  proc = subprocess.Popen(args, stdout=subprocess.PIPE)
  
  while True:
    line = proc.stdout.readline()
    if line != '':
      line = line.strip();
      # Print the line so the user can see the detailed timing breakdown.
      print line
      matchObj = re.match(r'Total Grade: ([\d\.]*)\/([\d\.]*)$', line, re.M)
      if matchObj:
        return float(matchObj.group(1)), float(matchObj.group(2))
    else:
      break

  return -1, -1

def main():
  if len(sys.argv) != 2:
    print "Usage: ./grade_peformance.py <path to paraGraph>"
    sys.exit(1)
  
  paraGraph = sys.argv[1]

  score = 0
  possibleScore = 0
  for g in GRAPHS:
    print "Timing " + g
    graphScore, pScore = runGraph(paraGraph, g) 
    if graphScore < 0:
      sys.stderr.write("Error parsing total grade for graph " + g + "\n")
    score += graphScore
    possibleScore += pScore

  print ""
  print "**************************************************"
  print "Final Performance Score: %f/%f" % (score, possibleScore)
  print "**************************************************"

if __name__ == "__main__":
  main()

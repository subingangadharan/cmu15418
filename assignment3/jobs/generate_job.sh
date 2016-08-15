#/usr/bin/env bash

app=$1
graph=$2

if [ ${#} -ne 2 ]; then
  echo "Usage: $0 <app> <path to graph>"
else
  curdir=`pwd`
  curdir=${curdir%/jobs}
  sed "s:PROGDIR:${curdir}:g" example.job > tmp1.job
  graphname=$(basename ${graph})
  sed "s:APP:${app}:g" tmp1.job > tmp2.job
  sed "s:GRAPH:${graph}:g" tmp2.job > ${USER}_${app}_${graphname}.job
  rm -f tmp1.job tmp2.job
fi

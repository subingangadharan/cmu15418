#/usr/bin/env bash

curdir=`pwd`
curdir=${curdir%/jobs}
sed "s:PROGDIR:${curdir}:g" grade_performance_example.job > ${USER}_grade_performance.job

#!/bin/bash

# First argument is the maximum number of workers to allocate for your job; the
# cluster will reserve that many computers, plus one for the master, for the
# duration of the job.
N_WORKERS=$1

# Second argument is the trace to run.
TRACE=$2

qsub -F "$N_WORKERS $TRACE" -l nodes=$((N_WORKERS + 1)):ppn=24 latedays.qsub

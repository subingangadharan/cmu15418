#!/bin/bash
CMD=$0
NUM_WORKERS=$1
TRACE=$2
JOBNAME=$PBS_JOBID

cd $PBS_O_WORKDIR
PATH=$PATH:$PBS_O_PATH

USERNAME=`whoami`
MASTER=`head -n1 $PBS_NODEFILE`
NODES=`sort -u $PBS_NODEFILE`

KILL_STRING="pkill -9 -u $USERNAME -x master; pkill -9 -u $USERNAME -x worker"

if [ $MASTER.local = $HOSTNAME ] ; then
  echo $LD_LIBRARY_PATH
  echo "spawning master process on $HOSTNAME ($PBS_NODENUM of $PBS_NUM_NODES)"
  echo $NODES
  echo "Username is " $USERNAME

  echo "command was: '$CMD $NUM_WORKERS $TRACE'"

  . ~/.bashrc
  . ~/.bash_profile
  module load opt-python
  module load gcc-4.9.2

  # make a logs directory
  mkdir -p logs.$JOBNAME

  # Start the launcher and sleep a moment to make sure it is listening.
  ./scripts/nodemanager_local.py --nodefile $PBS_NODEFILE 6666 --log_dir=logs.$JOBNAME $debug_cflags $configflags &
  nodemanager_pid=$!

  # Start the master and sleep a moment to make sure it is listening.
  ./master --max_workers $NUM_WORKERS --address=$(hostname):15418 --log_dir=logs.$JOBNAME $debug_cflags $(hostname):6666 &
  master_pid=$!

  function cleanup_processes {

    echo "Cleaning up after a completed test..."

    # Kill the master and worker node launcher  
    pkill -9 -u $USERNAME -x master
    pkill -9 -u $USERNAME -x "scripts/nodemanager_local.py"

    # try really, really hard to clean any trailing worker processes
    # up on the worker nodes
    for i in $NODES; do
       ssh $i.local "$KILL_STRING"
       #ssh compute-0-31.local "$KILL_STRING"
    done
  }

  trap cleanup_processes SIGINT SIGTERM SIGKILL

  # Run the test harness (this python script generates all the requests
  # and verifies the correctness of the server's responses)
  ./scripts/workgen.py $debug_pyflags $(hostname):15418 $TRACE

  success=$?

  # Tell the master to die by sending it the tagged message (shutdown, 0).
  #python -c 'import comm; import sys; sys.stdout.write(comm.TaggedMessage(comm.SHUTDOWN, 0).to_bytes())' | nc $(hostname) 15418
  printf "\x06\x00\x00\x00\x00\x00\x00\x00" | nc $(hostname) 15418

  # wait for the master to complete
  wait $master_pid

  cleanup_processes

  # Return successfully if the all work was accepted correctly.
  exit $success
fi


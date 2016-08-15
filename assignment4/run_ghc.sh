#!/bin/bash

# Optional third argument! We will use ports 15418 and 15419 by default.
launcher_port=${3-15418}
master_port=$((launcher_port + 1))

# make a logs directory
mkdir -p logs

# Start the launcher and sleep a moment to make sure it is listening.
./scripts/nodemanager_local.py $debug_pyflags $launcher_port --log_dir=logs $debug_cflags $configflags &
nodemanager_pid=$!

# Start the master and sleep a moment to make sure it is listening.
./master --max_workers $1 --address=$(hostname):$master_port --log_dir=logs $debug_cflags $(hostname):$launcher_port &
master_pid=$!

# Run the test harness (this python script generates all the requests
# and verifies the correctness of the server's responses)
./scripts/workgen.py $debug_pyflags $(hostname):$master_port $2

success=$?

# Tell the master to die by sending it the tagged message (shutdown, 0).
#python -c 'import comm; import sys; sys.stdout.write(comm.TaggedMessage(comm.SHUTDOWN, 0).to_bytes())' | nc $(hostname) 15418
printf "\x06\x00\x00\x00\x00\x00\x00\x00" | nc $(hostname) $master_port
trap "kill -9 $master_pid; kill -9 $nodemanager_pid; exit $success" SIGINT
wait $master_pid

# Kill the the launcher to clean up.
kill -9 $nodemanager_pid

# Return successfully iff the all work was accepted correctly.
exit $success

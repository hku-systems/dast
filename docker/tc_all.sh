#!/bin/bash

for i in `seq $4`
do
	echo "tc for $i"
	ssh $3.$i "cd /janus && ./docker/setup-tc.sh $1 $2"
done

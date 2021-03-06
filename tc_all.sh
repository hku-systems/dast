#!/bin/bash

sleep 5

for i in `seq $4`
do
	echo "tc for $i"
	ssh $3_$i "cd /janus && ./setup-tc.sh $1 $2"
done

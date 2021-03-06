#!/bin/bash

for i in `seq $2`
do
	echo "add key for $i"
	ssh-keyscan -H $1.$i >> ~/.ssh/known_hosts
done

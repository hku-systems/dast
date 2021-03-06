#!/bin/bash

if [ $1 == "start" ]
then
    sudo docker network create -d overlay --subnet=10.44.0.0/16 --attachable edc-cluster-worker
else
    sudo docker network rm edc-cluster-worker
fi
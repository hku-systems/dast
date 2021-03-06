#!/bin/bash

if [ $1 == "start" ]
then
    /mnt/share_cluster/run_command_cluster.sh "echo $2 | sudo -S docker volume create --driver local --opt type=nfs --opt o=addr=10.22.1.1,rw --opt device=:/opt/share_cluster/edc-cluster edc-nfs"
else
    /mnt/share_cluster/run_command_cluster.sh "echo $2 | sudo -S docker volume rm edc-nfs"
fi

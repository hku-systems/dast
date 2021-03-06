#!/bin/bash

/mnt/share_cluster/run_command_cluster.sh "echo $1 | sudo -S docker pull 10.22.1.1:5000/edc:latest"
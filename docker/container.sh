#!/bin/bash

set -x

n=10
N_MACHINE=18

MASTER_NETWORK=10.44.2
MASTER_NETWORK_NAME=edc-cluster-worker
MASTER_IP=$MASTER_NETWORK.240

WORKER_NETWORK=10.44.2
WORKER_NETWORK_NAME=edc-cluster-worker

PASSWD=$2
WORKER_PREFIX="edc_worker_"
MASTER_PREFIX="edc_master"

machines=(4 4 3 3 2 3 2 3 2 3 3 4 5 1 2 3 4 5 6 7 8 9 10 12 13 14 15 16 17 18 1 2 5 6 7 8 9 10 12 13 14 15 16 17 18 1 2 5 6 7 8 9 10 12 13 14 15 16 17 18 1 2 5 6 7 8 9 10 12 13 14 15 16 17 18 1 2 3 4 5 6 7 8 9 10 1 2 3 4 5 6 7 8 9 10 1 2 3 4 5 6 7 8 9 10 1 2 3 4 5 6 7 8 9 10 1 2 3 4 5 6 7 8 9 10 1 2 3 4 5 6 7 8 9 10 12 13 14 15 16 17 18)

if [ $1 == "start" ]
then

    machine_cur=0
    instance_cur=1

    if [ ! $2 == "master" ]
    then
        while [ $instance_cur -le $n ]
        do
            idx=${machines[$machine_cur]}
            echo "deploy $instance_cur at $idx"
            
            # ssh 10.22.1.$idx "echo $PASSWD | sudo -S docker run -d -v edc-nfs:/janus --cap-add NET_ADMIN --network $WORKER_NETWORK_NAME --name $WORKER_PREFIX$machine_cur --ip $WORKER_NETWORK.$instance_cur 10.22.1.1:5000/edc:latest /janus/docker/tc_ssh.sh eth0 $MASTER_IP"
            # ssh 10.22.1.$idx "echo $PASSWD | sudo -S docker network connect $MASTER_NETWORK_NAME $WORKER_PREFIX$machine_cur"
            ssh 10.22.1.$idx "echo $PASSWD | sudo -S docker run -d --cap-add NET_ADMIN --network $WORKER_NETWORK_NAME --name $WORKER_PREFIX$machine_cur --ip $WORKER_NETWORK.$instance_cur 10.22.1.1:5000/dast:latest /janus/docker/tc_ssh.sh eth0 $MASTER_IP"
            machine_cur=$((machine_cur + 1))
            instance_cur=$((instance_cur + 1))
        done
    else
    # single machine
    # for i in `seq $n`
    # do
    #     echo "start worker $i"
    #     docker run -d -v edc-nfs:/janus --cap-add NET_ADMIN --network edc-cluster --ip $WORKER_NETWORK.$i edc /usr/sbin/sshd -D
    # done

    # sudo docker run -it -v edc-nfs:/janus --cap-add NET_ADMIN --network $MASTER_NETWORK_NAME --name $MASTER_PREFIX --ip $MASTER_IP 10.22.1.1:5000/edc:latest bash
    # sudo docker run -d -v edc-nfs:/janus --cap-add NET_ADMIN --network $MASTER_NETWORK_NAME --name $MASTER_PREFIX --ip $MASTER_IP 10.22.1.1:5000/edc:latest /usr/sbin/sshd -D
    # sudo docker network connect $WORKER_NETWORK_NAME $MASTER_PREFIX

    sudo docker run -v edc-nfs:/janus --cap-add NET_ADMIN --network $WORKER_NETWORK_NAME --ip $MASTER_IP 10.22.1.1:5000/edc:latest bash -c "cd /janus \
        && ./docker/scan_all.sh $WORKER_NETWORK $n \
        && python3 exp1_n_clients.py"
    fi
else
    for i in `seq $N_MACHINE`
    do
        ssh 10.22.1.$i "echo $PASSWD | sudo -S /mnt/share_cluster/edc-cluster/docker/stop-container.sh"
    done
    echo "All containers have been stop."
fi

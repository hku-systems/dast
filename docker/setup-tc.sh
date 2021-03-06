#!/bin/bash
set -x
# ./setup-tc.sh eth0 master


my_ip=$1



# master_ip="$2"
# network_str=$(ifconfig $device|grep 'inet addr'|tr -s ' '| cut -d' ' -f3|cut -d':' -f2)
# network=$(echo $network_str|cut -d'.' -f1).$(echo $network_str|cut -d'.' -f2).0.0


# tc qdisc add dev $device root netem delay 50ms 30ms
# tc qdisc add dev eth0 root netem delay 50ms
tc qdisc del dev lo root
tc qdisc add dev lo root netem delay 2.5ms

tc qdisc del dev eth0 root
tc qdisc add dev eth0 handle 1: root htb default 2

tc class add dev eth0 parent 1: classid 1:1 htb rate 40Gbit
tc class add dev eth0 parent 1: classid 1:2 htb rate 40Gbit

tc qdisc add dev eth0 parent 1:1 handle 22: netem delay 2.5ms
if [[ $# -eq 1 ]]
then 
  tc qdisc add dev eth0 parent 1:2 handle 11: netem delay 50ms
else 
  tc qdisc add dev eth0 parent 1:2 handle 11: netem delay 50ms ${2}ms
fi
tc filter add dev eth0 parent 1: prio 2 protocol ip u32 match ip dst ${my_ip}/24 classid 1:1
tc filter add dev eth0 parent 1: prio 3 protocol ip u32 match ip dst ${my_ip}/16 flowid 1:2
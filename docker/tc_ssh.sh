#!/bin/bash


/janus/docker/setup-tc.sh $1

echo "start ssh server"
echo "MaxSessions 100" >> /etc/ssh/sshd_config
echo "MaxStartups 100" >> /etc/ssh/sshd_config

/usr/sbin/sshd -D

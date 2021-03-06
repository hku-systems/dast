from exp_config import *
import subprocess
import yaml


NETWORK_NAME = "dast_network"
CONTAINER_NAME_PREFIX = "dast_"


hosts = [] 
with open (HOSTS_FILE, 'r') as f:
    data = yaml.load(f)
    hosts = data['hosts']

containers = []

for i in range(MAX_REGIONS):
    # each region takes a subnet
    for j in range(N_SHARD_PER_REGION):
        index = i * N_SHARD_PER_REGION + j
        # print(index)
        host = hosts[index % len(hosts)]
        IP = NETWORK_BASE + "." + str(i+1) + "." + str(j)
        containers.append(IP)
        print(index, host, IP)

        container_name = CONTAINER_NAME_PREFIX + str(index)

        all_subnet = NETWORK_BASE + ".0.0"
        ssh_command = "docker run -d --cap-add NET_ADMIN --network " + NETWORK_NAME +  " --name " + container_name + " --ip " + IP + " " + IMAGE_NAME + " /janus/docker/tc_ssh.sh " + IP 
        print(ssh_command)
        subprocess.call(['ssh', host, ssh_command])

with open(CONTAINERS_FILE, 'w+') as f:
    data = {}
    data['containers'] = containers
    yaml.dump(data, f)






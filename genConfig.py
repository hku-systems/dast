import yaml
from exp_config import *


# each region is in one container
# intra-container RTT is 5 ms
# cross-container RTT is 100 ms

# should be smaller the number of available containers
# all least be 3


# by default three
n_rep_per_shard = 3

# this is not the level of clients per shard
# each client process has a concureny level
# but too high concurrency level make the client process bottleneck
# so use multiple client process.
n_client_process_per_shard = 3

containers = {}
with open(CONTAINERS_FILE) as f:
    containers = yaml.load(f)

print(containers)

output_config = {}

output_config['site'] = {}
output_config['site']['server'] = []
output_config['site']['client'] = []
output_config['process'] = {}
output_config['host'] = {}

siteid = 0
port = 8000
c_siteid = 0
for i in range(MAX_REGIONS):
    for j in range(N_SHARD_PER_REGION):
        shard_sites = []
        for k in range(n_rep_per_shard):
            site_name = "s" + str(siteid)
            shard_sites.append(site_name + ":" + str(port))

            proces_name = "S" + \
                str(i * N_SHARD_PER_REGION + j + 1) + str(chr(ord('A') + k))
            output_config['process'][site_name] = proces_name

            host_name = containers['containers'][i * N_SHARD_PER_REGION + j]
            output_config['host'][proces_name] = host_name

            siteid = siteid + 1
            port = port + 1
        output_config['site']['server'].append(shard_sites)

        client_sites = []
        for k in range(n_client_process_per_shard):
            c_site_name = "c" + str(c_siteid)
            client_sites.append(c_site_name)

            c_proces_name = "C" + \
                str(i * N_SHARD_PER_REGION + j + 1) + str(chr(ord('A') + k))
            output_config['process'][c_site_name] = c_proces_name

            c_host_name = containers['containers'][i * N_SHARD_PER_REGION + j]
            output_config['host'][c_proces_name] = c_host_name

            c_siteid = c_siteid + 1
        output_config['site']['client'].append(client_sites)

output_config['auxiliaryraft'] = {}
output_config['auxiliaryraft']['S1A'] = 25000
second_rep = 1 + N_SHARD_PER_REGION
output_config['auxiliaryraft']["S" + str(second_rep) + "A"] = 25001
third_rep = 1 + 2 * N_SHARD_PER_REGION
output_config['auxiliaryraft']["S" + str(third_rep) + "A"] = 25002

output_config['dastparams'] = {}
output_config['dastparams']['local_sync_interval_ms'] = 5

output_config['n_shard_per_region'] = N_SHARD_PER_REGION

print(yaml.dump(output_config, ))
with open(DEPLOYMENT_FILE, 'w') as f:
    yaml.dump(output_config, f)

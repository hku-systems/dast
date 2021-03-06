## All configs that need to be adjusted. 

## you do not need to change these values
CONTAINERS_FILE = "containers.yml"
HOSTS_FILE = 'hosts.yml'
DEPLOYMENT_FILE = 'config/out.yml'
IMAGE_NAME = "hkusys/dast:latest"
NETWORK_BASE = "10.55"



N_SHARD_PER_REGION = 2

# The number of regions used in all experiments (except for exp 4 scalability)
N_REGIONS = 4 # Must >= 3 

## The total number of regions deployed. 
MAX_REGIONS = 6

# chronos = dast
# brq = janus
SysNames = ['chronos', 'brq', 'slog', 'tapir']

## USE 90 or 95 if your cluster is not clean (i.e., with resource contention)
TAIL_PERCENTILE = 99
# TAIL_PERCENTILE = 95
# TAIL_PERCENTILE = 90


# The number of trials for each data point. 
# using 3 or 5 can avoid outliers
# N_REPEAT = 3
N_REPEAT = 5
# N_REPEAT = 1

# the total number of shards for all experiments except for experiment 4 (scalability)






#####################################
#           Experiment 1            #
# ###################################
# the number of client thread in each client process 
# Note that this is not "client per region"
# CONCURRENCY_LEVELS = [20, 40]
# CONCURRENCY_LEVELS = [5, 10, 20, 30, 40, 50]
CONCURRENCY_LEVELS = [1, 5, 10, 15, 20, 25, 30, 40, 50]
CLIENT_PROCESS_PER_SHARD = [1, 2, 3]
# CLIENT_PROCESS_PER_SHARD = [2]
# client per region = client_process_per_shard * shard_per_region * concurrency_level (automatically calculated)

#####################################
#           Experiment 2            #
# ###################################
# iterate through different ratios of transaction being CRTs.
# Fewer data points can speed up the experiment but also show the same trends.
CRT_RATIOS = [10, 20, 30, 40, 50, 60, 70, 80, 90]
# CRT_RATIOS = [10, 90]
# CRT_RATIOS = [10, 30, 50, 70, 90]

# Do we need the results of baseline systems
EXP2_DAST_ONLY = True

#####################################
#           Experiment 3            #
# ###################################
# iterate through different number of zipf coefficient values. 
# zipfs = [0.5, 0.7, 1]
zipfs = [0.5, 0.6, 0.7, 0.8, 0.9, 1]

# Do we need the results of baseline systems
EXP3_DAST_ONLY = True


#####################################
#           Experiment 4            #
# ###################################
# iterate through number of regions. 
# must be at least 3 if you want to test SLOG
# must be smaller than max regions. 
SCALABILITY_N_REGIONS = [3, 5]

# Do we need the results of baseline systems
EXP4_DAST_ONLY = True

#####################################
#           Experiment 5            #
# ###################################
# iterate through number of 100 +- x ms for RTT
ROUBUST_RTT_VAR = [10, 20, 30, 40, 50, 60, 70, 80]
# ROUBUST_RTT_VAR = [20, 50, 80]

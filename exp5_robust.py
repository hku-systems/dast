import subprocess
from exp_config import *
import genFigure
import yaml


trial_name = "scalability"

sysname = 'chronos'
trial_name = 'robust'

containers  = []
with open(CONTAINERS_FILE, 'r') as f:
    data = yaml.load(f)
    containers = data['containers']
print(containers)

for v in ROUBUST_RTT_VAR:
    for c in containers:
        ssh_cmd = '/janus/docker/setup-tc.sh ' + c + ' ' + str(v) 
        print(ssh_cmd)
        subprocess.call(['ssh', c, ssh_cmd])

    cmd = ["./run_all.py"]
    cmd.extend(["-hh", "config/out.yml"])
    cmd.extend(["-s", str(N_REGIONS * N_SHARD_PER_REGION)])
    cmd.extend(["-c", str(N_REGIONS * N_SHARD_PER_REGION * 2)])
    cmd.extend(["-r", "3"])
    cmd.extend(["-cc", "config/tpcc.yml"])
    cmd.extend(["-cc", "config/client_closed.yml"])
    cmd.extend(["-cc", "config/concurrent_15.yml"]) 
    cmd.extend(["-cc", "config/" + sysname + ".yml"])
    cmd.extend(["-b", "tpcc"])
    cmd.extend(["-m", sysname + ":" + sysname])
    cmd.extend(["-t", trial_name])
    cmd.extend(["--allow-client-overlap"])
    cmd.extend([str(v)])

    cmd = [str(c) for c in cmd]

    res = subprocess.call(cmd)


genFigure.drawFig(trial_name)


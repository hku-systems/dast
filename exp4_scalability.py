import subprocess
from exp_config import *
import genFigure


trial_name = "scalability"

targetSys = []
if EXP3_DAST_ONLY:
        targetSys = ["chronos"]
else: 
        targetSys = SysNames

for sysname in targetSys:
    for nr in SCALABILITY_N_REGIONS:
        cmd = ["./run_all.py"]
        cmd.extend(["-hh", "config/out.yml"])
        cmd.extend(["-s", str(nr * N_SHARD_PER_REGION)])
        cmd.extend(["-c", str(nr * N_SHARD_PER_REGION * 2)])
        cmd.extend(["-r", "3"])
        cmd.extend(["-cc", "config/tpcc.yml"])
        cmd.extend(["-cc", "config/client_closed.yml"])
        cmd.extend(["-cc", "config/concurrent_15.yml"]) 
        cmd.extend(["-cc", "config/" + sysname + ".yml"])
        cmd.extend(["-b", "tpcc"])
        cmd.extend(["-m", sysname + ":" + sysname])
        cmd.extend(["-t", trial_name])
        cmd.extend(["--allow-client-overlap"])
        cmd.extend(["testing"])

        cmd = [str(c) for c in cmd]

        res = subprocess.call(cmd)


genFigure.drawFig(trial_name)


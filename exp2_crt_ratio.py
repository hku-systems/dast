import subprocess
from exp_config import *
import genFigure


trial_name = "crt_ratio"

targetSys = []
if EXP2_DAST_ONLY:
        targetSys = ["chronos"]
else: 
        targetSys = SysNames

for sysname in targetSys:
        cmd = ["./run_all.py"]
        cmd.extend(["-hh", "config/out.yml"])
        cmd.extend(["-s", str(N_REGIONS * N_SHARD_PER_REGION)])
        cmd.extend(["-c", str(N_REGIONS * N_SHARD_PER_REGION)])
        cmd.extend(["-r", "3"])
        cmd.extend(["-cc", "config/tpcc_crt_ratio.yml"])
        cmd.extend(["-cc", "config/client_closed.yml"])
        cmd.extend(["-cc", "config/concurrent_25.yml"]) # default
        cmd.extend(["-cc", "config/" + sysname + ".yml"])
        cmd.extend(["-b", "tpcc"])
        cmd.extend(["-m", sysname + ":" + sysname])
        cmd.extend(["-t", trial_name])
        cmd.extend(["--allow-client-overlap"])
        cmd.extend(["testing"])

        cmd = [str(c) for c in cmd]

        res = subprocess.call(cmd)

genFigure.drawFig(trial_name)


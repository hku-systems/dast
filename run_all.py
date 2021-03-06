#! /usr/bin/env python
import sys
import copy
import traceback
import os
import os.path
import tempfile
import subprocess
import itertools
import shutil
import glob
import signal

from argparse import ArgumentParser
from logging import info, debug, error
from pylib.placement_strategy import ClientPlacement, BalancedPlacementStrategy, LeaderPlacementStrategy
from exp_config import *

import logging
import yaml


DEFAULT_MODES = ["tpl_ww:multi_paxos",
                 "occ:multi_paxos",
                 "tapir:tapir",
                 "brq:brq"]

DEFAULT_CLIENTS = ["1:2"]
DEFAULT_SERVERS = ["1:2"]
DEFAULT_BENCHMARKS = [ "rw_benchmark", "tpccd" ]
DEFAULT_TRIAL_DURATION = 15
DEFAULT_EXECUTABLE = "./run.py"

APPEND_DEFAULTS = {
    'client_counts': DEFAULT_CLIENTS,
    'server_counts': DEFAULT_SERVERS, 
    'benchmarks': DEFAULT_BENCHMARKS,
    'modes': DEFAULT_MODES,
}
TMP_DIR='./tmp'

logging.basicConfig(format='%(asctime)s,%(msecs)d %(levelname)-8s [%(filename)s:%(lineno)d] %(message)s',
    datefmt='%Y-%m-%d:%H:%M:%S',
    level=logging.DEBUG)

logger = logging.getLogger('')


def create_parser():
    parser = ArgumentParser()
    parser.add_argument(dest="experiment_name",
                        help="name of experiment")
    parser.add_argument('-e', "--executable", dest="executable",
                        help="the executable to run",
                        default = DEFAULT_EXECUTABLE)

    parser.add_argument('-t', "--trial", dest="trial_name",
                        help="the trials to run, must be one of \"n_clients\", \"crt_ratio\", \"scalability\", \"tpca\"",
                        default = "", 
                        required= True)

    parser.add_argument("-c", "--client-count", dest="client_counts",
                        help="client counts; accpets " + \
                        "'<start>:<stop>:<step>' tuples with same semantics as " + \
                        "range builtin function.",
                        action='append',
                        default=[])
    parser.add_argument("-cl", "--client-load", dest="client_loads",
                        help="load generation for open clients",
                        nargs="+", type=int, default=[-1])
    parser.add_argument("-st", "--server-timeout", dest="s_timeout", default="30")
    parser.add_argument("-s", "--server-count", dest="server_counts",
                        help="client counts; accpets " + \
                        "'<start>:<stop>:<step>' tuples with same semantics as " + \
                        "range builtin function.",
                        action='append',
                        default =[])
    parser.add_argument("-z", "--zipf", dest="zipf", default=[None],
                        help="zipf values",
                        nargs="+", type=str)
    parser.add_argument("-d", "--duration", dest="duration",
                        help="trial duration",
                        type=int,
                        default = DEFAULT_TRIAL_DURATION)
    parser.add_argument("-r", "--replicas", dest="num_replicas",
                        help="number of replicas",
                        type=int,
                        default = 1)
    parser.add_argument('-b', "--benchmarks", dest="benchmarks",
                        help="the benchmarks to run",
                        action='append',
                        default =[])
    parser.add_argument("-m", "--modes", dest="modes",
                        help="Concurrency control modes; tuple '<cc-mode>:<ab-mode>'",
                        action='append',
                        default=[])
    parser.add_argument("-hh", "--hosts", dest="hosts_file",
                        help="path to file containing hosts yml config",
                        default='config/hosts.yml',
                        required=True)
    parser.add_argument("-cc", "--config", dest="other_config",
                        action='append',
                        default=[],
                        help="path to yml config (not containing processes)")
    parser.add_argument("-g", "--generate-graphs", dest="generate_graph",
                        action='store_true', default=False, help="generate graphs")
    parser.add_argument("-cp", "--client-placement", dest="client_placement",
                        choices=[ClientPlacement.BALANCED, ClientPlacement.WITH_LEADER], 
                        default=ClientPlacement.BALANCED, help="client placement strategy (with leader for multipaxos)")
    parser.add_argument("-u", "--cpu-count", dest="cpu_count", type=int,
                        default=1, help="number of cores on the servers")
    parser.add_argument("-dc", "--data-centers", dest="data_centers", nargs="+", type=str,
                        default=[], help="data center names (for multi-dc setup)")
    parser.add_argument("--allow-client-overlap", dest="allow_client_overlap",
                        action='store_true', default=False, help="allow clients and server to be mapped to same machine (for testing locally)")
    return parser


def parse_commandline():
    args = create_parser().parse_args()
    for k,v in args.__dict__.iteritems():
        if k in APPEND_DEFAULTS and v == []:
            args.__dict__[k] = APPEND_DEFAULTS[k] 
    return args


def gen_experiment_suffix(b, m, c, z, cl):
    m = m.replace(':', '-')
    if z is not None:
        return "{}_{}_{}_{}_{}".format(b, m, c, z, cl)
    else:
        return "{}_{}_{}_{}".format(b, m, c, cl)


def get_range(r):
    a = []
    parts = r.split(':')
    for p in parts:
        a.append(int(p))
    if len(parts)==1:
        return range(a[0],a[0]+1)
    else:
        return range(*a)


def gen_process_and_site(args, experiment_name, num_c, num_s, num_replicas, hosts_config, mode):
    logger.info("gen_process_and_site")
    hosts = hosts_config['host']
    logger.debug("hosts", hosts)

    
    layout = generate_layout(args, num_c, num_s, num_replicas, hosts_config)

    if not os.path.isdir(TMP_DIR):
        os.makedirs(TMP_DIR)

    site_process_file = tempfile.NamedTemporaryFile(
        mode='w', 
        prefix='janus-proc-{}'.format(experiment_name),
        suffix='.yml',
        dir=TMP_DIR,
        delete=False)
                                                
    contents = yaml.dump(layout, default_flow_style=False)
    
    result = None 
    with site_process_file:
        site_process_file.write(contents)
        result = site_process_file.name

    return result 

def load_config(fn):
    with open(fn, 'r') as f:
        contents = yaml.load(f)
        return contents

def modify_dynamic_params(args, benchmark, mode, abmode, zipf):
    output_configs = []
    configs = args.other_config
    
    for config in configs:
        modified = False

        output_config = config
        config = load_config(config)

        if 'bench' in config:
            if 'workload' in config['bench']:
                config['bench']['workload'] = benchmark
                modified = True
            if 'dist' in config['bench'] and zipf is not None:
                try:
                    zipf_value = float(zipf)
                    config['bench']['coefficient'] = zipf_value
                    config['bench']['dist'] = 'zipf' 
                except ValueError:
                    config['bench']['dist'] = str(zipf)
                modified = True
        if 'mode' in config and 'cc' in config['mode']:
            config['mode']['cc'] = mode
            modified = True
        if 'mode' in config and 'ab' in config['mode']:
            config['mode']['ab'] = abmode
            modified = True
         
        
        if modified:
            f = tempfile.NamedTemporaryFile(
                mode='w', 
                prefix='janus-other-{}'.format(args.experiment_name),
                suffix='.yml',
                dir=TMP_DIR,
                delete=False)
            output_config = f.name
            logger.debug("generated config: %s", output_config)
            contents = yaml.dump(config, default_flow_style=False)
            with f:
                f.write(contents)

        output_configs.append(output_config)
    return output_configs


def aggregate_configs(*args):
    logging.debug("aggregate configs: {}".format(args))
    config = {}
    for fn in args:
        config.update(load_config(fn))
    return config

def generate_layout(args, num_c, num_s, num_replicas, hosts_config):
        logger.debug("generate layout called")
        data_centers = args.data_centers
        logger.debug(data_centers)


        proc_names = hosts_config['host'].keys()
        # hosts = self.hosts_by_datacenter(hosts_config['host'].keys(), data_centers)
        print("procs", proc_names)
        server_sites_all = hosts_config['site']['server']
        server_sites = []
        # truncate to number of shards and number of replicas
        # This will 
        for partition in range(num_s):
            par = []
            for replica in range(num_replicas):
                if len(server_sites_all) < num_s: 
                    logging.fatal("not enough server sites (number of partitions) in config file")
                    exit(1)
                if len(server_sites_all[partition]) < num_replicas: 
                    logging.fatal("not enough server sites (number of replicas) in config file")
                    exit(1)
                par.append(server_sites_all[partition][replica])
            server_sites.append(par)

        print(server_sites)

        client_sites_all = hosts_config['site']['client']
        client_sites = []
        n_par = len(server_sites)
        if num_c > n_par:
            # reserve the positions
            for partition in range(n_par):
                client_sites.append([])
            # fill in num_c clients vertically
            col_itr = 0
            count = 0
            while col_itr < len(client_sites_all[0]):
                for partition in range(n_par):
                    if col_itr >= len(client_sites_all[partition]):
                        logging.fatal("not enough client sites (number of replicas) in config file")
                        exit(1)
                    client_sites[partition].append(client_sites_all[partition][col_itr])
                    count += 1
                    if count == num_c:
                       break
                col_itr += 1
                if count == num_c:
                    break
        else:
            # num_c < n_par    
            for i in range(num_c):
                par = []
                par.append(client_sites_all[i][0])
                client_sites.append(par)
        print(client_sites) 

        site = {'client': client_sites, 'server': server_sites}
        process_all = hosts_config['process']
        process = {}
        for s in process_all.keys():
            exist = False
            for row in client_sites:
                for c in row: 
                    if s == c:
                        exist = True 
            for row in server_sites:
                for c in row: 
                    if s == c.split(":")[0]:
                        exist = True
            if exist:
                process[s] = process_all[s]
        # process = hosts_config['process']

        print(process)
        result = {'site': site, 'process': process}
        print("result", result)
        return result

def generate_config(args, experiment_name, benchmark, mode, zipf, client_load, num_client,
                    num_server, num_replicas):
    logger.debug("generate_config: {}, {}, {}, {}, {}".format(
        experiment_name, benchmark, mode, num_client, zipf))
    hosts_config = load_config(args.hosts_file)
    proc_and_site_config = gen_process_and_site(args, experiment_name,
                                                num_client, num_server,
                                                num_replicas, hosts_config, mode)
    
    logger.debug("site and process config: %s", proc_and_site_config)
    cc_mode, ab_mode = mode.split(':')
    config_files = modify_dynamic_params(args, benchmark, cc_mode, ab_mode,
                                         zipf) 
    config_files.insert(0, args.hosts_file)
    config_files.append(proc_and_site_config)
    logger.info(config_files)
    result = aggregate_configs(*config_files)

    if result['client']['type'] == 'open':
        if client_load == -1:
            logger.fatal("must set client load param for open clients")
            sys.exit(1)
        else:
            result['client']['rate'] = client_load

    with tempfile.NamedTemporaryFile(
        mode='w', 
        prefix='janus-final-{}'.format(args.experiment_name),
        suffix='.yml',
        dir=TMP_DIR,
        delete=False) as f:
        f.write(yaml.dump(result))
        result = f.name
    logger.info("result: %s", result)
    return result

exp_id = 0
def run_experiment(config_file, name, args, benchmark, mode, num_client):
    global exp_id
    exp_id += 1
    cmd = [args.executable]
    cmd.extend(["-id", exp_id])
    cmd.extend(["-n", "{}".format(name)])
    cmd.extend(["-t", str(args.s_timeout)])
    cmd.extend(["-f", config_file]) 
    cmd.extend(["-d", args.duration])
    cmd.extend(["-e", args.trial_name])
    cmd = [str(c) for c in cmd]

    logger.info("running: %s", " ".join(cmd))
    res=subprocess.call(cmd)
    if res != 0:
        logger.error("subprocess returned %d", res)
    else:
        logger.info("subprocess success.")
    return res

def save_git_revision():
    rev_file = "/home/ubuntu/janus/build/revision.txt"
    if os.path.isfile(rev_file):
        cmd = "cat {}".format(rev_file)
        rev = subprocess.check_output(cmd, shell=True)
        return rev

    log_dir = "./log/"
    rev = None
    fn = "{}/revision.txt".format(log_dir)
    cmd = 'git rev-parse HEAD'
    with open(fn, 'w') as f:
        logger.info('running: {}'.format(cmd))
        rev = subprocess.check_output(cmd, shell=True)
        f.write(rev)
    return rev


def archive_results(name):
    log_dir = "./log/"
    scripts_dir = "./scripts/"
    archive_dir = "./archive/"
    log_file = os.path.join(log_dir, name + ".log")
    data_file = os.path.join(log_dir, name + ".yml")
    archive_file = os.path.join(archive_dir, name + ".tgz")
    
    try:
        logger.info("copy {} to {}".format(data_file, archive_dir))
        shutil.copy2(data_file, archive_dir)
    except IOError:
        traceback.print_exc()

    archive_cmd = os.path.join(scripts_dir, "archive.sh")
    cmd = [archive_cmd, name]
    logger.info("running: {}".format(" ".join(cmd)))
    res = subprocess.call(cmd)
    if res != 0:
        logger.info("Error {} while archiving.".format(res))


def scrape_data(name):
    log_dir = "./log/"
    scripts_dir = "./scripts/"
    log_file = os.path.join(log_dir, name + ".log")
    output_data_file = os.path.join(log_dir, name + ".yml")
    
    cmd = [os.path.join(scripts_dir, "extract_txn_info.py"),
           log_file, 
           output_data_file]

    logger.info("executing {}".format(' '.join(cmd)))
    res=subprocess.call([os.path.join(scripts_dir, "extract_txn_info.py"),
                         log_file, output_data_file])
    if res!=0:
        logger.error("Error scraping data!!")


def generate_graphs(args):
    if args.generate_graph:	
        restore_dir = os.getcwd()
        try:
            archive_dir = "./archive/"
            os.chdir(archive_dir)
            cmd = ['../scripts/make_graphs', "*.csv", ".", ".."] 
            res=subprocess.call(cmd)
            if res != 0:
                logger.error('Error generating graphs!!!')
        finally:
            os.chdir(restore_dir)
	

def aggregate_results(name):
    restore_dir = os.getcwd()
    try:
        archive_dir = "./archive/"
        cc = os.path.join(os.getcwd(), 'scripts/aggregate_run_output.py')
        cmd = [cc, '-p', name, '-r', save_git_revision()]
        os.chdir(archive_dir)
        cmd += glob.glob('*yml')

        res=subprocess.call(cmd)
        if res!=0:
            logger.error("Error aggregating data!!")
    finally:
        os.chdir(restore_dir)


def run_experiments(args):
    server_counts = itertools.chain.from_iterable([get_range(sr) for sr in args.server_counts])
    client_counts = itertools.chain.from_iterable([get_range(cr) for cr in args.client_counts])

    experiment_params = (server_counts,
                         client_counts,
                         args.modes,
                         args.benchmarks,
                         args.zipf,
                         args.client_loads)

    experiments = itertools.product(*experiment_params)
    for params in experiments:
        (num_server, num_client, mode, benchmark, zipf, client_load) = params
        experiment_suffix = gen_experiment_suffix(
            benchmark,
            mode,
            num_client,
            zipf,
            client_load)
        experiment_name = "{}-{}".format(
            args.experiment_name,
            experiment_suffix)

        logger.info("Experiment: {}".format(params))
        config_file = generate_config(
            args,
            experiment_name,
            benchmark, mode,
            zipf,
            client_load,
            num_client,
            num_server,
            args.num_replicas)
        try:
            save_git_revision()

            if args.trial_name == "crt_ratio":
                logger.info("config file {}".format(config_file))
                for ratio in CRT_RATIOS: 
                # for ratio in [30]:
                    data = None
                    with open(config_file, 'r') as f:
                        data = yaml.load(f)
                    data['bench']['tpcc_payment_crt_rate'] = ratio
                    with open(config_file, 'w') as f:
                        yaml.dump(data, f)

                    for i in range(N_REPEAT):
                        result = run_experiment(config_file, 
                                    experiment_name, 
                                    args, 
                                    benchmark, 
                                    mode, 
                                    num_client)
                        if result != 0:
                            logger.error("experiment returned {}".format(result))
            elif args.trial_name == "n_clients":
                ## concurrency = threads per client process
                for client_concurrency in CONCURRENCY_LEVELS:
                    data = None
                    with open(config_file, 'r') as f:
                        data = yaml.load(f)
                    data['n_concurrent'] = client_concurrency
                    with open(config_file, 'w') as f:
                        yaml.dump(data, f)

                    for i in range(N_REPEAT):
                        result = run_experiment(config_file, 
                                    experiment_name, 
                                    args, 
                                    benchmark, 
                                    mode, 
                                    num_client)
                        if result != 0:
                            logger.error("experiment returned {}".format(result))
            elif args.trial_name == "tpca":
                ## concurrency = threads per client process
                for zipf in zipfs:
                # for zipf in [0.5, 0.7, 1.0]:
                # for ratio in [1]:
                    data = None
                    with open(config_file, 'r') as f:
                        data = yaml.load(f)
                    data['bench']['coefficient'] = zipf
                    with open(config_file, 'w') as f:
                        yaml.dump(data, f)
                        
                    for i in range(N_REPEAT):
                        result = run_experiment(config_file, 
                                    experiment_name, 
                                    args, 
                                    benchmark, 
                                    mode, 
                                    num_client)
                        if result != 0:
                            logger.error("experiment returned {}".format(result))
            
            elif args.trial_name == "scalability":
                for i in range(N_REPEAT):
                    result = run_experiment(config_file, 
                                experiment_name, 
                                args, 
                                benchmark, 
                                mode, 
                                num_client)
                    if result != 0:
                        logger.error("experiment returned {}".format(result))
            
            elif args.trial_name == "robust":
                for i in range(N_REPEAT):
                    result = run_experiment(config_file, 
                                experiment_name, 
                                args, 
                                benchmark, 
                                mode, 
                                num_client)
                    if result != 0:
                        logger.error("experiment returned {}".format(result))
            else:
                result = run_experiment(config_file, 
                                    experiment_name, 
                                    args, 
                                    benchmark, 
                                    mode, 
                                    num_client)
                if result != 0:
                    logger.error("experiment returned {}".format(result))
 #           scrape_data(experiment_name)
 #           archive_results(experiment_name)
        except Exception:
            logger.info("Experiment %s failed.",
                        experiment_name)
            traceback.print_exc()
    
#    aggregate_results(experiment_name)
#    generate_graphs(args)
                   

def print_args(args):
    for k,v in args.__dict__.iteritems():
        logger.debug("%s = %s", k, v)

def main():
    logging.basicConfig(format="%(levelname)s : %(message)s")
    logger.setLevel(logging.DEBUG)
    args = parse_commandline()
    print_args(args)
    try:
        os.setpgrp()
        run_experiments(args)
    except Exception:
        traceback.print_exc()
    finally:
        os.killpg(0, signal.SIGTERM)

if __name__ == "__main__":
    main()

# Artifact for paper #121: Achieving Low Tail-latency and High Scalability for Serializable Transactions in Edge Computing

## Artifact summary
DAST is a distributed database designed for emerging edge computing applications
that desire serializability, low tail-latency, and horizontal scalability. 
DAST is developed based on a stable [commit](https://github.com/NYU-NEWS/janus/tree/3c1b60cd965551b4bd3b1f3c475e16fdde2e4c2b) of the open-source codebase of Janus, which is also used to 
evaluate the [Janus (OSDI '16)](https://www.usenix.org/conference/osdi16/technical-sessions/presentation/mu)
and the [ROCOCO (OSDI '14)](https://www.usenix.org/conference/osdi14/technical-sessions/presentation/mu) paper. 

This artifact contains the implementation of DAST, together with scripts 
for reproducing the main results of this work.

If you want to read the source code, *chronos* is equivalent to *dast*, and *brq* is equivalent 
to *Janus* due to legacy namings. 


## Artifact Check-list

+ Code link: [https://github.com/hku-systems/dast/](https://github.com/hku-systems/dast/)
+ OS Version: Ubuntu 16.04 or 18.04.
+ Docker version: >= 19.03.11.
+ Python version: >= 3.6.8.
+ Metrics: total throughput (txn/s); latency (ms)
+ Expected runtime: each trial of each data point takes about 2 minutes. 

## A very brief introduction of DAST
A typical DAST deployment contains many **regions**, with 
each region containing many clients and servers (nodes) connected with fast networks (e.g., 5G or LAN). 
By contrast, network communication across regions usually has much longer latency because it 
needs to go through the WAN for a long geographical distance. 
Each database **shard** (or **partition**) is assigned to
one region, and is replicated within the region for fault tolerance. 
DAST targets workloads with good spatial locality, where shards within a region are mostly 
accessed by intra-region clients, and most of a client's requests access only intra-region shards.
For instance, although not dedicated for edge computing, the well-known TPC-C benchmark manifests this 
pattern. 

In DAST, if a client transaction only accesses data within its region, the transaction is called an
**intra-region transaction** or an **IRT**; otherwise, the transaction is called a **cross-region transaction** or a **CRT**. 
Since intra-region network is fast, IRTs usually have much lower latency than CRTs, and achieving low client-perceived latency 
is a key motivation of moving the data from cloud to edge. Unfortunately, when a IRT conflict with on-going CRTs, the IRT may be blocked, resulting it a very high (tail-)latency.

DAST introduces a new timestamp-based distributed concurrency control protocol named "decentralized anticipation and stretch", 
which avoids an IRT from being blocked by on-going CRTs that are waiting for cross-region network communication in normal cases. 


## Deployment

### Selecting the experiment scale. 

DAST can be evaluated with R sub-regions and N shards per region, which will deploy R * N * 3 server processes in total (3 replicas per shard). 
Each process typically needs 2 cores. In the paper, we set R = 10 and N = 10 by default. 
You can downscale the 
experiment (e.g., R = 5 & N = 3, or even R = 3 and N = 2) to save resource and time, and *you will get similar results*: the throughput will be downgrades proportionally (by (10 / R) * (10 / N)), and the median-latency and tail-latency will be roughly the same as reported in the paper. Note that the baseline system SLOG must be run with R >= 3. 

Note that, as mentioned above, DAST focuses on reducing tail-latency caused by *blockings*, but not by resource contention. In real deployments, 
each client and server process should be run on different machines. However, in the evaluation, they have to share a small number of host machines.
If you find that the tail-latency is too-high, please downscale your experiment. Or, you can also change to use 95-percentile latency instead of 
99-percentile, which is more robust to resource contention. The tail-latency of baseline systems should still be much higher than DAST even using the 
95-percentile tail-latency. 

All settings can be found in `exp_config.py` with explanations in the comments. 
### Clone the codebase

      git clone git@github.com:hku-systems/dast.git
      
### Create the docker containers. 

Prerequisite: 
   1. You need to have multiple host machines with LAN networks.
   2. The machines should have NTP enabled. 
   3. The machines should be able to SSH each other without password. 
   4. Each host machine should have docker engine installed. 
   5. The user should be able to execute docker command without sudo privilege, which can be achieved by `sudo usermod -aG docker (username)`.
   6. The machines should join the same docker swarm cluster. If you haven't done so, you can follow [this Docker official guide](https://docs.docker.com/engine/swarm/swarm-tutorial/)

Create the overlay network with (assuming that `10.55.0.0/16` is available).
      
      docker network create -d overlay --attachable --subnet=10.55.0.0/16 dast_network
      
**=> (For artifact evaluation only) start from this step if you are using our cluster.**       

Then, please add the IP of your machines to the `hosts.yml` file. (Note the format, the little `-` is for identifying array items in yaml files). Also, please define your experiment scale at `exp_config.py`.

Run the following command to start the docker containers. This can be slow on the first run because it needs to pull images from the docker hub. 

      python start_containers.py

After the previous command finishes, you should have N * R containers deployed on your cluster. Each container should be assigned the address of 
`10.55.x.y`, where containers with the same `x` value are effectively in the same region with RTT of 5ms, and the RTT among regions is 100ms.  

Run the following command to create the deployment file. 

      python genConfig.py

For simplicity, server processes for the same shard are co-located in the same container. Note that the RTT within each container (the loopback device) is also 5ms. 

Start a *test container* and execute experiment scrips (see below) inside the container. 

      docker run -v [your_code_path]:/janus --cap-add NET_ADMIN --network dast_network --ip 10.55.0.240 -it hkusys/dast:latest

      # add ssh key to all containers 
      # inside the test container
      cd /janus
      python add_keys.py

      ## Then, you can execute the below experiments. 


## Experiments

### Experiment 1: End-to-end performance with different number of clients. 

This experiment runs DAST with the default TPC-C workload and changes the number of clients to saturate each system. 

+ Command to run:

      # inside the test container
      python exp1_n_clients.py 

+ Output: a pdf file named `results/data-n_clients.pdf` containing five figures for
  1. system throughput, 
  2. The median-latency of IRTs
  3. The tail-latency of IRTs
  4. The median-latency of CRTs 
  5. The tail-latency of CRTs. 

Raw output data are also in the `results` folder. You can manually generate the partial figure using the following command if the experiment script hangs (also works for the other four experiments).

      python -c 'import genFigure; genFigure.drawFig("n_clients")'
   
+ Paper claims and expected results. 
  1. Claim 1: DAST has lower tail-latency for IRTs because DAST avoids IRTs getting blocked. 

        Expected results: 
        * DAST's IRT tail-latency climbs from <20 ms; baseline systems' IRT tail-latency climbs from >100 ms. 
        * The numbers should be irrelevant to the experiment scale, as long as resource contention of the host is not significant. 
        * baseline systems' IRT tail-latency may be < 100 ms when the number of clients is very small (e.g., 1 client per shard) as the blocking cases may fall out of the 99-percentile.  

  2. Claim 2: DAST has lower tail-latency for CRTs.
   
        Expected results: 
        * DAST's CRT tail-latency climbs slowly from ~230ms; baseline systems' CRT tail-latency climbs faster.
        * The numbers should be irrelevant to the experiment scale.

  3. Claim 3: DAST has comparable throughput and median-latency as baseline systems. 
   
        Expected results: 
        * DAST's performance may be a little bit better because we kept optimizing DAST's code after paper submission. 
        * Note that the reason why DAST has lower tail-latency (claim 1 and claim 2) is not that DAST has better implementation with higher peak throughput. The tail-latency of baseline systems is high even when the systems are far from saturated because of *blockings*, which is mostly avoided in DAST. 


### Experiment 2: Performance with different ratio of transactions being CRTs. 

This experiment tests DAST using the TPC-C-Payment transaction (the one with value dependency) with different ratio of transactions being CRTs.  

+ Command to run: 
  
      # inside the test container
      python exp2_crt_ratio.py 

+ Output: a pdf file named `results/data-crt_ratio.pdf` containing five figures.
  
+ Paper claim: DAST maintains low (median and tail) latency for IRTs, but DAST's CRT latency may increase when the CRT ratio gets higher. This is a limitation of DAST, as DAST targets workloads with good spatial locality.   
  
+ Expected results and explanation: 
   * With the ratio of CRTs increasing, DAST's throughput decreases because CRTs' finish time is much longer than IRTs (the only variable is the ratio of CRTs; the number of clients is fixed).
   * The median-latency and tail-latency of IRTs remains low. 
   * The median-latency and tail-latency of CRTs may increase at first, but will drop then. Note that the increase may not manifest in some setups. The CRT latency is affected by two opposite factors: (1) a larger ratio of CRTs causes more "stretches", which may delay the execution of subsequent CRTs (but not IRTs), and (2) with the CRT ratio increasing, the total load of the system decreases, leading to lower latency.
  *  We did not make major claims on DAST v.s. baseline systems because this trial is to test DAST's limitation. The performance of baseline systems should be affected by the similar two factors as DAST. 

### Experiment 3: Performance with different conflict rates. 

This experiments test DAST use different conflict rates (`zipf` coefficient) in the TPC-A workload. 

+ Command to run:
  
      # inside the test container
      python exp3_tpca.py

+ Output: a pdf file named `results/data-tpca.pdf` containing five figures.

+ Paper claim: DAST's performance is relatively stable with different conflict rates. 

+ Expected results: 
    + DAST's lines are flat in the figures. This is because DAST uses timestamps to order all transactions accessing the same shard (i.e., SMR-based), regardless of whether they conflict.
    + We did not make major claims on DAST v.s. baseline systems; their experiments can be omitted. SLOG's performance should be stable, similar to DAST. The performance of Tapir and Janus drop with higher conflict rates, as they can enter fast path only if the conflict rate is not high (admitted in their paper). 

### Experiment 4: Scalability to the number of regions.

This experiment tests DAST's scalability to number of regions. 

+ Command to run: 
  
      # inside the test container
      python exp4_scalability.py 

+ Output: a pdf file named `results/data-scalability.pdf` containing five figures.

+ Paper claims and expected results:
    + DAST's throughput increases proportionally to the number of regions because the *total throughput is mainly contributed by IRTs in each region* (irrelevant to the number of regions).
    +  DAST's latency is roughly stable. 

+ Misc:
    + We used AWS to conducted the scalability evaluation (with up to 100 c5.24xlarge instances). We missed to report it in our submission version but have added it to our camera ready version. 
    + The Docker swam engine crashed twice when doing the experiment. 
    + With limited resource, you can re-construct the deployment with a smaller region size (e.g., 2 shards per region). The result should show the same trends.
  
### Experiment 5: DAST's robustness on unstable cross-region networks. 

This experiment changes the cross-region RTT from 100 ms to 100 ± X ms. 

+ Command to run:
         
      # inside the test container
      python exp5_robust.py 

+ Output: a pdf file named `results/data-robust.pdf` containing five figures.

+ Paper claims and expected results: 
    + DAST's IRT latency is roughly stable, and CRT latency increases roughly proportional to $X$. 
    + DAST's throughput slightly dropped (because CRT's latency increases but the number of clients is unchanged.)

+ Misc:
    + Some version of Linux kernel (e.g., `4.15.0-58-generic` for Ubuntu 18.04) has a bug on the Linux `tc` command for setting the cross region RTT to 100 ± X ms, see this [bug report](https://bugs.launchpad.net/ubuntu/+source/linux/+bug/1783822). 
    + If you see very poor performance (in most cases DAST even cannot start), please use a different version (e.g., Ubuntu 16.04 is OK) or simply skip this trial. 


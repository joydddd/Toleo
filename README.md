# SniperSim Simulator for Toleo 
This is a sniper sim and DRAMSim based simulator for Toleo (ASPLOS '24) Please refer to our paper "Toleo: Scaling Freshness to Tera-scale Memory
Using CXL and PIM" for details. 

# Getting Started
Running evaluation for Toleo requires: 
1. Install DRAMSim3 from this [fork](https://github.com/joydddd/DRAMsim3). 
2. Install simulator sniper-toleo. (this repo) 
3. setup benchmarks with PIN hooks. Forks with PIN hooks can be found in this [list](https://github.com/stars/joydddd/lists/toleo)
4. Download simulation [script](https://raw.githubusercontent.com/joydddd/VNserver_spec/main/run_toleo_sim.py) `run_toleo_sim.py` for automated simulation on given benchmarks. 


## File Structure
```
toleo_root
├── run_toleo_sim.py
├── DRAMsim3
├── sniper-toleo
│   ├── DRAMsim3 -> ../DRAMsim3 (link)
│   ├── run-sniper
│   └── README.md (this file)
├── genomicsbench
│   ├── input-datasets
│   │    ├── bsw
│   │    ├── chain
│   │    ...
│   └── benchmarks
│      ├── fmi
│      │  └── sim-<date-time>/<region>/sim.out
│      ├── bsw-s
│      ...
├── gabps
│   └── run
│      ├── pr-kron-s
│      │  └── sim-<date-time>/<region>/sim.out
│      ├── sssp
│      ....
├── redis
├── memtier_benchmark
├── memcached
├── hyrise
└── llama2.c
```

Make TOLEO_ROOT directory
```
mkdir toleo_root
cd toleo_root
```

## Install DRAMsim3
```
git clone https://github.com/joydddd/DRAMsim3
cd DRAMsim3
```
Please follow the README from this [fork](https://github.com/joydddd/DRAMsim3) to build DRAMsim3. THERMAL is not required for toleo simulation. 
Run DRAMsim3 test to make sure it is successfully installed:
```
 ./build/dramsim3main configs/DDR4_8Gb_x8_3200.ini -c 100000 -t tests/example.trace
cat dramsim3.json
```

## Install sniper-toleo
### Prerequisite
- A DRAMsim3 installation from this [fork](https://github.com/joydddd/DRAMsim3), and link to sniper-toleo repo.
```
## in folder toleo_root. 
git clone git@github.com:joydddd/sniper-toleo.git # clone sniper-toleo (this repo)cd sniper-toleo
ln -s ../DRAMsim3 DRAMsim3
```

### (optional) Setup docker environment
```
cd sniper-toleo/docker
make   # build docker image
make run # run docker as user
# make run-root # runs docker as root.
cd .. 
```
Now you're in the docker container. Continue to build sniper-toloe following the steps below. You could skip the dependent package installation. 

### Install SniperSim
Please follow the naive install instructions on Sniper Sim [Getting Started Page](https://snipersim.org/w/Getting_Started) to install sniper-toleo. Necessary steps are provided below

1. Install dependent packages.
```
sudo dpkg --add-architecture i386
sudo apt-get install binutils build-essential curl git libboost-dev libbz2-dev libc6:i386 libncurses5:i386 libsqlite3-dev libstdc++6:i386 python wget zlib1g-dev
```

> [!NOTE]
> Know issue: snipersim assum python2 as the default python version. Make sure your python command points to python2.7.

2. build simulator with PIN
```
make USE_PIN=1 -j N #where N is the number of cores in your machine to use parallel make
```


### Test Run
```
cd test/fft
make
```
This command runs sniper-toleo simulation for three setups: Toleo, C+I, and no-protections. Simulation results can be found in folders `test/fft/no_protection_output` `test/fft/ci_output` `test/fft/toleo_output` respectively. 

### Run Your Own Benchmark
Now you've successfully setup sniper-toleo and is ready to simluate toleo for any benchmark of your choice! run the simulation with 
```
./run-sniper -n 32 -c <arch> -d <output-dir>  -- <run benchmakr cmd>
# <arch> = zen4_cxl: baseline no protection, 
#          zen4_vn:  toleo
#          zen4_no_freshness: CI
#          zen4_cxl_invisimem: InvisiMem (Please use invisimem branch of sniper-toleo for this architecture!) 
#          zen4_no_dramsim: light weighted Toleo without DRAMSim3. Used for Toleo page type analysis. 
```
More controls over the simulation are available via `./run-sniper --help`. 
> [!NOTE]
> Our simulator is configured for 32 cores. Please setup your benchmark to run with 32 threads for best experience. Benchmarks with >32 threads might run into deadlock problems.

# Citing Toleo
If you use Toleo in your research, please cite: 
```
@misc{dong2024toleoscalingfreshnessterascale, title={Toleo: Scaling Freshness to Tera-scale Memory using CXL and PIM}, author={Juechu Dong and Jonah Rosenblum and Satish Narayanasamy}, year={2024}, eprint={2410.12749}, archivePrefix={arXiv}, primaryClass={cs.AR}, url={https://arxiv.org/abs/2410.12749}, }
```

# Additional Information

### Benchmarks Used in Evalutions 
We provided PIN hooks inserted benchmarks used in our evaluations in this [list](https://github.com/stars/joydddd/lists/toleo). Please refer to [benchmark setup instruction](BenchSetup.md). 
### Evaluation Workflow
We provided a batch run [script](https://raw.githubusercontent.com/joydddd/VNserver_spec/main/run_toleo_sim.py) to run our evaluation workflows. Please refer to [Evalution Workflow Instructions](evaluation_workflow.md).





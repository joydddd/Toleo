# Evaluation Workflow
In this section we provide instructions on how to use our batch run script to test the benchmark setups, test the instrumentation setups, and finally run simluations in batches. 
Running all the simulations on one machine of >32 cores takes **~20 days**.  We highly recommend you start with our toy bench suite `sim_test` for quick evaluation. (`bsw-s` from genomicsbench and `pr-kron-s` from gapbs, see [benchmark setup instruction](BenchSetup.md)) 

### Prerequisite
- A working installation of sniper-toleo that run `test/fft` successfully.
- Benchmarks that are cloned and built following [benchmark setup instruction]


## Simulation Automation Script
We provide a batch run [script](https://raw.githubusercontent.com/joydddd/VNserver_spec/main/run_toleo_sim.py) that includes instrumentation details of our selected benchmark suite. 

Download batch run script  `run_toleo_sim.py` 
```
# in toleo_root
wget https://github.com/joydddd/VNserver_spec/raw/main/run_toleo_sim.py
chmod +x run_toleo_sim.py
./run_toleo_sim.py --help
```
Run `run_toleo_sim.py --help` to learn how to use the script. `./run_toleo_sim.py print-bench` to print all benchmarks supported by this script. 

## Simulation Workflow

### Native Run
First, we want to run the benchmarks natively to make sure they are setup properly. 


Take genomicsbench as an exmaple:  

```
./run_toleo_sim.py native --bench genomicsbench # runs all tests in genomicsbench
./run_toleo_sim.py native --bench bsw-s # runs only bsw-s
```
The benchmark prints `PIN_START` and `PIN_END` in the terminal when it enters and leaves the region of interest (ROI). This indicates that PIN_HOOKs are properly inserted and our instrumentation tool can detect when the benchmark enters ROI. 

It also reports basic information about the benchmark, e.g. CPU time, peak RSS size, Page Faults etc. 
<details>
<summary>Example output for bsw-s native run </summary>
 
```
toleo_root$ ./run_toleo_sim.py native --bench bsw-s
******************* Running native bsw-s************************
/usr/bin/time -v ./bsw -pairs ../../input-datasets/bsw/small/bandedSWA_SRR7733443_100k_input.txt  -t 32 -b 512 2>&1 | tee /home/joydong/toleo/genomicsbench/benchmarks/bsw-s/native-run/sniper.out
PIN START
PIN END
Number of input pairs: 100000
Allocating 0.221 GB memory for input buffers...
4] workTicks = 51504554
29] workTicks = 53017524
23] workTicks = 56130408
16] workTicks = 53817462
30] workTicks = 52364988
17] workTicks = 56771468
12] workTicks = 52361226
26] workTicks = 54032656
18] workTicks = 55996382
6] workTicks = 54965176
8] workTicks = 53517756
7] workTicks = 55016286
21] workTicks = 57274246
19] workTicks = 52326987
5] workTicks = 51139868
9] workTicks = 60749688
24] workTicks = 50829902
1] workTicks = 51238592
3] workTicks = 54227330
11] workTicks = 52844928
27] workTicks = 51141502
15] workTicks = 52152112
31] workTicks = 55819150
25] workTicks = 52839380
13] workTicks = 54880854
2] workTicks = 52383152
20] workTicks = 54900462
10] workTicks = 54577728
22] workTicks = 54673222
28] workTicks = 60828196
14] workTicks = 51987192
0] workTicks = 51599402
Executed AVX2 vector code...
Processor freq: 3801.83 MHz
Read time = 0.07 s
Overall SW cycles = 63471058, 0.02 s
Total Pairs processed: 100000
avgTicks = 53997180.593750, maxTicks = 60828196, load imbalance = 1.126507
        Command being timed: "./bsw -pairs ../../input-datasets/bsw/small/bandedSWA_SRR7733443_100k_input.txt -t 32 -b 512"
        User time (seconds): 0.59
        System time (seconds): 0.12
        Percent of CPU this job got: 64%
        Elapsed (wall clock) time (h:mm:ss or m:ss): 0:01.12
        Average shared text size (kbytes): 0
        Average unshared data size (kbytes): 0
        Average stack size (kbytes): 0
        Average total size (kbytes): 0
        Maximum resident set size (kbytes): 251032
        Average resident set size (kbytes): 0
        Major (requiring I/O) page faults: 0
        Minor (reclaiming a frame) page faults: 64055
        Voluntary context switches: 537
        Involuntary context switches: 78
        Swaps: 0
        File system inputs: 32
        File system outputs: 0
        Socket messages sent: 0
        Socket messages received: 0
        Signals delivered: 0
        Page size (bytes): 4096
        Exit status: 0
Arch: zen4_cxl
```


</details>


### (optional) Instrumented Run
We then test if the benchmarks are properly instrumented by pinplay tools. 

> TODO: Add instructions for installing Intel SDE tools. 

```
./run_toleo_sim.py region --bench bsw-s # run bsw-s benchmark
```
The program will print how many instructions are executed at warmup start, sim start, and sim end in the terminal. More control information can be found in folder `<path_to_benchmark>/region`. 


### Run simulation 
Now that we have verified our benchmarks are running properly natively and with pinplay tools, we can simulate them with the sniper-toleo simluation on different arhictectural configurations. 
In particular, we provide configurations to simulation our 4 evaluation setups (no-protection, CI, toleo & invisimem) and generate performance metrics (runtime, cache hit rates, memory latency, and memory bandwidth utilization etc. ). We also provide a light-weighted no_dramsim configuration to run long simulations and generate Trip format stats. 

Each Simluation takes hours - days, therefore we suggest starting with the `sim_test` suite (`bsw-s` from genomicsbench, and `pr-kron-s` from gapbs. Both should simulate in less than 30 minutes).

**No protection. baseline of performance overheads** 

Simulate no memory protection
```
./run_toleo_sim.py sniper --bench bsw-s --arch zen4_cxl -a
```

**Toleo**

Simulate Toleo
```
./run_toleo_sim.py sniper --bench bsw-s --arch zen4_vn -a
```


**CI**

Simulate memory with confidentiality and integrity protection and no freshness protection. (CI) 
```
./run_toleo_sim.py sniper --bench bsw-s --arch zen4_no_freshness -a
```

> [!TIP]
> You can batch run many benchmarks on multiple architectural configurations. 
> ```
> ./run_toleo_sim.py sniper --bench bsw-s pr-kron-s --arch zen4_vn zen4_cxl zen4_no_freshness -a
> ```


     
**InvisiMem**

> [!NOTE]
> Use sniper-toleo from branch `invisimem` to run invisiMem baseline.
```
cd sniper-toleo
git checkout invisimem
make USE_PIN=1
cd ..
toleo_sim.py sniper --bench bsw-s --arch zen4_cxl_invisimem -a
```


**NoDramsim**
This is a light weighted configuration for running long simulations of the Trip format. This configuration runs without DRAMsim3 simulator and doesn't generate accurate performance metrics. However, its Trip format stats are accurate. 
```
./run_toleo_sim.py sniper --bench bsw-s --arch zen4_no_dramsim -a
```

### Simulation Results
Simulation results can be found in each benchmark binary location, under a fold named `sim-<date:time>-<arch>/<region>`. For example, `toleo_root/genomicsbench/bsw-s/sim-2024-06-05_04:51:28-zen4_vn/r2-t32` is the result path for `bsw-s` simulation. In this output directory, you'll find performance stats in `sim.out` and Trip format stats in `dram_trace_analysis.csv`. 

`dram_trace_analysis.csv` contains information about how many pages the program accessed and in what format it is stored on Toleo. Toleo usage can thus be calcualted from this csv form. 
```
icount	page-touched	page-readonly	page-one-write	page-flat	page-uneven	page-full
44530000127	39305	34741	4408	0	1	0
44850000180	191858	332	60759	68	130699	0
45171000105	251162	447	118536	163	132016	0
...
```

`sim.out` should look like this, where runtime, cpi, memory utilization etc. are reported. 
```
                                                  | Core 0     | Core 1     | Core 2     | Core 3     | Core 4     | Core 5     | Core 6     | Core 7     | Core 8     | Core 9     | Core 10    | Core 11    | Core 12    | Core 13    | Core 14    | Core 15    | Core 16    | Core 17    | Core 18    | Core 19    | Core 20    | Core 21    | Core 22    | Core 23    | Core 24    | Core 25    | Core 26    | Core 27    | Core 28    | Core 29    | Core 30    | Core 31   
  Instructions                                     |  109266686 |  108849656 |   92564201 |   99072226 |  103035533 |  107353037 |   85906176 |  103790533 |   98014786 |   99456484 |  115422069 |   96762534 |   83362768 |   88163988 |  101808633 |   96233594 |  110709183 |  102766956 |   98685011 |   91283689 |  103053788 |  108931426 |  103265275 |   95872132 |   87286372 |   94015000 |   95230511 |  103867843 |  113258292 |   98421677 |  102472843 |  101745615
  Cycles                                           |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980 |   61332980
  IPC                                              |       1.78 |       1.77 |       1.51 |       1.62 |       1.68 |       1.75 |       1.40 |       1.69 |       1.60 |       1.62 |       1.88 |       1.58 |       1.36 |       1.44 |       1.66 |       1.57 |       1.81 |       1.68 |       1.61 |       1.49 |       1.68 |       1.78 |       1.68 |       1.56 |       1.42 |       1.53 |       1.55 |       1.69 |       1.85 |       1.60 |       1.67 |       1.66
  Time (ns)                                        |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103 |   27259103
  Idle time (ns)                                   |      52811 |      54565 |      67964 |      57233 |      53431 |      59237 |     102098 |      52499 |     102181 |     101712 |      99856 |     101095 |     103235 |      97330 |     101672 |      97657 |      80764 |      82569 |      96886 |      84574 |      82246 |      88764 |      78245 |      83921 |      79705 |      77935 |      84552 |      73045 |      81067 |      76557 |      77169 |      79684
  Idle time (%)                                    |       0.2% |       0.2% |       0.2% |       0.2% |       0.2% |       0.2% |       0.4% |       0.2% |       0.4% |       0.4% |       0.4% |       0.4% |       0.4% |       0.4% |       0.4% |       0.4% |       0.3% |       0.3% |       0.4% |       0.3% |       0.3% |       0.3% |       0.3% |       0.3% |       0.3% |       0.3% |       0.3% |       0.3% |       0.3% |       0.3% |       0.3% |       0.3%
Branch predictor stats                             |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
  num correct                                      |   13938385 |   13885263 |   12310485 |   12556080 |   13136702 |   14716822 |   11202135 |   13605526 |   13200398 |   13439327 |   16132631 |   12604782 |   10923674 |   11361680 |   13709881 |   12869228 |   14140934 |   13111333 |   13168336 |   12057347 |   13388859 |   13918960 |   13195975 |   12518140 |   11435684 |   12410725 |   12381455 |   13265084 |   15843643 |   12796319 |   13073974 |   13002156
  num incorrect                                    |     240298 |     228235 |     513204 |     340367 |     273712 |     485060 |     537003 |     324249 |     490061 |     500099 |     393429 |     409690 |     530739 |     577473 |     522709 |     491114 |     244013 |     254945 |     519113 |     459945 |     296244 |     249436 |     275508 |     497439 |     514754 |     486459 |     476991 |     261250 |     450886 |     382152 |     271342 |     269344
  misprediction rate                               |      1.69% |      1.62% |      4.00% |      2.64% |      2.04% |      3.19% |      4.57% |      2.33% |      3.58% |      3.59% |      2.38% |      3.15% |      4.63% |      4.84% |      3.67% |      3.68% |      1.70% |      1.91% |      3.79% |      3.67% |      2.16% |      1.76% |      2.05% |      3.82% |      4.31% |      3.77% |      3.71% |      1.93% |      2.77% |      2.90% |      2.03% |      2.03%
  mpki                                             |       2.20 |       2.10 |       5.54 |       3.44 |       2.66 |       4.52 |       6.25 |       3.12 |       5.00 |       5.03 |       3.41 |       4.23 |       6.37 |       6.55 |       5.13 |       5.10 |       2.20 |       2.48 |       5.26 |       5.04 |       2.87 |       2.29 |       2.67 |       5.19 |       5.90 |       5.17 |       5.01 |       2.52 |       3.98 |       3.88 |       2.65 |       2.65
TLB Summary                                        |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
  I-TLB                                            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
    num accesses                                   |   12929550 |   12813273 |   11827229 |   11809516 |   12234895 |   13619542 |   11129511 |   12556181 |   12452989 |   12592471 |   14412243 |   12016918 |   10861293 |   11478933 |   12931143 |   12304336 |   13141893 |   12219703 |   12590351 |   11499298 |   12420782 |   12933125 |   12319481 |   12363281 |   11104030 |   11983967 |   12182019 |   12370495 |   14325967 |   12122601 |   12158589 |   12149599
    num misses                                     |         38 |         37 |         38 |         37 |         37 |         38 |         38 |         37 |         38 |         38 |         38 |         82 |         82 |         38 |         38 |         38 |         38 |         37 |         38 |         82 |         82 |         37 |         37 |         38 |         82 |         82 |         38 |         38 |         38 |         82 |         37 |         37
    miss rate                                      |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00%
    mpki                                           |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00
  D-TLB                                            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
    num accesses                                   |   37394002 |   37620212 |   30744226 |   32357108 |   34750341 |   38184092 |   26156606 |   35721889 |   33456694 |   34060114 |   42777123 |   30206566 |   23475952 |   25644353 |   34281850 |   32549247 |   37897365 |   35077436 |   33259000 |   29078759 |   34288843 |   37135321 |   34942682 |   32968116 |   26517760 |   30008987 |   32975733 |   35231240 |   40629279 |   31294821 |   34745322 |   34569558
    num misses                                     |    1631986 |    1723248 |      70898 |    1093966 |    1472525 |     277796 |       2140 |    1038344 |      27475 |     155851 |     925841 |     736131 |      11329 |       2305 |      70401 |     109403 |    1640794 |    1251962 |      36429 |     231831 |    1258763 |    1682752 |    1380278 |       1614 |      45037 |     231063 |       1146 |    1552065 |     663337 |     821023 |    1394027 |    1082809
    miss rate                                      |      4.36% |      4.58% |      0.23% |      3.38% |      4.24% |      0.73% |      0.01% |      2.91% |      0.08% |      0.46% |      2.16% |      2.44% |      0.05% |      0.01% |      0.21% |      0.34% |      4.33% |      3.57% |      0.11% |      0.80% |      3.67% |      4.53% |      3.95% |      0.00% |      0.17% |      0.77% |      0.00% |      4.41% |      1.63% |      2.62% |      4.01% |      3.13%
    mpki                                           |      14.94 |      15.83 |       0.77 |      11.04 |      14.29 |       2.59 |       0.02 |      10.00 |       0.28 |       1.57 |       8.02 |       7.61 |       0.14 |       0.03 |       0.69 |       1.14 |      14.82 |      12.18 |       0.37 |       2.54 |      12.21 |      15.45 |      13.37 |       0.02 |       0.52 |       2.46 |       0.01 |      14.94 |       5.86 |       8.34 |      13.60 |      10.64
  L2 TLB                                           |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
    num accesses                                   |    1631962 |    1723241 |      70936 |    1093934 |    1472456 |     277834 |       2178 |    1038381 |      27513 |     155889 |     925879 |     736213 |      11411 |       2343 |      70439 |     109441 |    1640769 |    1251918 |      36467 |     231856 |    1258775 |    1682789 |    1380268 |       1652 |      45024 |     231084 |       1184 |    1552059 |     663338 |     821073 |    1394029 |    1082822
    num misses                                     |     271812 |     244301 |        495 |     112361 |     161281 |        489 |        458 |      61709 |        457 |        559 |     164307 |      71369 |       2091 |        435 |        601 |        547 |     227015 |      91658 |        711 |      36974 |     143123 |     309246 |      54463 |        249 |       5261 |      24157 |        135 |     269741 |       1635 |      97392 |     131397 |      26803
    miss rate                                      |     16.66% |     14.18% |      0.70% |     10.27% |     10.95% |      0.18% |     21.03% |      5.94% |      1.66% |      0.36% |     17.75% |      9.69% |     18.32% |     18.57% |      0.85% |      0.50% |     13.84% |      7.32% |      1.95% |     15.95% |     11.37% |     18.38% |      3.95% |     15.07% |     11.68% |     10.45% |     11.40% |     17.38% |      0.25% |     11.86% |      9.43% |      2.48%
    mpki                                           |       2.49 |       2.24 |       0.01 |       1.13 |       1.57 |       0.00 |       0.01 |       0.59 |       0.00 |       0.01 |       1.42 |       0.74 |       0.03 |       0.00 |       0.01 |       0.01 |       2.05 |       0.89 |       0.01 |       0.41 |       1.39 |       2.84 |       0.53 |       0.00 |       0.06 |       0.26 |       0.00 |       2.60 |       0.01 |       0.99 |       1.28 |       0.26
Cache Summary                                      |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
  Cache L1-I                                       |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
    num cache accesses                             |   12929550 |   12813273 |   11827229 |   11809516 |   12234895 |   13619542 |   11129511 |   12556181 |   12452989 |   12592471 |   14412243 |   12016918 |   10861293 |   11478933 |   12931143 |   12304336 |   13141893 |   12219703 |   12590351 |   11499298 |   12420782 |   12933125 |   12319481 |   12363281 |   11104030 |   11983967 |   12182019 |   12370495 |   14325967 |   12122601 |   12158589 |   12149599
    num cache misses                               |       2123 |       1907 |       1847 |       2763 |       2276 |       1843 |       1563 |       2340 |       1938 |       2189 |       2130 |       3405 |       2497 |       1689 |       2260 |       1942 |       2027 |       2423 |       2141 |       3746 |       3905 |       2307 |       2462 |       1187 |       2914 |       3342 |        995 |       2111 |       2429 |       3069 |       2371 |       2812
    miss rate                                      |      0.02% |      0.01% |      0.02% |      0.02% |      0.02% |      0.01% |      0.01% |      0.02% |      0.02% |      0.02% |      0.01% |      0.03% |      0.02% |      0.01% |      0.02% |      0.02% |      0.02% |      0.02% |      0.02% |      0.03% |      0.03% |      0.02% |      0.02% |      0.01% |      0.03% |      0.03% |      0.01% |      0.02% |      0.02% |      0.03% |      0.02% |      0.02%
    mpki                                           |       0.02 |       0.02 |       0.02 |       0.03 |       0.02 |       0.02 |       0.02 |       0.02 |       0.02 |       0.02 |       0.02 |       0.04 |       0.03 |       0.02 |       0.02 |       0.02 |       0.02 |       0.02 |       0.02 |       0.04 |       0.04 |       0.02 |       0.02 |       0.01 |       0.03 |       0.04 |       0.01 |       0.02 |       0.02 |       0.03 |       0.02 |       0.03
  Cache L1-D                                       |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
    num cache accesses                             |   37393197 |   37619718 |   30743138 |   32356417 |   34749382 |   38183593 |   26156606 |   35721277 |   33456694 |   34060114 |   42777123 |   30206566 |   23475952 |   25643499 |   34281850 |   32548146 |   37896398 |   35076384 |   33258319 |   29078084 |   34288033 |   37134478 |   34941793 |   32967601 |   26516742 |   30008275 |   32975009 |   35230167 |   40627864 |   31294047 |   34744420 |   34568830
    num cache misses                               |     633901 |     722423 |     216493 |     296050 |     469723 |     183216 |     246158 |     223686 |     203261 |     210112 |     168266 |     346257 |     285295 |     265052 |     229246 |     220276 |     680291 |     218466 |     254598 |     391743 |     474390 |     737576 |     225254 |     223937 |     307705 |     339105 |     208148 |     568510 |     192828 |     345634 |     268675 |     189972
    miss rate                                      |      1.70% |      1.92% |      0.70% |      0.91% |      1.35% |      0.48% |      0.94% |      0.63% |      0.61% |      0.62% |      0.39% |      1.15% |      1.22% |      1.03% |      0.67% |      0.68% |      1.80% |      0.62% |      0.77% |      1.35% |      1.38% |      1.99% |      0.64% |      0.68% |      1.16% |      1.13% |      0.63% |      1.61% |      0.47% |      1.10% |      0.77% |      0.55%
    mpki                                           |       5.80 |       6.64 |       2.34 |       2.99 |       4.56 |       1.71 |       2.87 |       2.16 |       2.07 |       2.11 |       1.46 |       3.58 |       3.42 |       3.01 |       2.25 |       2.29 |       6.14 |       2.13 |       2.58 |       4.29 |       4.60 |       6.77 |       2.18 |       2.34 |       3.53 |       3.61 |       2.19 |       5.47 |       1.70 |       3.51 |       2.62 |       1.87
  Cache L2                                         |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
    num cache accesses                             |     676536 |     771922 |     219237 |     333514 |     514522 |     185340 |     248900 |     259226 |     205858 |     213189 |     177023 |     373465 |     294822 |     267023 |     231755 |     223073 |     720259 |     268379 |     256973 |     407475 |     518550 |     778077 |     273816 |     225556 |     318615 |     354176 |     210053 |     611377 |     195618 |     379480 |     318930 |     244182
    num cache misses                               |      78619 |      89631 |      43201 |      92672 |      90202 |      27327 |      41190 |      81153 |      39703 |      44513 |      36220 |      90353 |      59431 |      27680 |      36146 |      44165 |      80310 |     101862 |      47605 |     101536 |     136137 |      82152 |      96789 |      21538 |      72673 |      69269 |      25719 |      88233 |      44581 |      87383 |      97297 |     107508
    miss rate                                      |     11.62% |     11.61% |     19.71% |     27.79% |     17.53% |     14.74% |     16.55% |     31.31% |     19.29% |     20.88% |     20.46% |     24.19% |     20.16% |     10.37% |     15.60% |     19.80% |     11.15% |     37.95% |     18.53% |     24.92% |     26.25% |     10.56% |     35.35% |      9.55% |     22.81% |     19.56% |     12.24% |     14.43% |     22.79% |     23.03% |     30.51% |     44.03%
    mpki                                           |       0.72 |       0.82 |       0.47 |       0.94 |       0.88 |       0.25 |       0.48 |       0.78 |       0.41 |       0.45 |       0.31 |       0.93 |       0.71 |       0.31 |       0.36 |       0.46 |       0.73 |       0.99 |       0.48 |       1.11 |       1.32 |       0.75 |       0.94 |       0.22 |       0.83 |       0.74 |       0.27 |       0.85 |       0.39 |       0.89 |       0.95 |       1.06
  Cache L3                                         |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
    num cache accesses                             |     120894 |     138402 |      49232 |     129889 |     134560 |      28399 |      47175 |     117110 |      43443 |      48648 |      43068 |     116045 |      71641 |      28939 |      36796 |      49518 |     118653 |     150838 |      52677 |     119589 |     178102 |     121142 |     144974 |      23127 |      86046 |      82556 |      31380 |     132027 |      48008 |     119392 |     146308 |     161055
    num cache misses                               |      72328 |      84828 |      42202 |      86818 |      85312 |      26430 |      40546 |      76181 |      38405 |      43138 |      34579 |      80392 |      53253 |      26087 |      34237 |      43236 |      75323 |      93849 |      44457 |      90819 |     119489 |      74663 |      90903 |      21235 |      66887 |      62892 |      25370 |      81933 |      43269 |      77352 |      92118 |      99281
    miss rate                                      |     59.83% |     61.29% |     85.72% |     66.84% |     63.40% |     93.07% |     85.95% |     65.05% |     88.40% |     88.67% |     80.29% |     69.28% |     74.33% |     90.14% |     93.05% |     87.31% |     63.48% |     62.22% |     84.40% |     75.94% |     67.09% |     61.63% |     62.70% |     91.82% |     77.73% |     76.18% |     80.85% |     62.06% |     90.13% |     64.79% |     62.96% |     61.64%
    mpki                                           |       0.66 |       0.78 |       0.46 |       0.88 |       0.83 |       0.25 |       0.47 |       0.73 |       0.39 |       0.43 |       0.30 |       0.83 |       0.64 |       0.30 |       0.34 |       0.45 |       0.68 |       0.91 |       0.45 |       0.99 |       1.16 |       0.69 |       0.88 |       0.22 |       0.77 |       0.67 |       0.27 |       0.79 |       0.38 |       0.79 |       0.90 |       0.98
Page Tabel summary                                 |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
  num pages [cxl, dram]                            |       5647 |      33524 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0
  usage (GB) [cxl, dram]                           |     0.0215 |     0.1279 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000 |     0.0000
DRAM effective Access                              |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
  num data reads                                   |    1733408 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0
  num data writes                                  |    1035260 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0
  num mac accesses                                 |    1534569 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0
  average data read latency                        |     157.93 |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf
  average data bandwidth (GB/s)                    |       6.50 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00
  DRAM latency breakdown                           |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
    data fetch                                     |     133.67 |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf
    mac fetch                                      |      11.76 |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf
    decrypt                                        |      12.50 |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf
DRAM summary                                       |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
  num dram accesses                                |    4303236 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0
  average dram access latency (ns)                 |     148.25 |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf
  average dram read latency (ns)                   |     140.32 |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf
  average dram backpressure delay                  |      23.12 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00
  average dram queue utilization                   |     14.97% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00%
  average dram bandwidth (GB/s)                    |      10.10 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00
CXL effective Access                               |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
  num data reads                                   |     292253 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0
  avg read latency                                 |     228.36 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00
  num data writes                                  |     172552 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0
  num mac accesses                                 |     112580 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0
  data bandwidth (GB/s)                            |       1.09 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00
  CXL summary                                      |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
    num cxl accesses                               |     577385 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0
    average cxl access latency (ns)                |     210.83 |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf
    average cxl queueing delay                     |      66.09 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00
    average cxl bandwidth utilization              |     10.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00%
    average cxl bandwidth (GB/s)                   |       1.36 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00
  CXL DRAM summary                                 |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
    num cxl dram accesses                          |     577385 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0
    average dram access latency(CXL expander) (ns) |      44.70 |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf
    average cxl dram backpressure delay            |       0.13 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00
    average cxl dram bandwidth utilization         |      1.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00%
    average cxl dram bandwidth (GB/s)              |       1.36 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00 |       0.00
MEE summary                                        |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
  num mee crypto                                   |    2768668 |     464804 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0
  average mee crypto latency (ns)                  |      12.50 |      12.50 |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf
  average mee crypto queue latency (ns)            |       0.00 |       0.00 |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf |        inf
  average mee bandwidth utilization                |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00% |      0.00%
  MEE MAC Cache                                    |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
    num mac access                                 |    2768668 |     464804 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0
    num mac misses                                 |     933952 |      80410 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0
    mac cache miss rate                            |     33.73% |     17.30% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf% |       inf%
  MEE VN Table                                     |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
    num vn access                                  |    1733408 |     292253 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0
Coherency Traffic                                  |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |            |           
  num loads from dram                              |      48029 |      56962 |       9657 |      45079 |      51623 |       3839 |       9758 |      41414 |       7253 |       8680 |      10862 |      31248 |      17788 |       2726 |       3128 |       8829 |      45979 |      56229 |       4674 |      23228 |      48864 |      46643 |      55191 |       3834 |      19339 |      18238 |       9665 |      52961 |       8194 |      39374 |      55515 |      62472
  num loads from dram cache                        |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          0
  num loads from remote cache                      |          0 |          0 |          0 |          0 |          0 |         11 |          0 |          1 |          0 |          0 |          0 |          0 |          0 |          0 |          0 |          1 |          0 |          0 |          0 |          1 |          0 |          0 |          0 |          0 |         62 |          2 |          0 |          0 |          0 |          0 |          0 |          0
```

# Benchmarks

Benchmarks were run inside the Ubuntu 22.04 VM with ROCm 5.7.1 and the soft
PowerPlay service set to:

```text
POWER_W=130
PROFILE=COMPUTE
PERF_LEVEL=high
```

## Public Baselines

AMD's published MI25 reference values:

```text
FP32 peak:             12.29 TFLOPS
Memory bandwidth peak: 484 GB/s
TDP:                   300 W peak
Compute units:         64
```

Geekbench 6 OpenCL public chart value:

```text
Radeon Instinct MI25: 68,562
```

The card tested here is not exposing the full MI25 profile:

```text
Radeon PRO V320 profile
56 compute units
Default PowerPlay cap: 110 W
```

## Geekbench 6.7.1 OpenCL

Result:

```text
OpenCL Score: 53,585
Result URL: https://browser.geekbench.com/v6/compute/6407022
```

Comparison:

```text
53585 / 68562 = 78.2% of Geekbench's public Radeon Instinct MI25 OpenCL chart value
```

Telemetry during the run:

```text
Max power:     129 W
Max GPU busy:  96%
Max junction:  57 C
Max memory:    54 C
```

Geekbench is bursty; it did not thermally stress the card like the sustained
FP32 loop.

## HIP Memory Bench

Vectorized `float4` memory benchmark:

```text
copy4:  380.45 GB/s best observed
add4:   ~340 GB/s
triad4: ~340 GB/s
```

Compared with the `484 GB/s` MI25 HBM2 reference peak:

```text
380.45 / 484 = 78.6% of theoretical peak
```

## HIP FP32 Loop

Custom FP32 FMA loop:

```text
Best observed: ~8.9 TFLOP/s
```

This is below the full MI25 `12.29 TFLOPS` spec, but the tested card exposes
`56 CU`, not `64 CU`, and is running a lower-power V320 profile.

Approximate 56-CU theoretical peak at 1500 MHz:

```text
56 CU * 64 lanes/CU * 2 FLOP/FMA * 1500 MHz = 10.75 TFLOP/s
```

Observed:

```text
8.9 / 10.75 = ~82.8% of exposed 56-CU theoretical peak
```

## Power Cap Sweep

Short sustained compute sweep:

```text
WATTS  AVG_SCLK  MIN_SCLK  MAX_SCLK  AVG_POWER  MAX_JUNC  MAX_MEM  AVG_BUSY  FP32
110    1161 MHz  991 MHz   1500 MHz  85 W       66 C      55 C     66%       7.03
120    1137 MHz  991 MHz   1500 MHz  98 W       69 C      58 C     73%       7.14
130    1258 MHz  991 MHz   1500 MHz  90 W       71 C      59 C     54%       7.42
140    1228 MHz  991 MHz   1500 MHz  101 W      74 C      60 C     65%       7.77
150    1243 MHz  1138 MHz  1500 MHz  120 W      76 C      62 C     73%       8.17
```

Interpretation:

```text
Best thermals:       110 W
Best average SCLK:   130 W
Best raw FP32:       150 W
Best balanced pick:  130 W
```

## Notes

The higher `150 W` cap works at runtime, but it does not automatically produce
the best sustained-clock behavior. `130 W` looked cleaner as a balanced setting
for this card/cooling/VM setup.

## HIP Conway's Game of Life

Example source:

```text
examples/hip-game-of-life.cpp
```

Small correctness/preview run:

```text
Grid: 96x48
Steps: 64
Init: glider
Alive cells after run: 5
```

Large benchmark run:

```text
Grid: 8192x8192
Steps: 10,000
Cells updated: 671,088,640,000
Kernel time: 19,973.598 ms
Throughput: 33.60 billion cell-updates/sec
Alive cells after run: 1,936,727 (2.89%)
```

Telemetry during the long run at the persistent `130 W` setting:

```text
Power:       127-130 W
GPU busy:    99-100%
Max edge:    54 C during sampled load
Max junction:65 C during sampled load
Max memory:  58 C during sampled load
SCLK:        991 MHz during sustained load
MCLK:        500 MHz during sustained load
```

This workload is memory-neighborhood heavy and sustained the configured power
cap while keeping temperatures reasonable.

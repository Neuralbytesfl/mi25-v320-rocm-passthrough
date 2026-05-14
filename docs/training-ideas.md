# Training Ideas for This MI25/V320 Setup

This card can train models, but not in the same frictionless way people train
CUDA-first projects on modern NVIDIA RTX cards. The hardware is useful; the main
constraint is software support for older ROCm `gfx900` / Vega GPUs.

## Practical Targets

Good targets:

| Model type | Fit | Notes |
| --- | ---: | --- |
| Small neural networks | Good | Easy first target |
| CNN image classifiers | Good | MNIST, CIFAR, small custom datasets |
| Small transformers | Possible | Framework support is the hard part |
| Tiny language models | Possible | Character-level or small token models |
| Cellular automata models | Very good | Directly matches the Game of Life work |
| Physics/grid simulation ML | Very good | Stencil/grid workloads fit the hardware |
| Reinforcement learning batch simulation | Good | GPU can accelerate synthetic environments |
| FP64/math-heavy experiments | Niche-good | MI25 has stronger FP64 than many consumer GPUs |

Poor first targets:

| Model type | Issue |
| --- | --- |
| Modern LLM training | Too slow and too much software friction |
| Large diffusion models | Memory and framework support constraints |
| CUDA-first projects | NVIDIA-only assumptions |
| Modern PyTorch ROCm | `gfx900` support is awkward/deprecated |
| Tensor Core workloads | MI25 does not have NVIDIA Tensor Cores |

## Hardware Strengths

Observed setup:

```text
16 GiB HBM2
gfx900 / Vega 10
56 compute units exposed
~380 GB/s best observed HIP memory copy
~8.9 TFLOP/s best observed custom FP32 loop
~32-34 billion Game of Life cell-updates/sec on large grids
```

The software stack is the limiting factor:

```text
ROCm support for gfx900 is old and fragile.
Many current AI projects assume CUDA.
Modern ROCm/PyTorch wheels may not support this GPU cleanly.
```

## Best Project Direction

The strongest project is not general LLM training. It is:

```text
synthetic-data generation + learned cellular automata / physics-style models
```

The GPU is already proven to generate Game of Life ground truth at large scale.
This makes it a good platform for training learned simulators.

## Example Projects

### 1. Learn Conway's Life Rule

Train a tiny model:

```text
Input:  3x3 neighborhood
Output: next cell state
```

This is a correctness demo, but it is almost too easy because Conway's Life is a
fixed local rule.

### 2. Predict Multiple Steps Ahead

Harder and more interesting:

```text
Input:  current 64x64 patch
Target: state 4, 8, or 16 generations later
```

This forces the model to learn motion, collisions, oscillators, and extinction.

### 3. Classify Pattern Behavior

Given a starting patch, predict:

```text
dies out
stabilizes
oscillates
expands
moves/glider-like
chaotic
```

This turns the cellular automata simulator into a synthetic labeling engine.

### 4. Neural Cellular Automata

Train a model that behaves like a cellular automaton or learns its own local
update rule. This is a strong demo because the data generation is exact,
repeatable, and cheap.

## Data Generation Rate

Measured large-grid Game of Life throughput:

```text
~33.6 billion cell-updates/sec
```

That is:

```text
33.6e9 * 3600 = ~121 trillion cell-updates/hour
```

Do not store all generated states raw. Stream batches.

Example storage math:

```text
1024 x 1024 board = 1,048,576 cells
1 byte per cell   = ~1 MiB per board
input+target pair = ~2 MiB per sample
1 million pairs   = ~2 TiB
```

The better architecture is:

```text
GPU simulates large boards
extract random patches
feed training batches
discard or sparsely checkpoint generated states
```

## Suggested Toolchains

Start with:

```text
HIP/OpenCL simulator + simple custom trainer
HIP/OpenCL simulator + CPU trainer for first prototypes
tinygrad experiments
older ROCm/PyTorch stack only if gfx900 support works cleanly
```

Avoid starting with a modern CUDA-first training project. The first milestone
should be a small model that trains end-to-end reliably on this hardware.

## Bottom Line

This MI25/V320 setup is most interesting as a cheap synthetic-data generator and
grid-simulation accelerator. It can train models, but the highest-confidence
path is learned cellular automata and physics-style experiments rather than
large general AI models.

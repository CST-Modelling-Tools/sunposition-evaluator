# sunposition-evaluator

This repository evaluates the PSA+ (Updated PSA) sun position algorithm against the real  
`MICA_psa_2020_2050_MB` dataset.

It provides:

- A scientific regression test anchored to published PSA+ (2020) results  
- A repeatable performance benchmark on a real dataset  
- A cross-platform evaluator CLI  
- Optional OpenMP parallelization (~5× speedup) with safe fallback  

---

## Repository Goals

This repository is designed to support:

- Scientific validation of the PSA+ model  
- Performance optimization workflows (including GOW integration)  
- Reproducible benchmarking across platforms  
- Safe optimization with regression guardrails  

---

## Fixture

The committed fixture is located in:

basic_test/

Contents:

- sun_position_evaluator_spec.json
- default_psa_plus_input.json
- data/MICA_psa_2020_2050_MB.csv

The binary dataset:

MICA_psa_2020_2050_MB.bin

is not committed.

Instead, it is automatically generated from the CSV when needed by:

- regression tests
- benchmark
- evaluator CLI

---

## Scientific Baseline

The regression test uses PSA+ Table 2 values from:

Blanco et al. (2020) – Updating the PSA sun position algorithm

### Target values (arcseconds)

| Metric | Average | Std Dev | Mean Dev |
|------|--------|--------|----------|
| Azimuth | 0.18 | 11.80 | 8.38 |
| Zenith | 0.05 | 6.96 | 5.40 |
| Sun vector | 8.78 | 5.40 | 4.36 |

### Tolerances

Defined in:

tests/sun_position_regression_test_main.cpp

Chosen to:

- detect real scientific regressions
- tolerate rounding in the paper and cross-platform floating-point differences

---

## Build

### Common options

-DSUNPOSITION_BUILD_BENCHMARK=ON|OFF
-DSUNPOSITION_ENABLE_OPENMP=ON|OFF
-DBUILD_TESTING=ON|OFF

---

### Windows

#### Visual Studio

cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release

#### Ninja

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

---

### Linux

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

---

### macOS

Default (no OpenMP):

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

Optional (OpenMP via LLVM):

brew install llvm

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build

---

## Install

cmake --install build --prefix out/install

Windows multi-config:

cmake --install build --config Release --prefix out/install

---

## Run Regression Test

Using CTest:

ctest --test-dir build --output-on-failure

Direct execution:

./build/sun_position_regression_test --fixture-dir basic_test

Windows:

.\build\Release\sun_position_regression_test.exe --fixture-dir basic_test

---

## Run Benchmark

Default:

./build/sun_position_benchmark --fixture-dir basic_test

With repetitions:

./build/sun_position_benchmark --fixture-dir basic_test --repetitions 100

Output includes:

- total wall time
- average time per evaluation
- best time
- OpenMP status

---

## Benchmark Interpretation

Recommended usage:

- Quick test: --repetitions 10
- Reliable comparison: --repetitions 100

Always compare using:

- same machine
- same build config
- same dataset
- same repetition count

Primary metric:

average_time_per_evaluation_ms

---

## Performance (Important)

Sequential (baseline): ~155 ms per evaluation

OpenMP enabled: ~31 ms per evaluation

~5× speedup on typical multi-core systems

---

## OpenMP

Behavior:

- Enabled automatically if available
- Disabled automatically if not
- Never required to build

Runtime indicator:

openmp_enabled: true|false

Thread control:

OMP_NUM_THREADS=8

HPC / GOW usage:

- Many evaluator processes → OMP_NUM_THREADS=1
- Few evaluator processes → OMP_NUM_THREADS > 1

---

## Evaluator CLI

sun_position_evaluator --input <input.json> --output <output.json> [--spec <spec.json>]

Spec resolution order:

1. --spec
2. next to input file
3. current working directory
4. basic_test/

---

## Dataset

- CSV is committed
- binary is generated locally
- ensures reproducibility and portability

---

## Cross-Platform Notes

- Windows: fully supported (MSVC, Ninja)
- Linux: fully supported (GCC/Clang)
- macOS:
  - works without OpenMP
  - OpenMP requires LLVM toolchain

---

## Development Workflow

Before any optimization:

1. Run regression test
2. Run benchmark (>=100 repetitions)

After any change:

1. Regression must pass
2. Benchmark must improve or remain neutral

---

## Summary

This repository provides:

- a scientifically validated evaluator
- a robust performance benchmark
- a cross-platform build
- a parallelized implementation with fallback

It is suitable for:

- optimization workflows (GOW)
- HPC environments
- reproducible scientific benchmarking
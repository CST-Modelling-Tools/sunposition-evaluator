# sunposition-evaluator

This repository evaluates the current PSA+ parameter set against the real
`MICA_psa_2020_2050_MB` fixture and now includes two guardrails before any speed
refactor:

- a correctness regression test anchored to the PSA+ 2020 paper values
- a repeatable timing benchmark on the same full fixture dataset

## Fixture

The committed fixture lives under `basic_test/`:

- `basic_test/sun_position_evaluator_spec.json`
- `basic_test/default_psa_plus_input.json`
- `basic_test/data/MICA_psa_2020_2050_MB.csv`

The generated binary dataset is intentionally not committed. The regression test
and benchmark generate `basic_test/data/MICA_psa_2020_2050_MB.bin`
automatically from the committed CSV whenever the binary is missing.

## Scientific baseline

The regression test uses the PSA+ Table 2 values from the 2020 paper
“Updating the PSA sun position algorithm” as the scientific baseline.

Target values used by the test:

- Azimuth error (arcsec): average `0.18`, standard deviation `11.80`,
  mean deviation `8.38`
- Zenith error (arcsec): average `0.05`, standard deviation `6.96`,
  mean deviation `5.40`
- Sun vector error (arcsec): average `8.78`, standard deviation `5.40`,
  mean deviation `4.36`

The tolerances are documented in
`tests/sun_position_regression_test_main.cpp` and are chosen to be:

- tight enough to catch real scientific regressions
- loose enough to tolerate the paper's rounded values and tiny
  cross-platform floating-point differences

## Build

The repository builds from source on Windows, Linux, and macOS with standard
CMake workflows. Normal builds keep executables in the build tree; installation
uses a standard CMake install prefix rather than writing back into the source
tree.

The main CMake options are:

- `-DSUNPOSITION_BUILD_BENCHMARK=ON|OFF`
- `-DSUNPOSITION_ENABLE_OPENMP=ON|OFF`
- `-DBUILD_TESTING=ON|OFF`

OpenMP is optional. CMake prints one of these configure messages:

- `OpenMP support enabled via OpenMP::OpenMP_CXX`
- `OpenMP support not found; building with sequential fallback`
- `OpenMP support disabled by SUNPOSITION_ENABLE_OPENMP=OFF`

### Windows

Visual Studio generator:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

Ninja or other single-config generator:

```powershell
cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-ninja
```

### Linux

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### macOS

Build without OpenMP works out of the box with Apple Clang:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

If you want OpenMP on macOS, use a toolchain that provides it, typically LLVM
installed via Homebrew:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

If `find_package(OpenMP)` still does not detect OpenMP with Apple Clang, the
repository still builds and runs correctly in sequential mode.

## Install

Use a standard install prefix on every platform:

```powershell
cmake --install build --config Release --prefix out/install
```

On single-config generators:

```bash
cmake --install build --prefix out/install
```

## Run The Regression Test

Using CTest:

```powershell
ctest --test-dir build --output-on-failure --build-config Release
```

Running the executable directly also works:

```powershell
.\build\Release\sun_position_regression_test.exe --fixture-dir basic_test
```

Single-config example:

```bash
./build/sun_position_regression_test --fixture-dir basic_test
```

## Run The Benchmark

The benchmark measures full evaluator executions on the real fixture with the
default PSA+ parameter set.

Default run:

```powershell
.\build\Release\sun_position_benchmark.exe --fixture-dir basic_test
```

Custom repetition count:

```powershell
.\build\Release\sun_position_benchmark.exe --fixture-dir basic_test --repetitions 10
```

Single-config example:

```bash
./build/sun_position_benchmark --fixture-dir basic_test --repetitions 10
```

The benchmark performs one warm-up evaluation and then reports:

- total wall time
- average time per evaluation
- best time
- repetition count

The default repetition count is `5` because each repetition already evaluates
the full `815256`-sample dataset, which keeps the timing baseline practical
while still averaging out a meaningful amount of noise.

## Benchmark Interpretation

Recommended repetition counts:

- `--repetitions 10` for quick local checks while iterating
- `--repetitions 100` for before/after comparisons that are stable enough to
  use as a performance baseline

Suggested comparison workflow:

```powershell
.\out\install\sun_position_benchmark.exe --fixture-dir basic_test --repetitions 100
```

When comparing two versions, keep these conditions the same:

- same machine
- same build configuration
- same compiler and generator
- same fixture dataset
- same repetition count

For optimization work, prefer comparing `average_time_per_evaluation_ms`.
`best_time_ms` is still useful as a lower-noise reference point, but average
time is the more reliable regression gate.

## Evaluator CLI

The evaluator accepts:

```text
sun_position_evaluator --input <input.json> --output <output.json> [--spec <sun_position_evaluator_spec.json>]
```

If `--spec` is omitted, the executable looks for
`sun_position_evaluator_spec.json` in this order:

1. next to the input JSON
2. in the current working directory
3. in the repository's `basic_test/` directory

This removes the previous requirement to run the evaluator only from a specific
working directory.

## OpenMP Notes

- OpenMP is enabled target-wise only when `find_package(OpenMP)` succeeds.
- The repository never requires OpenMP to build.
- If OpenMP is unavailable, the evaluator, regression test, benchmark, and
  dataset builder all continue to work with the sequential evaluator path.
- The benchmark prints `openmp_enabled: true|false` so the active mode is easy
  to confirm at runtime.
- The evaluator implementation keeps the same scientific calculations in both
  modes; OpenMP only parallelizes the per-sample evaluation loop.

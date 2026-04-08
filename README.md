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

Configure and build with CMake:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Normal builds now keep executables and companion files in the build tree rather
than in the repository root `bin/` directory.

If you use a single-config generator such as Ninja or Unix Makefiles, configure
the build tree as Release:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Install Release Executables

To copy only the release `.exe` files into `bin/`:

```powershell
cmake --install build --config Release
```

There is also a convenience target:

```powershell
cmake --build build --target install_release --config Release
```

For single-config generators, use:

```powershell
cmake --build build --target install_release
```

This keeps `bin/` as a clean install location for release programs instead of a
general build artifact directory.

## Run The Regression Test

Using CTest:

```powershell
ctest --test-dir build --output-on-failure --build-config Release
```

Running the executable directly from the repository root also works:

```powershell
.\build\sun_position_regression_test.exe --fixture-dir basic_test
```

## Run The Benchmark

The benchmark measures full evaluator executions on the real fixture with the
default PSA+ parameter set.

Default run:

```powershell
.\build\sun_position_benchmark.exe --fixture-dir basic_test
```

Custom repetition count:

```powershell
.\build\sun_position_benchmark.exe --fixture-dir basic_test --repetitions 10
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
.\bin\sun_position_benchmark.exe --fixture-dir basic_test --repetitions 100
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

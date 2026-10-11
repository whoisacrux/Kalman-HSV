# Numerical verification — v1.1

Date: 2026-10-11. Environment: Linux x86_64, GCC 13.3, C++17.

Both test executables were compiled and run directly with warning checks enabled. These are actual numerical test results, not GUI screenshots or hardware measurements.

```bash
g++ -std=c++17 -Wall -Wextra -Werror -pedantic tests/kalman_tests.cpp -Isrc -o kalman_tests
./kalman_tests
g++ -std=c++17 -Wall -Wextra -Werror -pedantic tests/motion_tests.cpp -Isrc -o motion_tests
./motion_tests
```

## Scalar filter

Initialization, constant signal, reset, nonnegative finite covariance, invalid input and constant-signal noise reduction: **PASS**.

Filtered/raw sum-of-squared-error ratio after the warm-up interval: **0.0195569**. This test uses a constant signal, standard deviation 2, Q=0.01 and R=4; the result does not describe the temperature demo's entire varying signal.

## Synthetic motion

Seed 42; independent Gaussian coordinate noise, sigma=12 simulation units; 600 updates at 1/30 s; measurement variance 144; acceleration spectral density 450. The table includes every observed update, with no missing measurements or discarded warm-up. Both methods use the same observation frames.

| Generated path | Raw 2D position RMSE | Filtered 2D position RMSE |
|---|---:|---:|
| Curved | 18.0058 | 6.26211 |
| Straight | 18.0058 | 5.97078 |
| Circle | 18.0058 | 6.74435 |

All values are in simulation units. These are current-position errors, not future-prediction errors. Identical raw RMSE follows from reusing the same generated noise sequence for each trajectory. `std::normal_distribution` can produce different numerical sequences between C++ standard-library implementations, so Windows numbers need not be identical to this table.

Separate behavior checks: **PASS**.

- A manual dropout lasts exactly 60 updates; no measurement correction occurs during this interval.
- Missing frames do not enter the paired raw/filtered RMSE calculation.
- Model uncertainty grows during the gap and falls after the first returned observation.
- Reset reproduces the same trajectory estimates within the same runtime.
- States stay finite and uncertainty stays positive on all three paths.
- Trail length stays at or below 150 samples.
- The experiment stops after 600 updates and further calls do not advance time.

## Scope of verification

These checks exercise the numerical code, which does not depend on OpenCV. The updated desktop GUI and Windows launcher were not executed in the packaging environment. A full CMake/OpenCV build is defined in the supplied Linux CI workflow but was not run here. Run `START_WINDOWS.cmd -Motion` on the local Windows machine before claiming verification of the new GUI.

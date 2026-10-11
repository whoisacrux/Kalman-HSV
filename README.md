# HSV and Kalman Lab

A C++/OpenCV desktop project for experimenting with color segmentation and Kalman filtering. There are three views: a webcam color mask, a temperature filtering example, and a 2D motion simulation.

The camera is used for the HSV view. Both Kalman examples run on generated data, so they can be tested without a camera.

## What it does

- **HSV color mask:** shows the original camera image next to a binary mask. Hue, saturation and brightness limits can be adjusted while the program runs. A cleanup setting removes small specks in the mask. There is also a color-chart demo for testing without a webcam.
- **Temperature filtering:** adds noise to a generated temperature signal, then plots the measurements and Kalman estimate alongside the original signal. Q and R can be adjusted to see how the result changes. The displayed samples can be exported as CSV.
- **2D motion:** generates curved, straight and circular paths. The display shows the true position, noisy measurement, filtered position and a forecast 0.4 seconds ahead. Measurements can be interrupted for two seconds to see how the estimate behaves without new data.

## Build and run

Requirements: **C++17, OpenCV 4 and CMake**. The Windows launcher uses **Visual Studio 2022 Build Tools** with the C++ desktop workload.

On Windows, open PowerShell in the project folder and run:

```powershell
.\START_WINDOWS.cmd -Motion
```

The script builds the application, runs the numerical tests and opens the 2D view. It checks `C:\opencv\build` by default and asks for the OpenCV configuration folder if needed.

Other startup options:

```powershell
# HSV demo without a camera
.\START_WINDOWS.cmd -Demo

# HSV view with camera index 1
.\START_WINDOWS.cmd -Camera 1

# Temperature simulation
.\START_WINDOWS.cmd -Simulation
```

After building, the executable is in `build-windows\Release`. Its OpenCV DLLs need to stay in the same folder.

For Linux commands and setup details, see [installation instructions](docs/INSTALL_TR.md). A [short Turkish guide](HIZLI_BASLANGIC_TR.md) is also included.

## Controls

Click the main image window before using keyboard shortcuts.

| Key | Action |
|---|---|
| `1` / `2` / `3` | Switch views |
| `Space` | Pause or resume a simulation |
| `R` | Restart the current simulation |
| `L` | Interrupt measurements for two seconds in the 2D view |
| `P` | Change the 2D path and restart |
| `E` | Save the visible temperature samples to `simulation.csv` |
| `N` / `O` | Try the next camera / reconnect in the HSV view |
| `D` / `M` | Toggle the color-chart demo / mirror the camera image |
| `Esc` / `Q` | Exit |

The 2D view also has clickable buttons. The sliders in **Lab Controls** belong to the HSV and temperature views; the motion demo uses fixed parameters.

## Reading the motion view

The filled circle is the true position. The thin box shows the noisy measurement, while the green box and trail show the filtered position. The cross marks the forecast; the ellipse shows its model-based uncertainty.

The filter estimates position and velocity separately on each axis. Between measurements it assumes constant velocity. Curved paths make it easier to see where that assumption starts to fall short.

Each run lasts 20 simulated seconds, with 30 updates per second. Measurement noise has a standard deviation of 12 simulation units per axis. The same random seed is used after each reset so runs can be compared within the same environment.

Pressing **L** stops measurement updates temporarily. The estimate continues from the motion model and its uncertainty grows. Correction resumes when measurements return. The ellipse uses two standard deviations along each axis; it is not a guaranteed error bound.

Raw and filtered RMSE are calculated over the same frames with available measurements. These values measure the current position error. The forecast is displayed separately and is not included in that RMSE comparison.

## Tests

The numerical tests cover filter initialization, repeatable resets, noise reduction, missing measurements, recovery after a gap, and bounded history. They can be built without OpenCV:

```bash
cmake -S . -B build-tests -DBUILD_LAB=OFF
cmake --build build-tests --config Release
ctest --test-dir build-tests -C Release --output-on-failure
```

Both test programs passed with GCC 13.3 on Linux. The measured results and verification scope are recorded in [TEST_RESULTS.md](docs/TEST_RESULTS.md). A GitHub Actions workflow is included for building the OpenCV application and running the tests on Linux.

## Code layout

- [`src/main.cpp`](src/main.cpp): windows, controls, webcam processing and drawing.
- [`src/kalman.hpp`](src/kalman.hpp): scalar filter used by the temperature example.
- [`src/motion_demo.hpp`](src/motion_demo.hpp): generated motion, state estimation and forecast.
- [`tests/`](tests/): numerical tests for both examples.
- [`scripts/windows.ps1`](scripts/windows.ps1): Windows build and startup script.

## References

- [OpenCV HSV thresholding tutorial](https://docs.opencv.org/4.12.0/da/d97/tutorial_threshold_inRange.html)
- [OpenCV Kalman filter documentation](https://docs.opencv.org/4.11.0/dd/d6a/classcv_1_1KalmanFilter.html)

The small filters in this project are implemented directly in C++ so their prediction and correction steps can be read alongside the simulation code.

## License

[MIT](LICENSE)

![alt text](image.png)
![alt text](image-1.png)
![alt text](image-2.png)

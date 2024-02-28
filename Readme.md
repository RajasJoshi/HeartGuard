# HeartGuard Lite

![](Images/logo/GithubBanner.png)

## An affordable solution to Heart Diagnostics

Cardiovascular diseases are the leading cause of death globally, especially in low and middle-income countries. Current heart monitoring methods are limited by high costs and accessibility challenges.
The HealthGuard Lite aims to develop a cost-effective, accessible heart monitoring solution using Raspberry Pi.​

## Hardware

Requiring only a Raspberry Pi, Electrocardiogram (ECG) electrodes, a Heart Rate Monitor and an Analogue to Digital Converter (ADC). The HeartGuard Lite provides realtime diognostics on heart rate variability, aswell as warnings of potential long term health risks.

## Key Features

- Real Time Heart Rate Variability Measurement
- Responsive Health Risk Suggestions
- Sensor Fusion to get the most accurate results

## Requirements

* a modern C++17 compiler (`gcc-8`, `clang-6.0`, `MSVC 2017` or above)
* [`cmake`](https://cmake.org) 3.15+
* [`conan`](https://conan.io) 2.0+ (optional)
* `cppcheck` (optional)
* `clang-format` (optional)

## Available CMake Options

* BUILD_TESTING     - builds the tests (requires `doctest`)
* BUILD_SHARED_LIBS - enables or disables the generation of shared libraries
* BUILD_WITH_MT     - valid only for MSVC, builds libraries as MultiThreaded DLL

If you activate the `BUILD_TESTING` flag, you need to perform in advance a `conan install` step, just to fetch the `doctest` dependency. Another dependency (OpenSSL) is used in this project as a demonstration of including a third-party library in the process, but it is totally optional and you can activate it only if you run conan in advance.

## How to build from command line

The project can be built using the following commands:

```shell
cd HeartGuard/Software/Firmware/
mkdir -p build # md build (on Windows)
cd build
cmake -DBUILD_TESTING=FALSE -DBUILD_SHARED_LIBS=FALSE -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cmake --build . --target package
```

## Acknowledgements

This project uses the [modern C++ project template](https://github.com/madduci/moderncpp-project-template) by [madduci](https://github.com/madduci).
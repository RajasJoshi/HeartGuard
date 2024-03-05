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
* `libpigpio-dev`

## Available CMake Options

* BUILD_SHARED_LIBS - enables or disables the generation of shared libraries

## How to build from command line

The project can be built using the following commands:

```shell
cd HeartGuard/Software/Firmware/
mkdir -p build # md build (on Windows)
cd build
cmake -DBUILD_SHARED_LIBS=FALSE -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cmake --build . --target package
```

## Acknowledgements

This project uses the [modern C++ project template](https://github.com/madduci/moderncpp-project-template) by [madduci](https://github.com/madduci).

This project also uses code from the [rpi_ads1115](https://github.com/berndporr/rpi_ads1115/tree/main?tab=readme-ov-file) repository by [berndporr](https://github.com/berndporr).
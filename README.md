# Tileio Demos

This repository contains a collection of demos to demonstrate the capabilities of [Tileio](https://github.com/AmbiqAI/tileio) along with showcasing several of Ambiq's AI Development Kits (ADKs).

## Available Demos

### [**HeartKit: ECG AI Heart Analysis**](./heartkit/README.md)

This demo performs advanced real-time, heart analysis using a multi-head neural network running on Ambiq’s ultra low-power SoC.

**Tileio Dashboard Configuration:** <a href="./heartkit/assets/hk-dashboard-config.json" download>hk-dashboard-config.json</a>

### [**TFLM Inference Egnine**](./tflm-engine/README.md)

This demo configures the EVB to run as a TFLM inference engine that serves requests over USB using RPC.

## Getting Started

To get started with the demos, you will need to perform the following steps in order to compile and flash the firmware to the target device.

### 1. Install Dependencies

First, you will need to clone the repository and its submodules.

```bash
git clone --recurse-submodules https://github.com/AmbiqAI/tileio-demos.git
```

Next, intall the following tools and ensure they are available in your PATH. Please refer to [neuralSPOT](https://ambiqai.github.io/neuralSPOT/) for more information.

* [Arm GNU Toolchain ^12.2](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
* [Segger J-Link ^7.92](https://www.segger.com/downloads/jlink/)

### 2. Compile and Flash Demo

For the target demo, you'll need to navigate to the respective directory and compile and flash the firmware to the evaluation board (EVB). Below is an example to compile and flash the HeartKit demo.

```bash
cd heartkit
make PLATFORM=apollo4p_blue_kxr clean
make PLATFORM=apollo4p_blue_kxr
make PLATFORM=apollo4p_blue_kxr deploy
```

### 3. Launch Tileio App

Launch the Tileio App using the iOS app or the [web app](https://ambiqai.github.io/tileio/). The first time you launch the app, you will need to create a new dashboard and either select the respective built-in dashboard or upload the latest dashboard configuration file. Once the dashboard is created, select the dashboard and connect to the EVB via BLE or USB. Please refer to [Tileio Documentation](https://ambiqai.github.io/tileio-docs/) to learn more about using the Tileio App.

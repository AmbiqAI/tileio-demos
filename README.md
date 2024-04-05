# Tileio Demos

This repository contains a collection of demos to demonstrate the capabilities of [Tileio](https://github.com/AmbiqAI/tileio) along with showcasing several of Ambiq's AI Development Kits (ADKs).

## Demo Zoo

| Name       | Target | Device Config  | Dashboard Config | FW |
| ---------- | ------ | -------------- | ----------------- | -- |
| HeartKit: ECG Segmentation | Apollo4 Blue | [device.json](./README.md) | [dashboard.json](./README.md) | [firmware.bin](./README.md) |
| HeartKit: ECG Denoise | Apollo4 Blue | [device.json](./README.md) | [dashboard.json](./README.md) | [firmware.bin](./README.md) |

## Demo Descriptions

### HeartKit: ECG Segmentation

This demo performs real-time ECG and PPG segmentation using HeartKit AI models along with PhysioKit framework. The diagram below shows the data flow from the sensor to the AI model and the results are streamed to the PhysioKit Pro app via BLE.

```mermaid
flowchart LR
    COLL[MAX86150 \nSensor] --> PRE[Filter &\nDownsample]
    PRE --> ECG_COL[Preprocess]
    PRE --> PPG_COL[Preprocess]

    subgraph ECG Stream
    ECG_COL --> ECG_SEG[Segment]
    ECG_SEG --> ECG_MET[Metrics]
    ECG_SEG --> ECG_BLE_SLOT[BLE: Signals]
    ECG_MET --> ECG_BLE_MET[BLE: Metrics]

    end

    subgraph PPG Stream
    PPG_COL --> PPG_SEG[Segment]
    PPG_SEG --> PPG_MET[Metrics]
    PPG_SEG --> PPG_BLE_SLOT[BLE: Signals]
    PPG_MET --> PPG_BLE_MET[BLE: Metrics]
    end
```

### HeartKit: ECG Denoise

This demo showcases real-time ECG denoising using AI. The data is collected from the MAX86150 sensor and processed using the HeartKit AI model.

```mermaid
flowchart LR
    COLL[MAX86150 \nSensor] --> PRE[Filter &\nDownsample]
    PRE --> ECG_COL[Preprocess]

    subgraph ECG Stream
    ECG_COL --> ECG_DENOISE[Denoise]
    ECG_DENOISE --> ECG_BLE_SLOT[BLE: Signals]
    end
```

### Air Quality Monitoring

This demo showcases real-time air quality monitoring using the Ambiq's Apollo3 Blue ADK. The data is collected from the BME680 sensor and processed using the AirKit AI model.

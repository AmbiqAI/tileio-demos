# HeartKit:ECG AI Heart Analysis

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

## Downloads

- [**Device Configuration**](../assets/device-configs/hk-device-config.json)
- [**Dashboard Configuration**](../assets/dashboard-configs/hk-dashboard-config.json)

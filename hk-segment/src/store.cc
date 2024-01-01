#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// neuralSPOT
#include "arm_math.h"
#include "ns_ble.h"
// PhysioKit
#include "physiokit/pk_hrv.h"
#include "physiokit/pk_ecg.h"
#include "physiokit/pk_ppg.h"
// Local
#include "constants.h"
#include "segmentation.h"
#include "segmentation_flatbuffer.h"
#include "store.h"

///////////////////////////////////////////////////////////////////////////////
// EVB Configuration
///////////////////////////////////////////////////////////////////////////////

const ns_power_config_t nsPwrCfg = {
    .api = &ns_power_V1_0_0,
    .eAIPowerMode = NS_MAXIMUM_PERF,
    .bNeedAudAdc = false,
    .bNeedSharedSRAM = true,
    .bNeedCrypto = true,
    .bNeedBluetooth = true,
    .bNeedUSB = true,
    .bNeedIOM = true,
    .bNeedAlternativeUART = false,
    .b128kTCM = false,
    .bEnableTempCo = false,
    .bNeedITM = true,
};

pk_uio_state_t uioState = {
    .btn0 = false,
    .btn1 = false,
    .btn2 = false,
    .led0 = false,
    .led1 = false,
    .led2 = false,
    .rsv0 = false,
    .rsv1 = false,
};

static int volatile btn0Pressed = false;
static int volatile btn1Pressed = false;
ns_button_config_t nsBtnCfg = {
    .api = &ns_button_V1_0_0,
    .button_0_enable = true,
    .button_1_enable = true,
    .button_0_flag = &btn0Pressed,
    .button_1_flag = &btn1Pressed
};

ns_core_config_t nsCoreCfg = {
    .api = &ns_core_V1_0_0
};

ns_i2c_config_t nsI2cCfg = {
    .api = &ns_i2c_V1_0_0,
    .iom = I2C_IOM
};


///////////////////////////////////////////////////////////////////////////////
// Sensor Configuration
///////////////////////////////////////////////////////////////////////////////

static inline int
max86150_write_read(uint16_t addr, const void *write_buf, size_t num_write, void *read_buf, size_t num_read) {
    return ns_i2c_write_read(&nsI2cCfg, addr, write_buf, num_write, read_buf, num_read);
}
static inline int
max86150_read(const void *buf, uint32_t num_bytes, uint16_t addr) {
    return ns_i2c_read(&nsI2cCfg, buf, num_bytes, addr);
}
static inline int
max86150_write(const void *buf, uint32_t num_bytes, uint16_t addr) {
    return ns_i2c_write(&nsI2cCfg, buf, num_bytes, addr);
}

static max86150_context_t maxCtx = {
    .addr = MAX86150_ADDR,
    .i2c_write_read = max86150_write_read,
    .i2c_read = max86150_read,
    .i2c_write = max86150_write,
};

static max86150_slot_type maxSlotsCfg[] = {
    Max86150SlotPpgLed1,
    Max86150SlotPpgLed2,
    Max86150SlotEcg,
    Max86150SlotOff
};

static max86150_config_t maxCfg = {
    .numSlots = 3,
    .fifoSlotConfigs = maxSlotsCfg,
    .fifoRolloverFlag = 1,      // Enable FIFO rollover
    .ppgSampleAvg = 2,          // Avg 4 samples
    .ppgAdcRange = 2,           // 16,384 nA Scale
    .ppgSampleRate = 5,         // 200 Hz
    .ppgPulseWidth = 1,         // 100 us
    .led0CurrentRange = 0,      // IR LED 50 mA range
    .led1CurrentRange = 0,      // RED LED 50 mA range
    .led2CurrentRange = 0,      // Pilot LED 50 mA range
    .led0PulseAmplitude = 0x32, // IR LED 20 mA 0x32
    .led1PulseAmplitude = 0x32, // RED LED 20 mA 0x32
    .led2PulseAmplitude = 0x32, // AMB LED 20 mA 0x32
    .ecgSampleRate = 3,         // Fs = 200 Hz
    .ecgIaGain = 2,             // 9.5 V/V
    .ecgPgaGain = 3             // 8 V/V
};

static uint32_t max86150Buffer[32 * 4];

sensor_context_t sensorCtx = {
    .maxCtx = &maxCtx,
    .maxCfg = &maxCfg,
    .buffer = max86150Buffer,
    .is_stimulus = true
};


static float32_t ecgSensorBuffer[SENSOR_BUF_LEN];
rb_config_t rbEcgSensor = {
    .buffer = (void *)ecgSensorBuffer,
    .dlen = sizeof(float32_t),
    .size = SENSOR_BUF_LEN,
    .head = 0,
    .tail = 0,
};
static float32_t ppg1SensorBuffer[SENSOR_BUF_LEN];
rb_config_t rbPpg1Sensor = {
    .buffer = (void *)ppg1SensorBuffer,
    .dlen = sizeof(float32_t),
    .size = SENSOR_BUF_LEN,
    .head = 0,
    .tail = 0,
};
static float32_t ppg2SensorBuffer[SENSOR_BUF_LEN];
rb_config_t rbPpg2Sensor = {
    .buffer = (void *)ppg2SensorBuffer,
    .dlen = sizeof(float32_t),
    .size = SENSOR_BUF_LEN,
    .head = 0,
    .tail = 0,
};

// print(pk.signal.generate_arm_biquad_sos(0.5, 30, 100, order=3, var_name="ecgSos"))
static float32_t ecgSosState[4 * ECG_SOS_LEN] = {0};
static float32_t ecgSos[5 * ECG_SOS_LEN] = {
   0.2467691808982006, 0.4935383617964012, 0.2467691808982006, -0.4141296048598937, -0.36229096617676754,
   1.0, 0.0, -1.0, 0.8213745394235588, 0.14232107570294283,
   1.0, -2.0, 1.0, 1.9684516644108876, -0.9694342914476478
};
arm_biquad_casd_df1_inst_f32 ecgFilterCtx = {.numStages = ECG_SOS_LEN, .pState = ecgSosState, .pCoeffs = ecgSos};

static float32_t ppg1SosState[4 * PPG_SOS_LEN] = {0};
static float32_t ppg2SosState[4 * PPG_SOS_LEN] = {0};
// print(pk.signal.generate_arm_biquad_sos(0.5, 4, 100, order=3, var_name='ppgSos'))
static float32_t ppgSos[5 * PPG_SOS_LEN] = {
   0.0010794898220693148, 0.0021589796441386297, 0.0010794898220693148, 1.7720892400740829, -0.8247887959007985,
   1.0, 0.0, -1.0, 1.7940163154176096, -0.8011510705587511,
   1.0, -2.0, 1.0, 1.9728975520867649, -0.9739584899738475
};
// static float32_t ppgSos[5 * PPG_SOS_LEN] = {
//    0.00014931384487373373, 0.00029862768974746746, 0.00014931384487373373, 1.8940501971094346, -0.9078634324846275,
//    1.0, 0.0, -1.0, 1.893802173226849, -0.8956747083704727,
//    1.0, -2.0, 1.0, 1.9866361645090183, -0.9869032103299213
// };

arm_biquad_casd_df1_inst_f32 ppg1FilterCtx = {.numStages = PPG_SOS_LEN, .pState = ppg1SosState, .pCoeffs = ppgSos};
arm_biquad_casd_df1_inst_f32 ppg2FilterCtx = {.numStages = PPG_SOS_LEN, .pState = ppg2SosState, .pCoeffs = ppgSos};


///////////////////////////////////////////////////////////////////////////////
// Segmentation Configuration
///////////////////////////////////////////////////////////////////////////////

static constexpr int segTensorArenaSize = 1024 * SEG_MODEL_SIZE_KB;
alignas(16) static uint8_t segTensorArena[segTensorArenaSize];
tf_model_context_t segModelCtx = {
    .arenaSize = segTensorArenaSize,
    .arena = segTensorArena,
    .buffer = segmentation_flatbuffer,
    .model = nullptr,
    .input = nullptr,
    .output = nullptr,
    .interpreter = nullptr,
};
float32_t segFilterBuffer[SEG_WINDOW_LEN];
float32_t segInputs[SEG_WINDOW_LEN];
uint8_t segMask[SEG_WINDOW_LEN];

static float32_t ecgSegBuffer[SEG_BUF_LEN];
rb_config_t rbEcgSeg = {
    .buffer = (void *)ecgSegBuffer,
    .dlen = sizeof(float32_t),
    .size = SEG_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static float32_t ppg1SegBuffer[SEG_BUF_LEN];
rb_config_t rbPpg1Seg = {
    .buffer = (void *) ppg1SegBuffer,
    .dlen = sizeof(float32_t),
    .size = SEG_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static float32_t ppg2SegBuffer[SEG_BUF_LEN];
rb_config_t rbPpg2Seg = {
    .buffer = (void *) ppg2SegBuffer,
    .dlen = sizeof(float32_t),
    .size = SEG_BUF_LEN,
    .head = 0,
    .tail = 0,
};


///////////////////////////////////////////////////////////////////////////////
// Metrics Configuration
///////////////////////////////////////////////////////////////////////////////

metrics_config_t metricsCfg = {
};

static float32_t ecgMetricsRBuffer[MET_BUF_LEN];
rb_config_t rbEcgMetrics = {
    .buffer = (void *)ecgMetricsRBuffer,
    .dlen = sizeof(float32_t),
    .size = MET_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static uint8_t ecgMaskMetricsRBuffer[MET_BUF_LEN];
rb_config_t rbEcgMaskMetrics = {
    .buffer = (void *)ecgMaskMetricsRBuffer,
    .dlen = sizeof(uint8_t),
    .size = MET_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static float32_t ppg1MetricsRBuffer[MET_BUF_LEN];
rb_config_t rbPpg1Metrics = {
    .buffer = (void *)ppg1MetricsRBuffer,
    .dlen = sizeof(float32_t),
    .size = MET_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static uint8_t ppg1MaskMetricsRBuffer[MET_BUF_LEN];
rb_config_t rbPpg1MaskMetrics = {
    .buffer = (void *)ppg1MaskMetricsRBuffer,
    .dlen = sizeof(uint8_t),
    .size = MET_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static float32_t ppg2MetricsRBuffer[MET_BUF_LEN];
rb_config_t rbPpg2Metrics = {
    .buffer = (void *)ppg2MetricsRBuffer,
    .dlen = sizeof(float32_t),
    .size = MET_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static uint8_t ppg2MaskMetricsRBuffer[MET_BUF_LEN];
rb_config_t rbPpg2MaskMetrics = {
    .buffer = (void *)ppg2MaskMetricsRBuffer,
    .dlen = sizeof(uint8_t),
    .size = MET_BUF_LEN,
    .head = 0,
    .tail = 0,
};

float32_t ecgMetricsData[MET_WINDOW_LEN];
float32_t ppg1MetricsData[MET_WINDOW_LEN];
float32_t ppg2MetricsData[MET_WINDOW_LEN];
uint8_t ecgMaskMetricsData[MET_WINDOW_LEN];
uint8_t ppg1MaskMetricsData[MET_WINDOW_LEN];
uint8_t ppg2MaskMetricsData[MET_WINDOW_LEN];

uint8_t metricsMask[MET_BUF_LEN];
uint32_t peaksMetrics[MAX_RR_PEAKS];
uint32_t rriMetrics[MAX_RR_PEAKS];

// float32_t spo2Coefs[3] = {-16.666666, 8.333333, 100};
float32_t spo2Coefs[3] = {-16.666666, 8.333333, 100};
// float32_t spo2Coefs[3] = {1.5958422, -34.6596622, 112.6898759};

static hrv_td_metrics_t ecgHrv;

static float32_t pkArena[5*MET_BUF_LEN];
ppg_peak_f32_t ppgFindPeakCtx = {
    .peakWin=0.111,
    .beatWin=0.666,
    .beatOffset=0.333,
    .peakDelayWin=0.3,
    .sampleRate=SAMPLE_RATE,
    .state=pkArena
};

static ecg_peak_f32_t qrsFindPeakCtx = {
    .qrsWin=0.1,
    .avgWin=1.0,
    .qrsPromWeight=1.5,
    .qrsMinLenWeight=0.4,
    .qrsDelayWin=0.3,
    .sampleRate=SAMPLE_RATE,
    .state=pkArena
};

metrics_results_t metricsResults = {
    .hr = 0,
    .spo2 = 0
};


///////////////////////////////////////////////////////////////////////////////
// BLE Configuration
///////////////////////////////////////////////////////////////////////////////

static uint32_t webbleWSFBufferPool[WEBBLE_WSF_BUFFER_SIZE];
static wsfBufPoolDesc_t webbleBufferDescriptors[WEBBLE_WSF_BUFFER_POOLS] = {
    {16, 8}, // 16 bytes, 8 buffers
    {32, 4},
    {64, 6},
    {512, 14}};

static ns_ble_pool_config_t bleWsfBuffers = {
    .pool = webbleWSFBufferPool,
    .poolSize = sizeof(webbleWSFBufferPool),
    .desc = webbleBufferDescriptors,
    .descNum = WEBBLE_WSF_BUFFER_POOLS
};
static uint8_t bleSigBuffer[BLE_SIG_BUF_LEN] = {0};
static uint8_t bleUioBuffer[1] = {0};

static ns_ble_service_t bleService;
static ns_ble_characteristic_t bleSigChar;
static ns_ble_characteristic_t bleMetricsChar;
static ns_ble_characteristic_t bleUioChar;

pk_ble_context_t bleCtx = {
    .pool = &bleWsfBuffers,
    .service = &bleService,
    .sigChar = &bleSigChar,
    .metricsChar = &bleMetricsChar,
    .uioChar = &bleUioChar,
    .sigBuffer = bleSigBuffer,
    .metricsResults = &metricsResults,
    .uioBuffer = &uioState.byte
};


static float32_t ecgTxBuffer[BLE_BUF_LEN];
rb_config_t rbEcgTx = {
    .buffer = (void *)ecgTxBuffer,
    .dlen = sizeof(float32_t),
    .size = BLE_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static uint8_t ecgMaskTxBuffer[BLE_BUF_LEN];
rb_config_t rbEcgMaskTx = {
    .buffer = (void *)ecgMaskTxBuffer,
    .dlen = sizeof(uint8_t),
    .size = BLE_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static float32_t ppg1TxBuffer[BLE_BUF_LEN];
rb_config_t rbPpg1Tx = {
    .buffer = (void *)ppg1TxBuffer,
    .dlen = sizeof(float32_t),
    .size = BLE_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static uint8_t ppg1MaskTxBuffer[BLE_BUF_LEN];
rb_config_t rbPpg1MaskTx = {
    .buffer = (void *)ppg1MaskTxBuffer,
    .dlen = sizeof(uint8_t),
    .size = BLE_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static float32_t ppg2TxBuffer[BLE_BUF_LEN];
rb_config_t rbPpg2Tx = {
    .buffer = (void *)ppg2TxBuffer,
    .dlen = sizeof(float32_t),
    .size = BLE_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static uint8_t ppg2MaskTxBuffer[BLE_BUF_LEN];
rb_config_t rbPpg2MaskTx = {
    .buffer = (void *)ppg2MaskTxBuffer,
    .dlen = sizeof(uint8_t),
    .size = BLE_BUF_LEN,
    .head = 0,
    .tail = 0,
};


///////////////////////////////////////////////////////////////////////////////
// LED Configuration
///////////////////////////////////////////////////////////////////////////////

// static uint8_t LED_COLORS[10][4] = {
//     {0,  32, 191, 246},
//     {1,  32, 191 ,246},
//     {2,  32, 191 ,246},
//     {3,  42, 163, 238},
//     {4,  71, 120, 228},
//     {5, 106,  80, 218},
//     {6, 144,  39, 208},
//     {7, 185,   0, 198},
//     {8, 207,   0, 193},
//     {9, 207,   0, 193},
// };

///////////////////////////////////////////////////////////////////////////////
// APP Configuration
///////////////////////////////////////////////////////////////////////////////

ns_timer_config_t tickTimerCfg = {
    .api = &ns_timer_V1_0_0,
    .timer = NS_TIMER_COUNTER,
    .enableInterrupt = false,
};

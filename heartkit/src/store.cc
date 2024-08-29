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
#include "ecg_segmentation.h"
#include "ecg_segmentation_flatbuffer.h"
#include "ecg_denoise.h"
#include "ecg_denoise_flatbuffer.h"
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

ns_timer_config_t timerCfg = {
    .api = &ns_timer_V1_0_0,
    .timer = NS_TIMER_COUNTER,
    .enableInterrupt = false
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
    Max86150SlotEcg,
    Max86150SlotOff,
    Max86150SlotOff,
    Max86150SlotOff
};

static max86150_config_t maxCfg = {
    .numSlots = 1,
    .fifoSlotConfigs = maxSlotsCfg,
    .fifoRolloverFlag = 1,      // Enable FIFO rollover
    .ppgSampleAvg = 2,          // Avg 4 samples
    .ppgAdcRange = 2,           // 16,384 nA Scale
    .ppgSampleRate = 5,         // 200 Hz
    .ppgPulseWidth = 1,         // 100 us
    .led0CurrentRange = 1,      // IR LED 50 mA range
    .led1CurrentRange = 1,      // RED LED 50 mA range
    .led2CurrentRange = 1,      // Pilot LED 50 mA range
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
    .input_source = 0
};


static float32_t ecgSensorBuffer[SENSOR_BUF_LEN];
rb_config_t rbEcgSensor = {
    .buffer = (void *)ecgSensorBuffer,
    .dlen = sizeof(float32_t),
    .size = SENSOR_BUF_LEN,
    .head = 0,
    .tail = 0,
};


///////////////////////////////////////////////////////////////////////////////
// Preprocess Configuration
///////////////////////////////////////////////////////////////////////////////

// print(pk.signal.generate_arm_biquad_sos(0.5, 30, 100, order=3, var_name="ecgSos"))
static float32_t ecgSosState[4 * ECG_SOS_LEN] = {0};
static float32_t ecgSos[5 * ECG_SOS_LEN] = {
//    0.2467691808982006, 0.4935383617964012, 0.2467691808982006, -0.4141296048598937, -0.36229096617676754,
//    1.0, 0.0, -1.0, 0.8213745394235588, 0.14232107570294283,
//    1.0, -2.0, 1.0, 1.9684516644108876, -0.9694342914476478
   0.016752146191797653, 0.033504292383595306, 0.016752146191797653, -0.30622194637462763, -0.05449269463537451,
   1.0, 2.0, 1.0, -0.344393306862485, -0.15985880409401879,
   1.0, 2.0, 1.0, -0.41412960485989386, -0.3622909661767674,
   1.0, 2.0, 1.0, -0.5310464459467606, -0.7218241622476405,
   1.0, 0.0, -1.0, 0.8213745394235586, 0.14232107570294292,
   1.0, -2.0, 1.0, 1.9406726557765677, -0.9416707574272809,
   1.0, -2.0, 1.0, 1.9518568600455033, -0.9528464655870746,
   1.0, -2.0, 1.0, 1.9684516644108876, -0.9694342914476478,
   1.0, -2.0, 1.0, 1.9883972215155044, -0.98938015479297
};
arm_biquad_casd_df1_inst_f32 ecgFilterCtx = {.numStages = ECG_SOS_LEN, .pState = ecgSosState, .pCoeffs = ecgSos};


///////////////////////////////////////////////////////////////////////////////
// ECG Denoise Configuration
///////////////////////////////////////////////////////////////////////////////

float32_t ecgDenScratch[ECG_DEN_WINDOW_LEN];
float32_t ecgDenInout[ECG_DEN_WINDOW_LEN];
float32_t ecgDenNoise[ECG_DEN_WINDOW_LEN];


static constexpr int denTensorArenaSize = 1024 * ECG_DEN_MODEL_SIZE_KB;
alignas(16) static uint8_t denTensorArena[denTensorArenaSize];
tf_model_context_t ecgDenModelCtx = {
    .arenaSize = denTensorArenaSize,
    .arena = denTensorArena,
    .buffer = ecg_denoise_flatbuffer,
    .model = nullptr,
    .input = nullptr,
    .output = nullptr,
    .interpreter = nullptr,
};

static float32_t ecgDenBuffer[ECG_DEN_BUF_LEN];
rb_config_t rbEcgDen = {
    .buffer = (void *)ecgDenBuffer,
    .dlen = sizeof(float32_t),
    .size = ECG_DEN_BUF_LEN,
    .head = 0,
    .tail = 0,
};

///////////////////////////////////////////////////////////////////////////////
// ECG Segmentation Configuration
///////////////////////////////////////////////////////////////////////////////

float32_t ecgSegScratch[ECG_SEG_WINDOW_LEN];
float32_t ecgSegInout[ECG_SEG_WINDOW_LEN];
uint16_t ecgSegMask[ECG_SEG_WINDOW_LEN];

static constexpr int segTensorArenaSize = 1024 * ECG_SEG_MODEL_SIZE_KB;
alignas(16) static uint8_t segTensorArena[segTensorArenaSize];
tf_model_context_t ecgSegModelCtx = {
    .arenaSize = segTensorArenaSize,
    .arena = segTensorArena,
    .buffer = ecg_segmentation_flatbuffer,
    .model = nullptr,
    .input = nullptr,
    .output = nullptr,
    .interpreter = nullptr,
};

static float32_t ecgSegBuffer[ECG_SEG_BUF_LEN];
rb_config_t rbEcgSeg = {
    .buffer = (void *)ecgSegBuffer,
    .dlen = sizeof(float32_t),
    .size = ECG_SEG_BUF_LEN,
    .head = 0,
    .tail = 0,
};


///////////////////////////////////////////////////////////////////////////////
// Shared Metrics Configuration
///////////////////////////////////////////////////////////////////////////////

metrics_config_t metricsCfg = {};
uint32_t peaksMetrics[MAX_RR_PEAKS];
uint32_t rriMetrics[MAX_RR_PEAKS];
uint8_t rriMask[MAX_RR_PEAKS];
static float32_t pkArena[5*ECG_MET_BUF_LEN];

///////////////////////////////////////////////////////////////////////////////
// ECG Metrics Configuration
///////////////////////////////////////////////////////////////////////////////

static float32_t ecgMetRBuffer[ECG_MET_BUF_LEN];
rb_config_t rbEcgMet = {
    .buffer = (void *)ecgMetRBuffer,
    .dlen = sizeof(float32_t),
    .size = ECG_MET_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static uint16_t ecgMaskMetRBuffer[ECG_MET_BUF_LEN];
rb_config_t rbEcgMaskMet = {
    .buffer = (void *)ecgMaskMetRBuffer,
    .dlen = sizeof(uint16_t),
    .size = ECG_MET_BUF_LEN,
    .head = 0,
    .tail = 0,
};

float32_t ecgMetData[ECG_MET_WINDOW_LEN];
float32_t ecgDenMetData[ECG_MET_WINDOW_LEN];
uint16_t ecgMaskMetData[ECG_MET_WINDOW_LEN];

hrv_td_metrics_t ecgHrvMetrics;
metrics_ecg_results_t ecgMetResults = {
    .hr = 0,
    .hrv = 0,
    .denoise_ips = 0,
    .segment_ips = 0,
    .denoise_cossim = 0
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

tio_uio_state_t uioState = {
    .io0 = 0,
    .io1 = 0,
    .io2 = 0,
    .io3 = 0,
    .io4 = 0,
    .io5 = 0,
    .io6 = 0,
    .io7 = 0,
};
static uint8_t bleSlot0SigBuffer[TIO_BLE_SLOT_SIG_BUF_LEN] = {0};
static uint8_t bleSlot1SigBuffer[TIO_BLE_SLOT_SIG_BUF_LEN] = {0};
static uint8_t bleSlot2SigBuffer[TIO_BLE_SLOT_SIG_BUF_LEN] = {0};
static uint8_t bleSlot3SigBuffer[TIO_BLE_SLOT_SIG_BUF_LEN] = {0};

static uint8_t bleSlot0MetBuffer[TIO_BLE_SLOT_MET_BUF_LEN] = {0};
static uint8_t bleSlot1MetBuffer[TIO_BLE_SLOT_MET_BUF_LEN] = {0};
static uint8_t bleSlot2MetBuffer[TIO_BLE_SLOT_MET_BUF_LEN] = {0};
static uint8_t bleSlot3MetBuffer[TIO_BLE_SLOT_MET_BUF_LEN] = {0};

static uint8_t bleUioBuffer[TIO_BLE_UIO_BUF_LEN] = {0};

static ns_ble_service_t bleService;
static ns_ble_characteristic_t bleSlot0SigChar;
static ns_ble_characteristic_t bleSlot1SigChar;
static ns_ble_characteristic_t bleSlot2SigChar;
static ns_ble_characteristic_t bleSlot3SigChar;
static ns_ble_characteristic_t bleSlot0MetChar;
static ns_ble_characteristic_t bleSlot1MetChar;
static ns_ble_characteristic_t bleSlot2MetChar;
static ns_ble_characteristic_t bleSlot3MetChar;

static ns_ble_characteristic_t bleUioChar;

tio_ble_context_t bleCtx = {
    .pool = &bleWsfBuffers,
    .service = &bleService,
    .slot0SigChar = &bleSlot0SigChar,
    .slot1SigChar = &bleSlot1SigChar,
    .slot2SigChar = &bleSlot2SigChar,
    .slot3SigChar = &bleSlot3SigChar,
    .slot0MetChar = &bleSlot0MetChar,
    .slot1MetChar = &bleSlot1MetChar,
    .slot2MetChar = &bleSlot2MetChar,
    .slot3MetChar = &bleSlot3MetChar,
    .uioChar = &bleUioChar,
    .slot0SigBuffer = bleSlot0SigBuffer,
    .slot1SigBuffer = bleSlot1SigBuffer,
    .slot2SigBuffer = bleSlot2SigBuffer,
    .slot3SigBuffer = bleSlot3SigBuffer,
    .slot0MetBuffer = bleSlot0MetBuffer,
    .slot1MetBuffer = bleSlot1MetBuffer,
    .slot2MetBuffer = bleSlot2MetBuffer,
    .slot3MetBuffer = bleSlot3MetBuffer,
    .uioBuffer = bleUioBuffer
};


static float32_t ecgRawTxBuffer[ECG_TX_BUF_LEN];
rb_config_t rbEcgRawTx = {
    .buffer = (void *)ecgRawTxBuffer,
    .dlen = sizeof(float32_t),
    .size = ECG_TX_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static float32_t ecgDenTxBuffer[ECG_TX_BUF_LEN];
rb_config_t rbEcgDenTx = {
    .buffer = (void *)ecgDenTxBuffer,
    .dlen = sizeof(float32_t),
    .size = ECG_TX_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static uint16_t ecgMaskTxBuffer[ECG_TX_BUF_LEN];
rb_config_t rbEcgMaskTx = {
    .buffer = (void *)ecgMaskTxBuffer,
    .dlen = sizeof(uint16_t),
    .size = ECG_TX_BUF_LEN,
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

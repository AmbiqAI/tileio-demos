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

ns_timer_config_t tickTimerCfg = {
    .api = &ns_timer_V1_0_0,
    .timer = NS_TIMER_COUNTER,
    .enableInterrupt = false,
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
    .is_live = false
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
   0.2467691808982006, 0.4935383617964012, 0.2467691808982006, -0.4141296048598937, -0.36229096617676754,
   1.0, 0.0, -1.0, 0.8213745394235588, 0.14232107570294283,
   1.0, -2.0, 1.0, 1.9684516644108876, -0.9694342914476478
};
arm_biquad_casd_df1_inst_f32 ecgFilterCtx = {.numStages = ECG_SOS_LEN, .pState = ecgSosState, .pCoeffs = ecgSos};


///////////////////////////////////////////////////////////////////////////////
// ECG Denoise Configuration
///////////////////////////////////////////////////////////////////////////////

float32_t ecgDenScratch[ECG_DEN_WINDOW_LEN];
float32_t ecgDenInout[ECG_DEN_WINDOW_LEN];
float32_t ecgDenNoise[ECG_DEN_WINDOW_LEN];

uint16_t ecgDenMask[ECG_DEN_WINDOW_LEN];

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

static float32_t ecgRawMetRBuffer[ECG_MET_BUF_LEN];
rb_config_t rbEcgRawMet = {
    .buffer = (void *)ecgRawMetRBuffer,
    .dlen = sizeof(float32_t),
    .size = ECG_MET_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static float32_t ecgNosMetRBuffer[ECG_MET_BUF_LEN];
rb_config_t rbEcgNosMet = {
    .buffer = (void *)ecgNosMetRBuffer,
    .dlen = sizeof(float32_t),
    .size = ECG_MET_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static float32_t ecgDenMetRBuffer[ECG_MET_BUF_LEN];
rb_config_t rbEcgDenMet = {
    .buffer = (void *)ecgDenMetRBuffer,
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

float32_t ecgRawMetData[ECG_MET_WINDOW_LEN];
float32_t ecgNosMetData[ECG_MET_WINDOW_LEN];
float32_t ecgDenMetData[ECG_MET_WINDOW_LEN];
uint16_t ecgMaskMetData[ECG_MET_WINDOW_LEN];

ecg_peak_f32_t ecgFindPeakCtx = {
    .qrsWin = 0.1,
    .avgWin = 1.0,
    .qrsPromWeight = 1.5,
    .qrsMinLenWeight = 0.4,
    .qrsDelayWin = 0.3,
    .sampleRate = ECG_SAMPLE_RATE,
    .state = pkArena
};

metrics_ecg_results_t ecgMetResults;


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
    .io0 = false,
    .io1 = false,
    .io2 = false,
    .io3 = false,
    .io4 = false,
    .io5 = false,
    .io6 = false,
    .io7 = false,
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


static float32_t ecgRawTxBuffer[ECG_DEN_BUF_LEN];
rb_config_t rbEcgRawTx = {
    .buffer = (void *)ecgRawTxBuffer,
    .dlen = sizeof(float32_t),
    .size = ECG_DEN_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static float32_t ecgNosTxBuffer[ECG_DEN_BUF_LEN];
rb_config_t rbEcgNosTx = {
    .buffer = (void *)ecgNosTxBuffer,
    .dlen = sizeof(float32_t),
    .size = ECG_DEN_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static float32_t ecgDenTxBuffer[ECG_DEN_BUF_LEN];
rb_config_t rbEcgDenTx = {
    .buffer = (void *)ecgDenTxBuffer,
    .dlen = sizeof(float32_t),
    .size = ECG_DEN_BUF_LEN,
    .head = 0,
    .tail = 0,
};

static uint16_t ecgMaskTxBuffer[ECG_DEN_BUF_LEN];
rb_config_t rbEcgMaskTx = {
    .buffer = (void *)ecgMaskTxBuffer,
    .dlen = sizeof(uint16_t),
    .size = ECG_DEN_BUF_LEN,
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

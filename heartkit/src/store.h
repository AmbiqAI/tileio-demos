
/**
 * @file store.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Act as central store for app
 * @version 1.0
 * @date 2023-03-27
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __APP_STORE_H
#define __APP_STORE_H

#include "arm_math.h"
// neuralSPOT
#include "ns_ambiqsuite_harness.h"
#include "ns_i2c.h"
#include "ns_peripherals_button.h"
#include "ns_peripherals_power.h"
#include "ns_ble.h"
// PhysioKit
#include "physiokit/pk_ppg.h"
#include "physiokit/pk_hrv.h"
// Locals
#include "constants.h"
#include "ecg_segmentation.h"
#include "sensor.h"
#include "metrics.h"
#include "ringbuffer.h"

typedef union {
    struct {
        uint8_t io0;
        uint8_t io1;
        uint8_t io2;
        uint8_t io3;
        uint8_t io4;
        uint8_t io5;
        uint8_t io6;
        uint8_t io7;
    } __attribute__((packed));
    uint64_t bytes;
} tio_uio_state_t;


typedef struct {
    ns_ble_pool_config_t *pool;
    ns_ble_service_t *service;

    ns_ble_characteristic_t *slot0SigChar;
    ns_ble_characteristic_t *slot1SigChar;
    ns_ble_characteristic_t *slot2SigChar;
    ns_ble_characteristic_t *slot3SigChar;

    ns_ble_characteristic_t *slot0MetChar;
    ns_ble_characteristic_t *slot1MetChar;
    ns_ble_characteristic_t *slot2MetChar;
    ns_ble_characteristic_t *slot3MetChar;

    ns_ble_characteristic_t *uioChar;

    void *slot0SigBuffer;
    void *slot1SigBuffer;
    void *slot2SigBuffer;
    void *slot3SigBuffer;

    void *slot0MetBuffer;
    void *slot1MetBuffer;
    void *slot2MetBuffer;
    void *slot3MetBuffer;

    uint8_t *uioBuffer;

} tio_ble_context_t;

enum HeartRhythm { HeartRhythmNormal, HeartRhythmAfib, HeartRhythmAfut };
typedef enum HeartRhythm HeartRhythm;

enum HeartBeat { HeartBeatNormal, HeartBeatPac, HeartBeatPvc, HeartBeatNoise };
typedef enum HeartBeat HeartBeat;

enum HeartRate { HeartRateNormal, HeartRateTachycardia, HeartRateBradycardia };
typedef enum HeartRate HeartRate;

enum HeartSegment { HeartSegmentNormal, HeartSegmentPWave, HeartSegmentQrs, HeartSegmentTWave };
typedef enum HeartSegment HeartSegment;


///////////////////////////////////////////////////////////////////////////////
// EVB Configuration
///////////////////////////////////////////////////////////////////////////////

extern const ns_power_config_t nsPwrCfg;
extern ns_core_config_t nsCoreCfg;
extern ns_i2c_config_t nsI2cCfg;
extern ns_button_config_t nsBtnCfg;
extern tio_uio_state_t uioState;

///////////////////////////////////////////////////////////////////////////////
// Sensor Configuration
///////////////////////////////////////////////////////////////////////////////

extern sensor_context_t sensorCtx;
extern rb_config_t rbEcgSensor;


///////////////////////////////////////////////////////////////////////////////
// Preprocess Configuration
///////////////////////////////////////////////////////////////////////////////

extern arm_biquad_casd_df1_inst_f32 ecgFilterCtx;


///////////////////////////////////////////////////////////////////////////////
// ECG Denoise Configuration
///////////////////////////////////////////////////////////////////////////////

extern tf_model_context_t ecgDenModelCtx;
extern float32_t ecgDenScratch[ECG_DEN_WINDOW_LEN];
extern float32_t ecgDenInout[ECG_DEN_WINDOW_LEN];
extern float32_t ecgDenNoise[ECG_DEN_WINDOW_LEN];
extern rb_config_t rbEcgDen;


///////////////////////////////////////////////////////////////////////////////
// ECG Segmentation Configuration
///////////////////////////////////////////////////////////////////////////////

extern tf_model_context_t ecgSegModelCtx;
extern float32_t ecgSegScratch[ECG_SEG_WINDOW_LEN];
extern float32_t ecgSegInout[ECG_SEG_WINDOW_LEN];
extern uint16_t ecgSegMask[ECG_SEG_WINDOW_LEN];
extern rb_config_t rbEcgSeg;

///////////////////////////////////////////////////////////////////////////////
// Shared Metrics Configuration
///////////////////////////////////////////////////////////////////////////////

extern metrics_config_t metricsCfg;
extern uint32_t peaksMetrics[MAX_RR_PEAKS];
extern uint32_t rriMetrics[MAX_RR_PEAKS];
extern uint8_t rriMask[MAX_RR_PEAKS];


///////////////////////////////////////////////////////////////////////////////
// ECG Metrics Configuration
///////////////////////////////////////////////////////////////////////////////

extern rb_config_t rbEcgMet;
extern rb_config_t rbEcgMaskMet;

extern float32_t ecgMetData[ECG_MET_WINDOW_LEN];
extern uint16_t ecgMaskMetData[ECG_MET_WINDOW_LEN];

extern hrv_td_metrics_t ecgHrvMetrics;

extern metrics_ecg_results_t ecgMetResults;


///////////////////////////////////////////////////////////////////////////////
// BLE Configuration
///////////////////////////////////////////////////////////////////////////////

extern tio_ble_context_t bleCtx;

extern rb_config_t rbEcgRawTx;
extern rb_config_t rbEcgDenTx;
extern rb_config_t rbEcgMaskTx;

///////////////////////////////////////////////////////////////////////////////
// APP Configuration
///////////////////////////////////////////////////////////////////////////////

extern ns_timer_config_t timerCfg;

#endif // __APP_STORE_H

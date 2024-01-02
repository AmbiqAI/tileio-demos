
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
#ifndef __PK_STORE_H
#define __PK_STORE_H

#include "arm_math.h"
// neuralSPOT
#include "ns_ambiqsuite_harness.h"
#include "ns_i2c.h"
#include "ns_peripherals_button.h"
#include "ns_peripherals_power.h"
#include "ns_ble.h"
// PhysioKit
#include "physiokit/pk_ppg.h"
// Locals
#include "constants.h"
#include "ecg_segmentation.h"
#include "sensor.h"
#include "metrics.h"
#include "ringbuffer.h"

typedef union {
    struct {
        uint8_t btn0;
        uint8_t btn1;
        uint8_t btn2;
        uint8_t btn3;
        uint8_t led0;
        uint8_t led1;
        uint8_t led2;
        uint8_t led3;
    } __attribute__((packed));
    uint64_t bytes;
} pk_uio_state_t;


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

    void *uioBuffer;

} pk_ble_context_t;

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
extern pk_uio_state_t uioState;

///////////////////////////////////////////////////////////////////////////////
// Sensor Configuration
///////////////////////////////////////////////////////////////////////////////

extern sensor_context_t sensorCtx;
extern rb_config_t rbEcgSensor;
extern rb_config_t rbPpg1Sensor;
extern rb_config_t rbPpg2Sensor;


///////////////////////////////////////////////////////////////////////////////
// Preprocess Configuration
///////////////////////////////////////////////////////////////////////////////

extern arm_biquad_casd_df1_inst_f32 ecgFilterCtx;
extern arm_biquad_casd_df1_inst_f32 ppg1FilterCtx;
extern arm_biquad_casd_df1_inst_f32 ppg2FilterCtx;


///////////////////////////////////////////////////////////////////////////////
// ECG Segmentation Configuration
///////////////////////////////////////////////////////////////////////////////

extern tf_model_context_t ecgSegModelCtx;
extern float32_t ecgSegScratch[ECG_SEG_WINDOW_LEN];
extern float32_t ecgSegInputs[ECG_SEG_WINDOW_LEN];
extern uint16_t ecgSegMask[ECG_SEG_WINDOW_LEN];
extern rb_config_t rbEcgSeg;


///////////////////////////////////////////////////////////////////////////////
// PPG Segmentation Configuration
///////////////////////////////////////////////////////////////////////////////

// extern tf_model_context_t ppgSegModelCtx;
extern float32_t ppgSegScratch[PPG_SEG_WINDOW_LEN];
extern float32_t ppgSegInputs[PPG_SEG_WINDOW_LEN];
extern uint16_t ppgSegMask[PPG_SEG_WINDOW_LEN];
extern rb_config_t rbPpg1Seg;
extern rb_config_t rbPpg2Seg;


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

extern metrics_ecg_results_t ecgMetResults;

///////////////////////////////////////////////////////////////////////////////
// PPG Metrics Configuration
///////////////////////////////////////////////////////////////////////////////

extern float32_t spo2Coefs[3];
extern rb_config_t rbPpg1Met;
extern rb_config_t rbPpg2Met;
extern rb_config_t rbPpgMaskMet;
extern float32_t ppg1MetData[PPG_MET_WINDOW_LEN];
extern float32_t ppg2MetData[PPG_MET_WINDOW_LEN];
extern uint16_t ppgMaskMetData[PPG_MET_WINDOW_LEN];
extern ppg_peak_f32_t ppgFindPeakCtx;

extern metrics_ppg_results_t ppgMetResults;

///////////////////////////////////////////////////////////////////////////////
// BLE Configuration
///////////////////////////////////////////////////////////////////////////////

extern pk_ble_context_t bleCtx;
extern rb_config_t rbEcgTx;
extern rb_config_t rbPpg1Tx;
extern rb_config_t rbPpg2Tx;
extern rb_config_t rbEcgMaskTx;
extern rb_config_t rbPpgMaskTx;


///////////////////////////////////////////////////////////////////////////////
// APP Configuration
///////////////////////////////////////////////////////////////////////////////

extern ns_timer_config_t tickTimerCfg;

#endif // __PK_STORE_H

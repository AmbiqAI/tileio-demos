
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
#include "segmentation.h"
#include "sensor.h"
#include "metrics.h"
#include "ringbuffer.h"

typedef union {
    struct {
        unsigned int btn0 : 1;
        unsigned int btn1 : 1;
        unsigned int btn2 : 1;
        unsigned int led0 : 1;
        unsigned int led1 : 1;
        unsigned int led2 : 1;
        unsigned int rsv0 : 1;
        unsigned int rsv1 : 1;
    } __attribute__((packed));
    uint8_t byte;
} pk_uio_state_t;

typedef struct {
    ns_ble_pool_config_t *pool;
    ns_ble_service_t *service;
    ns_ble_characteristic_t *sigChar;
    ns_ble_characteristic_t *metricsChar;
    ns_ble_characteristic_t *uioChar;
    uint8_t *sigBuffer;
    metrics_results_t *metricsResults;
    uint8_t *uioBuffer;
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
extern rb_config_t rbPpg1Sensor;
extern rb_config_t rbPpg2Sensor;
extern rb_config_t rbEcgSensor;

extern arm_biquad_casd_df1_inst_f32 ecgFilterCtx;
extern arm_biquad_casd_df1_inst_f32 ppg1FilterCtx;
extern arm_biquad_casd_df1_inst_f32 ppg2FilterCtx;


///////////////////////////////////////////////////////////////////////////////
// Segmentation Configuration
///////////////////////////////////////////////////////////////////////////////
extern tf_model_context_t segModelCtx;
extern float32_t segFilterBuffer[SEG_WINDOW_LEN];
extern float32_t segInputs[SEG_WINDOW_LEN];
extern uint8_t segMask[SEG_WINDOW_LEN];

extern rb_config_t rbEcgSeg;
extern rb_config_t rbPpg1Seg;
extern rb_config_t rbPpg2Seg;


///////////////////////////////////////////////////////////////////////////////
// Metrics Configuration
///////////////////////////////////////////////////////////////////////////////

extern float32_t spo2Coefs[3];
extern metrics_config_t metricsCfg;

extern rb_config_t rbEcgMetrics;
extern rb_config_t rbEcgMaskMetrics;
extern rb_config_t rbPpg1Metrics;
extern rb_config_t rbPpg1MaskMetrics;
extern rb_config_t rbPpg2Metrics;
extern rb_config_t rbPpg2MaskMetrics;

extern float32_t ecgMetricsData[MET_WINDOW_LEN];
extern float32_t ppg1MetricsData[MET_WINDOW_LEN];
extern float32_t ppg2MetricsData[MET_WINDOW_LEN];
extern uint8_t ecgMaskMetricsData[MET_WINDOW_LEN];
extern uint8_t ppg1MaskMetricsData[MET_WINDOW_LEN];
extern uint8_t ppg2MaskMetricsData[MET_WINDOW_LEN];

extern uint8_t metricsMask[MET_BUF_LEN];
extern uint32_t peaksMetrics[MAX_RR_PEAKS];
extern uint32_t rriMetrics[MAX_RR_PEAKS];

extern ppg_peak_f32_t ppgFindPeakCtx;
extern metrics_results_t metricsResults;

///////////////////////////////////////////////////////////////////////////////
// BLE Configuration
///////////////////////////////////////////////////////////////////////////////

extern pk_ble_context_t bleCtx;

extern rb_config_t rbPpg1Tx;
extern rb_config_t rbPpg2Tx;
extern rb_config_t rbEcgTx;
extern rb_config_t rbPpg1MaskTx;
extern rb_config_t rbPpg2MaskTx;
extern rb_config_t rbEcgMaskTx;


///////////////////////////////////////////////////////////////////////////////
// APP Configuration
///////////////////////////////////////////////////////////////////////////////

extern ns_timer_config_t tickTimerCfg;

#endif // __PK_STORE_H

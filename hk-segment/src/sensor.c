/**
 * @file sensor.cc
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Initializes and collects sensor data from MAX86150
 * @version 1.0
 * @date 2023-03-27
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "arm_math.h"
#include "ns_ambiqsuite_harness.h"
#include "ns_i2c.h"
#include "ns_max86150_driver.h"
#include "constants.h"
#include "ringbuffer.h"
#include "stimulus.h"
#include "sensor.h"

#define MAX86150_FIFO_DEPTH (32)
#define MAX86150_NUM_SLOTS (4)
#define RESET_DELAY_US (10000)

uint32_t
sensor_init(sensor_context_t *ctx) {
    max86150_powerup(ctx->maxCtx);
    ns_delay_us(RESET_DELAY_US);
    max86150_reset(ctx->maxCtx);
    ns_delay_us(RESET_DELAY_US);
    max86150_set_fifo_slots(ctx->maxCtx, ctx->maxCfg->fifoSlotConfigs);
    max86150_set_almost_full_rollover(ctx->maxCtx, ctx->maxCfg->fifoRolloverFlag);
    max86150_set_ppg_sample_average(ctx->maxCtx, ctx->maxCfg->ppgSampleAvg);
    max86150_set_ppg_adc_range(ctx->maxCtx, ctx->maxCfg->ppgAdcRange);
    max86150_set_ppg_sample_rate(ctx->maxCtx, ctx->maxCfg->ppgSampleRate);
    max86150_set_ppg_pulse_width(ctx->maxCtx, ctx->maxCfg->ppgPulseWidth);
    max86150_set_prox_int_flag(ctx->maxCtx, 0);
    max86150_set_led_current_range(ctx->maxCtx, 0, ctx->maxCfg->led0CurrentRange);
    max86150_set_led_current_range(ctx->maxCtx, 1, ctx->maxCfg->led1CurrentRange);
    max86150_set_led_current_range(ctx->maxCtx, 2, ctx->maxCfg->led2CurrentRange);
    max86150_set_led_pulse_amplitude(ctx->maxCtx, 0, ctx->maxCfg->led0PulseAmplitude);
    max86150_set_led_pulse_amplitude(ctx->maxCtx, 1, ctx->maxCfg->led1PulseAmplitude);
    max86150_set_led_pulse_amplitude(ctx->maxCtx, 2, ctx->maxCfg->led2PulseAmplitude);
    max86150_set_ecg_sample_rate(ctx->maxCtx, ctx->maxCfg->ecgSampleRate);
    max86150_set_ecg_ia_gain(ctx->maxCtx, ctx->maxCfg->ecgIaGain);
    max86150_set_ecg_pga_gain(ctx->maxCtx, ctx->maxCfg->ecgPgaGain);
    max86150_powerup(ctx->maxCtx);
    sensor_stop(ctx);
    return 0;
}

void
sensor_start(sensor_context_t *ctx) {
    // max86150_powerup(&maxCtx);
    max86150_set_fifo_enable(ctx->maxCtx, 1);
}

void
sensor_stop(sensor_context_t *ctx) {
    max86150_set_fifo_enable(ctx->maxCtx, 0);
    // max86150_shutdown(&maxCtx);
}


uint32_t
sensor_dummy_data(sensor_context_t *ctx) {
    static size_t _dummy_slot_idxs[4] = { 0, 0, 0, 0 };
    uint32_t numSamples = 16;
    size_t idx;
    for (size_t i = 0; i < numSamples; i++) {
        for (size_t j = 0; j < ctx->maxCfg->numSlots; j++) {
            idx = ctx->maxCfg->numSlots * i + j;
            if (ctx->maxCfg->fifoSlotConfigs[j] == Max86150SlotEcg) {
                ctx->buffer[idx] = ecg_stimulus[_dummy_slot_idxs[j]];
                _dummy_slot_idxs[j] = (_dummy_slot_idxs[j] + 1) % ecg_stimulus_len;
            } else if (ctx->maxCfg->fifoSlotConfigs[j] == Max86150SlotPpgLed1) {
                ctx->buffer[idx] = PPG_MET_MIN_VAL + ppg_stimulus[_dummy_slot_idxs[j]];
                _dummy_slot_idxs[j] = (_dummy_slot_idxs[j] + 1) % ppg_stimulus_len;
            } else if (ctx->maxCfg->fifoSlotConfigs[j] == Max86150SlotPpgLed2) {
                ctx->buffer[idx] = PPG_MET_MIN_VAL + ppg_stimulus[_dummy_slot_idxs[j]];
                _dummy_slot_idxs[j] = (_dummy_slot_idxs[j] + 1) % ppg_stimulus_len;
            } else {
                ctx->buffer[idx] = 0;
            }
        }
    }
    return numSamples;
}

uint32_t
sensor_capture_data(sensor_context_t *ctx) {
    if (ctx->is_stimulus) {
        return sensor_dummy_data(ctx);
    }
    return max86150_read_fifo_samples(ctx->maxCtx, ctx->buffer, ctx->maxCfg->fifoSlotConfigs, ctx->maxCfg->numSlots);
}

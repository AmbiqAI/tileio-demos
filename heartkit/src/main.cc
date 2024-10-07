/**
 * @file main.c
 * @author Adam Page (adam.page@ambiq.com)
 * @brief HeartKit demo
 * @version 1.0
 * @date 2024-04-16
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "ns_ambiqsuite_harness.h"
#include "ns_i2c.h"
#include "ns_peripherals_power.h"
#include "FreeRTOS.h"
#include "task.h"
#include "arm_math.h"
#include "ns_ble.h"

#include "main.h"
#include "am_devices_led.h"

#include "constants.h"
#include "store.h"
#include "ledstick.h"
#include "sensor.h"
#include "nstdb_noise.h"
#include "physiokit/pk_filter.h"
#include "physiokit/pk_ecg.h"
#include "physiokit/pk_math.h"
#include "tflm.h"
#include "ecg_segmentation.h"
#include "ecg_denoise.h"
#include "ecg_arrhythmia.h"
#include "metrics.h"
#include "ringbuffer.h"
#include "tileio.h"


#if (configAPPLICATION_ALLOCATED_HEAP == 1)
    #define APP_HEAP_SIZE (NS_BLE_DEFAULT_MALLOC_K * 4 * 1024)
size_t ucHeapSize = APP_HEAP_SIZE;
uint8_t ucHeap[APP_HEAP_SIZE] __attribute__((aligned(4)));
#endif

// RTOS Tasks
static TaskHandle_t sensorTaskHandle;
static TaskHandle_t processTaskHandle;
static TaskHandle_t tioTaskHandle;
static TaskHandle_t appSetupTask;

tio_context_t tioCtx = {
    .uio_update_cb = NULL,
    .slot_update_cb = NULL
};

////////////////////////////////////////////////////////////////
// APP STATE
////////////////////////////////////////////////////////////////

/**
 * @brief Flush pipeline buffers
 *
 */
void flush_pipeline() {
    ringbuffer_flush(&rbEcgSensor);
    ringbuffer_flush(&rbEcgDen);
    ringbuffer_flush(&rbEcgRawSeg);
    ringbuffer_flush(&rbEcgSeg);
    ringbuffer_flush(&rbEcgMet);
    ringbuffer_flush(&rbEcgMaskMet);
    ringbuffer_flush(&rbEcgRawTx);
    ringbuffer_flush(&rbEcgDenTx);
    ringbuffer_flush(&rbEcgMaskTx);
}

void
set_input_source(uint8_t source) {
    source = MIN(source, NUM_INPUT_PTS + 1); // + 1 for sensor mode
    if (appState.inputSource != source) {
        appState.inputSource = source;
        sensorCtx.inputSource = source;
        ns_lp_printf("Input Source: %d\n", sensorCtx.inputSource);
        // flush_pipeline();
    }
}

void
set_noise_inputs(uint8_t bw, uint8_t ma, uint8_t em) {
    appState.bwNoiseLevel = bw;
    appState.maNoiseLevel = ma;
    appState.emNoiseLevel = em;
    ns_lp_printf("Noise Level: %d,%d,%d\n", appState.bwNoiseLevel, appState.maNoiseLevel, appState.emNoiseLevel);
}

void
set_denoise_mode(uint8_t mode) {
    mode = MIN(mode, 2);
    if (appState.denoiseMode != mode) {
        appState.denoiseMode = mode;
        ns_lp_printf("Denoise Mode: %d\n", appState.denoiseMode);
    }
}

void
set_segmentation_mode(uint8_t mode) {
    mode = MIN(mode, 2);
    if (appState.segMode != mode) {
        appState.segMode = mode;
        ns_lp_printf("Segmentation Mode: %d\n", appState.segMode);
    }
}

void
set_arrhythmia_mode(uint8_t mode) {
    mode = MIN(mode, 2);
    if (appState.arrMode != mode) {
        appState.arrMode = mode;
        ns_lp_printf("Arrhythmia Mode: %d\n", appState.arrMode);
    }
}


void
set_speed_mode(uint8_t mode) {
    mode = MIN(mode, 1);
    if (appState.speedMode != mode) {
        appState.speedMode = mode;
        ns_set_performance_mode(mode ? NS_MAXIMUM_PERF : NS_MINIMUM_PERF);
        ns_lp_printf("CPU Speed Mode: %d\n", appState.speedMode);
    }
}


void
fetch_leds_state() {
    uint8_t ledVal = (am_devices_led_get(am_bsp_psLEDs, 2) << 2);
    ledVal |= (am_devices_led_get(am_bsp_psLEDs, 1) << 1);
    ledVal |= am_devices_led_get(am_bsp_psLEDs, 0);
    appState.ledState = ledVal;
}

////////////////////////////////////////////////////////////////
// TIO
////////////////////////////////////////////////////////////////


/**
 * @brief Send slot0 (ECG) signals to TIO
 *
 */
void send_slot0_signals() {
    int16_t buffer[120];
    float32_t ecgRaw, ecgDen;
    uint32_t length;
    size_t numSamples = MIN3(
        ringbuffer_len(&rbEcgRawTx),
        ringbuffer_len(&rbEcgDenTx),
        ringbuffer_len(&rbEcgMaskTx)
    );
    numSamples = MIN(
        numSamples,
        240/(3*sizeof(int16_t))
    );
    if (numSamples == 0) { return; }
    length = 0;
    for (size_t i = 0; i < numSamples; i++) {
        ringbuffer_pop(&rbEcgMaskTx, &buffer[length + 0], 1);
        ringbuffer_pop(&rbEcgRawTx, &ecgRaw, 1);
        ringbuffer_pop(&rbEcgDenTx, &ecgDen, 1);
        buffer[length + 1] = (int16_t)CLIP(TIO_SLOT0_SCALE*ecgRaw, -32768, 32767);
        buffer[length + 2] = (int16_t)CLIP(TIO_SLOT0_SCALE*ecgDen, -32768, 32767);
        length += 3;
    }
    tio_send_slot_data(0, 0, (uint8_t *)buffer, length * sizeof(int16_t));
}

/**
 * @brief Send slot0 (ECG) metrics to TIO
 *
 */
void send_slot0_metrics() {
    float32_t buffer[60];
    buffer[0] = appMetResults.hr;
    buffer[1] = appMetResults.hrv;
    buffer[2] = appMetResults.denoiseCossim;
    buffer[3] = appMetResults.arrhythmiaLabel;
    buffer[4] = appMetResults.denoiseIps;
    buffer[5] = appMetResults.segmentIps;
    buffer[6] = appMetResults.arrhythmiaIps;
    buffer[7] = appMetResults.cpuPercUtil;
    tio_send_slot_data(0, 1, (uint8_t *)buffer, 8*sizeof(float32_t));
}

void received_slot_data(uint8_t slot, uint8_t slot_type, const uint8_t *data, uint32_t length) {
    // No slot data expected
}

void send_uio_state() {
    uint8_t uioBuffer[8];
    uioBuffer[TIO_UIO_INPUT_SEL_IDX] = appState.inputSource;
    uioBuffer[TIO_UIO_BW_NOISE_IDX] = appState.bwNoiseLevel;
    uioBuffer[TIO_UIO_MA_NOISE_IDX] = appState.maNoiseLevel;
    uioBuffer[TIO_UIO_EM_NOISE_IDX] = appState.emNoiseLevel;
    uioBuffer[TIO_UIO_SPEED_MODE_IDX] = appState.speedMode;
    uioBuffer[TIO_UIO_DEN_MODE_IDX] = appState.denoiseMode;
    uioBuffer[TIO_UIO_SEG_MODE_IDX] = appState.segMode;
    uioBuffer[TIO_UIO_ARR_MODE_IDX] = appState.arrMode;
    tio_send_uio_state(uioBuffer, 8);
}

void received_uio_state(const uint8_t *data, uint32_t length) {
    set_input_source(data[TIO_UIO_INPUT_SEL_IDX]);
    set_noise_inputs(
        data[TIO_UIO_BW_NOISE_IDX],
        data[TIO_UIO_MA_NOISE_IDX],
        data[TIO_UIO_EM_NOISE_IDX]
    );
    set_speed_mode(data[TIO_UIO_SPEED_MODE_IDX]);
    set_denoise_mode(data[TIO_UIO_DEN_MODE_IDX]);
    set_segmentation_mode(data[TIO_UIO_SEG_MODE_IDX]);
    set_arrhythmia_mode(data[TIO_UIO_ARR_MODE_IDX]);
    send_uio_state();
}

////////////////////////////////////////////////////////////////
// SENSOR TASK BLOCK
////////////////////////////////////////////////////////////////

/**
 * @brief Extract sensor data
 *
 * @return uint32_t
 */
uint32_t
extract_sensor_data(size_t numSamples) {
    int32_t val;

    float32_t val_f32;
    size_t idx;
    for (size_t i = 0; i < numSamples; i++) {
        for (size_t j = 0; j < sensorCtx.maxCfg->numSlots; j++) {
            idx = sensorCtx.maxCfg->numSlots * i + j;
            if (sensorCtx.maxCfg->fifoSlotConfigs[j] == Max86150SlotEcg) {
                val = sensorCtx.buffer[idx];
                if (sensorCtx.inputSource < NUM_INPUT_PTS) {
                    val_f32 = val;
                } else {
                    val_f32  = val & (1 << 17) ? val - (1 << 18) : val; // 2's complement
                }
                ringbuffer_push(&rbEcgSensor, &val_f32, 1);
            } else if (sensorCtx.maxCfg->fifoSlotConfigs[j] == Max86150SlotPpgLed1) {
                val = sensorCtx.buffer[idx];
                val_f32 = val;
                // ringbuffer_push(&rbPpg1Led2Sensor, &val_f32, 1);
            } else if (sensorCtx.maxCfg->fifoSlotConfigs[j] == Max86150SlotPpgLed2) {
                val = sensorCtx.buffer[idx];
                // val_f32 = val;
                // ringbuffer_push(&rbPpg2Led2Sensor, &val_f32, 1);
            }
        }
    }
    return numSamples;
}

/**
 * @brief Preprocess sensor data
 *
 */
void preprocess_sensor_data() {
    size_t numSamples;
    // Downsample sensor data to slots
    numSamples = ringbuffer_len(&rbEcgSensor);
    for (size_t i = 0; i < numSamples/ECG_DS_RATE; i++) {
        ringbuffer_seek(&rbEcgSensor, ECG_DS_RATE - 1);
        ringbuffer_transfer(&rbEcgSensor, &rbEcgDen, 1);
    }
}

void SensorTask(void *pvParameters) {
    size_t numSamples, reqSamples;
    uint32_t remUs = 0, tickUs = 0;
    ns_timer_clear(&timer2Cfg);
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(80)); // Delay for 16 samples (200Hz)
        tickUs = ns_us_ticker_read(&timer2Cfg) + remUs;
        ns_timer_clear(&timer2Cfg);
        if (sensorCtx.inputSource < NUM_INPUT_PTS) {
            reqSamples = MIN(tickUs/1000/SENSOR_RATE_MS, 32),
            remUs = tickUs - 1000*reqSamples*SENSOR_RATE_MS;
            numSamples = sensor_dummy_data(&sensorCtx, reqSamples);
        } else {
            numSamples = sensor_capture_data(&sensorCtx);
        }
        extract_sensor_data(numSamples);
        preprocess_sensor_data();
        if (numSamples >= 30) {
            ns_lp_printf("<SENSOR %d >\n", numSamples);
        }
    }
}


////////////////////////////////////////////////////////////////
// PROCESS TASK BLOCK
////////////////////////////////////////////////////////////////

void ProcessTask(void *pvParameters) {
    uint32_t err = 0;
    uint32_t delayUs = 0, tickUs = 0;
    uint32_t deltaUs = 0;
    float32_t cpuIdleMs = 0, cpuBusyMs = 0;

    while (true) {
        err = 0;
        ns_timer_clear(&timerCfg);

        ///////////////////////////////////////////////
        // DENOSING SIGNAL BLOCK
        ///////////////////////////////////////////////
        if (ringbuffer_len(&rbEcgDen) >= ECG_DEN_WINDOW_LEN) {
            tickUs = ns_us_ticker_read(&timerCfg);

            // Grab data from ringbuffer
            ringbuffer_peek(&rbEcgDen, ecgDenInout, ECG_DEN_WINDOW_LEN);

            // Preprocess signal
            pk_standardize_f32(ecgDenInout, ecgDenInout, ECG_DEN_WINDOW_LEN, NORM_STD_EPS);

            // Copy clean signal to buffer
            arm_copy_f32(ecgDenInout, ecgDenNoise, ECG_DEN_WINDOW_LEN);

            // Add noise to signal based on input
            if (sensorCtx.inputSource < NUM_INPUT_PTS) {
                nstdb_add_bw_noise(ecgDenInout, ecgDenInout, ECG_DEN_WINDOW_LEN, (float32_t)appState.bwNoiseLevel*2.0e-5);
                nstdb_add_ma_noise(ecgDenInout, ecgDenInout, ECG_DEN_WINDOW_LEN, (float32_t)appState.maNoiseLevel*1.0e-5);
                nstdb_add_em_noise(ecgDenInout, ecgDenInout, ECG_DEN_WINDOW_LEN, (float32_t)appState.emNoiseLevel*1.0e-5);
            }

            // Copy noisy signal to seg buffer
            ringbuffer_push(&rbEcgRawSeg, &ecgDenInout[ECG_DEN_PAD_LEN], ECG_DEN_VALID_LEN);

            // Apply biquad filter for DSP and AI modes
            if (appState.denoiseMode == DenoiseModeDsp) {
                err = pk_apply_biquad_filtfilt_f32(&ecgFilterCtx, ecgDenInout, ecgDenInout, ECG_DEN_WINDOW_LEN, ecgDenScratch);
            }
            // Denoise using AI model
            else if (appState.denoiseMode == DenoiseModeAi) {
                err = ecg_denoise_inference(&ecgDenModelCtx, ecgDenInout, ecgDenInout, 0, ECG_DEN_THRESHOLD);
            } else {
                err = 0;
            }

            // Compute cosine similarity
            if (sensorCtx.inputSource < NUM_INPUT_PTS) {
                cosine_similarity_f32(
                    &ecgDenInout[ECG_DEN_PAD_LEN],
                    &ecgDenNoise[ECG_DEN_PAD_LEN],
                    ECG_DEN_VALID_LEN,
                    &appMetResults.denoiseCossim
                );
            // Skip for live sensor mode
            } else {
                appMetResults.denoiseCossim = 1.0;
            }
            appMetResults.denoiseCossim *= 100.0;

            // Copy clean signal to seg buffer
            ringbuffer_push(&rbEcgSeg, &ecgDenInout[ECG_DEN_PAD_LEN], ECG_DEN_VALID_LEN);

            // Seek ringbuffers
            ringbuffer_seek(&rbEcgDen, ECG_DEN_VALID_LEN);
            deltaUs = ns_us_ticker_read(&timerCfg) - tickUs;
            appMetResults.denoiseIps = 1000000.0/deltaUs;
            ns_lp_printf("<DENOISE Time: %d (err=%d) >\n", deltaUs/1000, err);
        }

        ///////////////////////////////////////////////
        // SEGMENTATION BLOCK
        ///////////////////////////////////////////////
        else if (ringbuffer_len(&rbEcgSeg) >= ECG_SEG_WINDOW_LEN) {
            tickUs = ns_us_ticker_read(&timerCfg);
            ringbuffer_peek(&rbEcgSeg, ecgSegInout, ECG_SEG_WINDOW_LEN);

            // pk_standardize_f32(ecgSegInout, ecgSegInout, ECG_SEG_WINDOW_LEN, NORM_STD_EPS);

            if (appState.segMode == SegmentationModeDsp) {
                err = ecg_physiokit_segmentation_inference(ecgSegInout, ecgSegMask, 0);
            } else if (appState.segMode == SegmentationModeAi) {
                err = ecg_segmentation_inference(&ecgSegModelCtx, ecgSegInout, ecgSegMask, 0, ECG_SEG_THRESHOLD);
            } else{
                err = 0;
                for (size_t i = 0; i < ECG_SEG_WINDOW_LEN; i++) {
                    ecgSegMask[i] = ECG_SEG_NONE;
                }
            }

            // Push seg mask to Tx
            ringbuffer_transfer(&rbEcgRawSeg, &rbEcgRawTx, ECG_SEG_VALID_LEN);
            ringbuffer_push(&rbEcgDenTx, &ecgSegInout[ECG_SEG_PAD_LEN], ECG_SEG_VALID_LEN);
            ringbuffer_push(&rbEcgMaskTx, &ecgSegMask[ECG_SEG_PAD_LEN], ECG_SEG_VALID_LEN);

            // Push ecg and mask to metrics
            ringbuffer_push(&rbEcgMet, &ecgSegInout[ECG_SEG_PAD_LEN], ECG_SEG_VALID_LEN);
            ringbuffer_push(&rbEcgMaskMet, &ecgSegMask[ECG_SEG_PAD_LEN], ECG_SEG_VALID_LEN);

            ringbuffer_seek(&rbEcgSeg, ECG_SEG_VALID_LEN);
            deltaUs = ns_us_ticker_read(&timerCfg) - tickUs;
            appMetResults.segmentIps = 1000000.0/deltaUs;
            ns_lp_printf("<SEGMENT Time: %d (err=%d) >\n", deltaUs/1000, err);
        }

        ///////////////////////////////////////////////
        // ARRHYTHMIA/METRICS BLOCK
        ///////////////////////////////////////////////
        else if (MIN(ringbuffer_len(&rbEcgMet), ringbuffer_len(&rbEcgMaskMet)) >= ECG_MET_WINDOW_LEN) {
            tickUs = ns_us_ticker_read(&timerCfg);

            // Grab data from ringbuffers
            ringbuffer_peek(&rbEcgMet, ecgMetData, ECG_MET_WINDOW_LEN);
            ringbuffer_peek(&rbEcgMaskMet, ecgMaskMetData, ECG_MET_WINDOW_LEN);

            // Compute metrics
            err = metrics_capture_ecg(
                &metricsCfg,
                ecgMetData, ecgMaskMetData, ECG_MET_WINDOW_LEN,
                &appMetResults
            );

            if (appState.arrMode == ArrhythmiaModeDsp) {
                appMetResults.arrhythmiaLabel = appMetResults.hr < 40 ? ECG_ARR_SB : appMetResults.hr > 100 ? ECG_ARR_GSVT : ECG_ARR_SR;
                ns_lp_printf("HR: %f\n", appMetResults.hr);
            } else if (appState.arrMode == ArrhythmiaModeAi) {
                appMetResults.arrhythmiaLabel = ecg_arrhythmia_inference(&ecgArrModelCtx, ecgMetData, ECG_ARR_THRESHOLD);
            } else {
                appMetResults.arrhythmiaLabel = 0;
            }
            deltaUs = ns_us_ticker_read(&timerCfg) - tickUs;
            appMetResults.arrhythmiaIps = 1000000.0/deltaUs;

            appMetResults.cpuPercUtil = 100.0*cpuBusyMs/(cpuBusyMs + cpuIdleMs);
            cpuBusyMs = 0;
            cpuIdleMs = 0;

            // Store metrics
            ringbuffer_seek(&rbEcgMet, ECG_MET_VALID_LEN);
            ringbuffer_seek(&rbEcgMaskMet, ECG_MET_VALID_LEN);

            // Broadcast metrics
            send_slot0_metrics();
            send_uio_state();
            ns_lp_printf("<METRICS Time: %d (err=%d) >\n", deltaUs/1000, err);
        }

        // Send slot0 signals
        send_slot0_signals();

        // Try to maintain 100ms loop to not back up Tileio
        deltaUs = ns_us_ticker_read(&timerCfg);
        cpuBusyMs += deltaUs/1000;
        if (deltaUs < 100000) {
            delayUs = 100000 - deltaUs;
            cpuIdleMs += delayUs/1000;
            vTaskDelay(pdMS_TO_TICKS(delayUs/1000));
        }
    }
}

void setup_task(void *pvParameters) {
    tio_start(&tioCtx);
    xTaskCreate(TioTask, "TioTask", 512, NULL, 3, &tioTaskHandle);
    xTaskCreate(SensorTask, "SensorTask", 512, 0, 3, &sensorTaskHandle);
    xTaskCreate(ProcessTask, "ProcessTask", 3072, 0, 1, &processTaskHandle);
    send_uio_state();
    vTaskSuspend(NULL);
    while (1) { };
}

int main(void) {

    sensorCtx.inputSource = appState.inputSource;
    nsPwrCfg.eAIPowerMode = appState.speedMode ? NS_MAXIMUM_PERF : NS_MINIMUM_PERF;
    tioCtx.slot_update_cb = &received_slot_data;
    tioCtx.uio_update_cb = &received_uio_state;

    NS_TRY(ns_core_init(&nsCoreCfg), "Core Init failed.\b");
    NS_TRY(ns_power_config(&nsPwrCfg), "Power Init Failed\n");
    NS_TRY(ns_i2c_interface_init(&nsI2cCfg, I2C_SPEED_HZ), "I2C Init Failed\n");
    NS_TRY(ns_timer_init(&timerCfg), "Timer Init failed.\n");
    NS_TRY(ns_timer_init(&timer2Cfg), "Timer 2 Init failed.\n");
    NS_TRY(ns_peripheral_button_init(&nsBtnCfg), "Button Init failed.\n");
    am_devices_led_array_init(am_bsp_psLEDs, AM_BSP_NUM_LEDS);
    am_devices_led_array_out(am_bsp_psLEDs, AM_BSP_NUM_LEDS, 0);
    NS_TRY(ledstick_init(&nsI2cCfg, LEDSTICK_ADDR), "Led Stick Init Failed\n");
    NS_TRY(sensor_init(&sensorCtx), "Sensor Init Failed\n");
    NS_TRY(tio_init(&tioCtx), "TIO Init Failed\n");
    NS_TRY(tflm_init(), "TFLM Init Failed\n");
    NS_TRY(ecg_denoise_init(&ecgDenModelCtx), "ECG Segmentation Init Failed\n");
    NS_TRY(ecg_segmentation_init(&ecgSegModelCtx), "ECG Segmentation Init Failed\n");
    NS_TRY(ecg_arrhythmia_init(&ecgArrModelCtx), "ECG Arrhythmia Init Failed\n");
    NS_TRY(metrics_init(&metricsCfg), "Metrics Init Failed\n");
    sensor_start(&sensorCtx);
    // ledstick_set_all_colors(&nsI2cCfg, LEDSTICK_ADDR, 0, 207, 193);
    // ledstick_set_all_brightness(&nsI2cCfg, LEDSTICK_ADDR, 15);
    ns_itm_printf_enable();
    ns_interrupt_master_enable();

    xTaskCreate(setup_task, "Setup", 512, 0, 3, &appSetupTask);
    vTaskStartScheduler();
    while (1) { };
}

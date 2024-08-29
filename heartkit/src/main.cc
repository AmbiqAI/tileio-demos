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
#include "ns_ble.h"
#include "ns_i2c.h"
#include "ns_peripherals_power.h"
#include "FreeRTOS.h"
#include "task.h"
#include "arm_math.h"

#include "am_devices_led.h"

#include "constants.h"
#include "store.h"
#include "ledstick.h"
#include "sensor.h"
#include "nstdb_noise.h"
#include "physiokit/pk_filter.h"
#include "physiokit/pk_ecg.h"
#include "tflm.h"
#include "ecg_segmentation.h"
#include "ecg_denoise.h"
#include "metrics.h"
#include "ringbuffer.h"
#include "main.h"

#if (configAPPLICATION_ALLOCATED_HEAP == 1)
    #define APP_HEAP_SIZE (NS_BLE_DEFAULT_MALLOC_K * 4 * 1024)
size_t ucHeapSize = APP_HEAP_SIZE;
uint8_t ucHeap[APP_HEAP_SIZE] __attribute__((aligned(4)));
#endif


// RTOS Tasks
TaskHandle_t radioTaskHandle;
TaskHandle_t sensorTaskHandle;
TaskHandle_t slot0TaskHandle;
TaskHandle_t appSetupTask;

void
set_leds_state() {
    uint32_t ledVal = uioState.io7 & 0x07;
    am_devices_led_array_out(am_bsp_psLEDs, AM_BSP_NUM_LEDS, ledVal);
}


void
fetch_leds_state() {
    uint8_t ledVal = (am_devices_led_get(am_bsp_psLEDs, 2) << 2);
    ledVal |= (am_devices_led_get(am_bsp_psLEDs, 1) << 1);
    ledVal |= am_devices_led_get(am_bsp_psLEDs, 0);
    uioState.io7 = ledVal;
}

////////////////////////////////////////////////////////////////
// BLE
////////////////////////////////////////////////////////////////

void webbleHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg) {
    ns_lp_printf("webbleHandler\n");
}
void webbleHandlerInit(wsfHandlerId_t handlerId) {
    ns_lp_printf("webbleHandlerInit\n");
}

int ble_read_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c, void *dest) {
    memcpy(dest, c->applicationValue, c->valueLen);
    return NS_STATUS_SUCCESS;
}

int ble_write_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c, void *src) {
    memcpy(c->applicationValue, src, c->valueLen);
    if (c == bleCtx.uioChar) {
        uioState.io0 = bleCtx.uioBuffer[0];
        uioState.io1 = bleCtx.uioBuffer[1];
        uioState.io2 = bleCtx.uioBuffer[2];
        uioState.io3 = bleCtx.uioBuffer[3];
        uioState.io4 = bleCtx.uioBuffer[4];
        uioState.io5 = bleCtx.uioBuffer[5];
        uioState.io6 = bleCtx.uioBuffer[6];
        uioState.io7 = bleCtx.uioBuffer[7];
        set_leds_state();
    }
    return NS_STATUS_SUCCESS;
}

void ble_send_slot0_signals() {

    uint32_t offset;
    float32_t ecgRaw, ecgDen;
    int16_t *bleBuffer = (int16_t*)bleCtx.slot0SigBuffer;
    size_t numSamples = MIN3(
        ringbuffer_len(&rbEcgRawTx),
        ringbuffer_len(&rbEcgDenTx),
        ringbuffer_len(&rbEcgMaskTx)
    );

    if (numSamples >= BLE_SLOT0_SIG_NUM_VALS) {
        bleBuffer[0] = BLE_SLOT0_SIG_NUM_VALS;
        offset = 1;
        for (size_t i = 0; i < BLE_SLOT0_SIG_NUM_VALS; i++) {
            ringbuffer_pop(&rbEcgMaskTx, &bleBuffer[offset + 0], 1);
            ringbuffer_pop(&rbEcgRawTx, &ecgRaw, 1);
            ringbuffer_pop(&rbEcgDenTx, &ecgDen, 1);
            bleBuffer[offset + 1] = (int16_t)CLIP(BLE_SLOT0_SCALE*ecgRaw, -32768, 32767);
            bleBuffer[offset + 2] = (int16_t)CLIP(BLE_SLOT0_SCALE*ecgDen, -32768, 32767);
            offset += 3;
        }
        ns_ble_send_value(bleCtx.slot0SigChar, NULL);
    }
}

void ble_send_slot1_signals() {
}

void ble_send_slot0_metrics() {
    uint16_t *metBuffer = (uint16_t*)bleCtx.slot0MetBuffer;
    float32_t *metResults = (float32_t *)(&metBuffer[1]);
    metBuffer[0] = 5;
    metResults[0] = ecgMetResults.hr;
    metResults[1] = ecgMetResults.hrv;
    metResults[2] = ecgMetResults.denoise_ips;
    metResults[3] = ecgMetResults.segment_ips;
    metResults[4] = ecgMetResults.denoise_cossim;
    ns_ble_send_value(bleCtx.slot0MetChar, NULL);
}

void ble_send_slot1_metrics() {
}

int ble_notify_slot0_sig_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c) {
    ble_send_slot0_signals();
    return NS_STATUS_SUCCESS;
}

int ble_notify_slot1_sig_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c) {
    ble_send_slot1_signals();
    return NS_STATUS_SUCCESS;
}

int ble_notify_slot0_met_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c) {
    // ble_send_slot0_metrics();
    return NS_STATUS_SUCCESS;
}

int ble_notify_slot1_met_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c) {
    // ble_send_slot1_metrics();
    return NS_STATUS_SUCCESS;
}

int ble_service_init(void) {
    // Initialize BLE service
    char bleName[] = "HeartKit";
    NS_TRY(ns_ble_char2uuid(TIO_SLOT_SVC_UUID, &(bleCtx.service->uuid128)), "Failed to convert UUID\n");
    memcpy(bleCtx.service->name, bleName, sizeof(bleName));
    bleCtx.service->nameLen = strlen(bleName);
    bleCtx.service->baseHandle = 0x0800;
    bleCtx.service->poolConfig = bleCtx.pool;
    bleCtx.service->numAttributes = 0;

    // SLOT0: ECG
    ns_ble_create_characteristic(
        bleCtx.slot0SigChar, TIO_SLOT0_SIG_CHAR_UUID, bleCtx.slot0SigBuffer, TIO_BLE_SLOT_SIG_BUF_LEN,
        NS_BLE_READ | NS_BLE_NOTIFY,
        NULL, NULL, ble_notify_slot0_sig_handler,
        1000/BLE_SLOT0_FS, true, &(bleCtx.service->numAttributes)
    );
    ns_ble_create_characteristic(
        bleCtx.slot0MetChar, TIO_SLOT0_MET_CHAR_UUID, bleCtx.slot0MetBuffer, TIO_BLE_SLOT_MET_BUF_LEN,
        NS_BLE_READ | NS_BLE_NOTIFY,
        NULL, NULL, ble_notify_slot0_met_handler,
        1000*MET_CAPTURE_SEC, true, &(bleCtx.service->numAttributes)
    );

    // UIO
    ns_ble_create_characteristic(
        bleCtx.uioChar, TIO_UIO_CHAR_UUID, bleCtx.uioBuffer, TIO_BLE_UIO_BUF_LEN,
        NS_BLE_READ | NS_BLE_WRITE | NS_BLE_NOTIFY,
        &ble_read_handler, &ble_write_handler, NULL,
        1000, true, &(bleCtx.service->numAttributes)
    );

    bleCtx.service->numCharacteristics = BLE_NUM_CHARS;
    ns_ble_create_service(bleCtx.service);
    ns_ble_add_characteristic(bleCtx.service, bleCtx.slot0SigChar);
    ns_ble_add_characteristic(bleCtx.service, bleCtx.slot0MetChar);
    ns_ble_add_characteristic(bleCtx.service, bleCtx.uioChar);
    // Initialize BLE, create structs, start service
    ns_ble_start_service(bleCtx.service);
    return NS_STATUS_SUCCESS;
}

void RadioTask(void *pvParameters) {
    NS_TRY(ble_service_init(), "BLE init failed.\n");
    ledstick_set_all_colors(&nsI2cCfg, LEDSTICK_ADDR, 0, 207, 193);
    ledstick_set_all_brightness(&nsI2cCfg, LEDSTICK_ADDR, 4);
    while (1) { wsfOsDispatcher(); }
}

uint32_t
capture_sensor() {
    int32_t val;
    size_t numSamples;
    float32_t ecgVal;
    numSamples = sensor_capture_data(&sensorCtx);
    // ns_lp_printf("Captured %d samples\n", numSamples);
    for (size_t i = 0; i < numSamples; i++) {
        val  = sensorCtx.buffer[sensorCtx.maxCfg->numSlots * i + SENSOR_ECG_SLOT];
        ecgVal  = val & (1 << 17) ? val - (1 << 18) : val; // 2's complement
        ringbuffer_push(&rbEcgSensor, &ecgVal, 1);
    }
    return numSamples;
}

void flush_pipeline() {
    ringbuffer_flush(&rbEcgSensor);
    ringbuffer_flush(&rbEcgDen);
    ringbuffer_flush(&rbEcgSeg);
    ringbuffer_flush(&rbEcgMet);
    ringbuffer_flush(&rbEcgMaskMet);
    ringbuffer_flush(&rbEcgRawTx);
    ringbuffer_flush(&rbEcgDenTx);
    ringbuffer_flush(&rbEcgMaskTx);
}

void preprocess() {
    int32_t val;
    size_t numSamples;
    float32_t ecgVal;
    if (sensorCtx.input_source != uioState.io0) {
        sensorCtx.input_source = uioState.io0;
        ns_lp_printf("Button IO0 toggled: %d\n", sensorCtx.input_source);
        flush_pipeline();
    }
    // Downsample sensor data to slots
    numSamples = ringbuffer_len(&rbEcgSensor);
    for (size_t i = 0; i < numSamples/ECG_DS_RATE; i++) {
        ringbuffer_seek(&rbEcgSensor, ECG_DS_RATE - 1);
        ringbuffer_transfer(&rbEcgSensor, &rbEcgDen, 1);
    }
}

void SensorTask(void *pvParameters) {
    while (true) {
        capture_sensor();
        preprocess();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void Slot0Task(void *pvParameters) {
    uint32_t err;
    size_t numSamples;
    uint32_t lastTickUs = 0, delayUs = 0;
    int32_t noiseLevel, bwNoise, emNoise, maNoise;
    bool doneStep = false;
    while (true) {
        doneStep = false;
        ns_timer_clear(&timerCfg);
        lastTickUs = 0;

        // Capture sensor data
        numSamples = capture_sensor();
        preprocess();
        lastTickUs = ns_us_ticker_read(&timerCfg);

        // Perform ECG denoise
        numSamples = ringbuffer_len(&rbEcgDen);
        if (numSamples >= ECG_DEN_WINDOW_LEN) {

            // Dont add noise to user input (noisy already)
            if (uioState.io0 < NUM_INPUT_PTS) {
                noiseLevel = uioState.io1;
            } else {
                noiseLevel = 0;
            }

            // Grab data from ringbuffer
            ringbuffer_peek(&rbEcgDen, ecgDenInout, ECG_DEN_WINDOW_LEN);

            // Preprocess signal
            pk_standardize_f32(ecgDenInout, ecgDenInout, ECG_DEN_WINDOW_LEN, NORM_STD_EPS);

            // Add noise to signal
            if (noiseLevel > 0) {
                // Randomly split noiseLevel into three parts
                noiseLevel = MAX(3, noiseLevel);
                bwNoise = rand() % noiseLevel;
                emNoise = rand() % (noiseLevel - bwNoise);
                maNoise = MAX(0, noiseLevel - bwNoise - emNoise);
                nstdb_add_bw_noise(ecgDenInout, ECG_DEN_WINDOW_LEN, bwNoise*2e-5);
                nstdb_add_em_noise(ecgDenInout, ECG_DEN_WINDOW_LEN, emNoise*2e-5);
                nstdb_add_ma_noise(ecgDenInout, ECG_DEN_WINDOW_LEN, maNoise*5e-5);
            }

            // Copy noisy signal to Tx
            ringbuffer_push(&rbEcgRawTx, &ecgDenInout[ECG_DEN_PAD_LEN], ECG_DEN_VALID_LEN);

            // Copy noisy
            arm_copy_f32(ecgDenInout, ecgDenScratch, ECG_DEN_WINDOW_LEN);

            // Denoise signal
            pk_apply_biquad_filtfilt_f32(&ecgFilterCtx, ecgDenInout, ecgDenInout, ECG_DEN_WINDOW_LEN, ecgDenScratch);

            err = ecg_denoise_inference(&ecgDenModelCtx, ecgDenInout, ecgDenInout, 0, ECG_DEN_THRESHOLD);

            // Compute cosine similarity
            if (noiseLevel > 0) {
                cosine_similarity_f32(&ecgDenInout[ECG_DEN_PAD_LEN], &ecgDenScratch[ECG_DEN_PAD_LEN], ECG_DEN_VALID_LEN, &ecgMetResults.denoise_cossim);
            } else {
                ecgMetResults.denoise_cossim = 1.0;
            }

            // Copy denoised signal to Tx and Seg
            ringbuffer_push(&rbEcgSeg, &ecgDenInout[ECG_DEN_PAD_LEN], ECG_DEN_VALID_LEN);

            // Seek ringbuffers
            ringbuffer_seek(&rbEcgDen, ECG_DEN_VALID_LEN);
            lastTickUs = ns_us_ticker_read(&timerCfg) - lastTickUs;
            ecgMetResults.denoise_ips = 1000000.0/lastTickUs;
            ns_lp_printf("Denoise Time: %d (err=%d)\n", lastTickUs, err);
            doneStep = true;
        }

        // Perform ECG segmentation
        numSamples = ringbuffer_len(&rbEcgSeg);
        if (!doneStep && numSamples >= ECG_SEG_WINDOW_LEN) {
            ringbuffer_peek(&rbEcgSeg, ecgSegInout, ECG_SEG_WINDOW_LEN);

            // pk_standardize_f32(ecgSegInout, ecgSegInout, ECG_SEG_WINDOW_LEN, NORM_STD_EPS);

            err = ecg_segmentation_inference(&ecgSegModelCtx, ecgSegInout, ecgSegMask, 0, ECG_SEG_THRESHOLD);

            // Push seg mask to Tx
            ringbuffer_push(&rbEcgDenTx, &ecgSegInout[ECG_DEN_PAD_LEN], ECG_DEN_VALID_LEN);
            ringbuffer_push(&rbEcgMaskTx, &ecgSegMask[ECG_SEG_PAD_LEN], ECG_SEG_VALID_LEN);

            // Push ecg and mask to metrics
            ringbuffer_push(&rbEcgMet, &ecgSegInout[ECG_SEG_PAD_LEN], ECG_SEG_VALID_LEN);
            ringbuffer_push(&rbEcgMaskMet, &ecgSegMask[ECG_SEG_PAD_LEN], ECG_SEG_VALID_LEN);

            ringbuffer_seek(&rbEcgSeg, ECG_SEG_VALID_LEN);
            lastTickUs = ns_us_ticker_read(&timerCfg) - lastTickUs;
            ecgMetResults.segment_ips = 1000000.0/lastTickUs;
            ns_lp_printf("Segment Time:  %d (err=%d)\n", lastTickUs, err);
            doneStep = true;
        }

        // Perform metrics
        numSamples = MIN(ringbuffer_len(&rbEcgMet), ringbuffer_len(&rbEcgMaskMet));
        if (!doneStep && numSamples >= ECG_MET_WINDOW_LEN) {
            // Grab data from ringbuffers
            ringbuffer_peek(&rbEcgMet, ecgMetData, ECG_MET_WINDOW_LEN);
            ringbuffer_peek(&rbEcgMaskMet, ecgMaskMetData, ECG_MET_WINDOW_LEN);
            // Compute metrics
            metrics_capture_ecg(
                &metricsCfg,
                ecgMetData, ecgMaskMetData, ECG_MET_WINDOW_LEN,
                &ecgMetResults
            );
            // Broadcast metrics
            ble_send_slot0_metrics();
            // Store metrics
            ringbuffer_seek(&rbEcgMet, ECG_MET_VALID_LEN);
            ringbuffer_seek(&rbEcgMaskMet, ECG_MET_VALID_LEN);
            lastTickUs = ns_us_ticker_read(&timerCfg) - lastTickUs;
            ns_lp_printf("Metrics Time: %d\n", lastTickUs);
            doneStep = true;
        }
        lastTickUs = ns_us_ticker_read(&timerCfg);

        // Target 100 ms (delay if faster)
        if (lastTickUs < 100000) {
            delayUs = 100000 - lastTickUs;
            vTaskDelay(pdMS_TO_TICKS(delayUs/1000));
        } else{
            ns_lp_printf("Delay: 0 ms!!!!\n");

        }
    }
}

void setup_task(void *pvParameters) {
    ns_ble_pre_init();
    xTaskCreate(RadioTask, "RadioTask", 512, 0, 3, &radioTaskHandle);
    // xTaskCreate(SensorTask, "SensorTask", 512, 0, 3, &sensorTaskHandle);
    xTaskCreate(Slot0Task, "Slot0Task", 2560, 0, 1, &slot0TaskHandle);
    vTaskSuspend(NULL);
    while (1) { };
}

int main(void) {
    NS_TRY(ns_core_init(&nsCoreCfg), "Core Init failed.\b");
    NS_TRY(ns_power_config(&nsPwrCfg), "Power Init Failed\n");
    NS_TRY(ns_i2c_interface_init(&nsI2cCfg, I2C_SPEED_HZ), "I2C Init Failed\n");
    NS_TRY(ns_timer_init(&timerCfg), "Timer0 Init failed.\n");

    // NS_TRY(ns_timer_init(&slot1TimerCfg), "Timer1 Init failed.\n");
    NS_TRY(ns_peripheral_button_init(&nsBtnCfg), "Button Init failed.\n");
    am_devices_led_array_init(am_bsp_psLEDs, AM_BSP_NUM_LEDS);
    am_devices_led_array_out(am_bsp_psLEDs, AM_BSP_NUM_LEDS, 7);
    NS_TRY(ledstick_init(&nsI2cCfg, LEDSTICK_ADDR), "Led Stick Init Failed\n");
    NS_TRY(sensor_init(&sensorCtx), "Sensor Init Failed\n");

    NS_TRY(tflm_init(), "TFLM Init Failed\n");
    NS_TRY(ecg_denoise_init(&ecgDenModelCtx), "ECG Segmentation Init Failed\n");
    NS_TRY(ecg_segmentation_init(&ecgSegModelCtx), "ECG Segmentation Init Failed\n");
    NS_TRY(metrics_init(&metricsCfg), "Metrics Init Failed\n");

    sensor_start(&sensorCtx);

    ns_itm_printf_enable();
    ns_interrupt_master_enable();

    xTaskCreate(setup_task, "Setup", 512, 0, 3, &appSetupTask);
    vTaskStartScheduler();
    while (1) { };
}

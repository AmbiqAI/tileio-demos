/**
 * @file main.c
 * @author Adam Page (adam.page@ambiq.com)
 * @brief HeartKit: ECG Denoise app
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
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
TaskHandle_t denoiseTaskHandle;
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
        sensorCtx.is_live = uioState.io0;
        set_leds_state();
    }
    return NS_STATUS_SUCCESS;
}

void ble_send_slot0_signals() {
    uint32_t offset;
    float32_t ecgRaw, ecgNos, ecgDen;
    int16_t *bleBuffer = (int16_t*)bleCtx.slot0SigBuffer;
    size_t numSamples = MIN4(
        ringbuffer_len(&rbEcgRawTx),
        ringbuffer_len(&rbEcgNosTx),
        ringbuffer_len(&rbEcgDenTx),
        ringbuffer_len(&rbEcgMaskTx)
    );
    if (numSamples >= BLE_SLOT0_SIG_NUM_VALS) {
        bleBuffer[0] = BLE_SLOT0_SIG_NUM_VALS;
        offset = 1;
        for (size_t i = 0; i < BLE_SLOT0_SIG_NUM_VALS; i++) {
            ringbuffer_pop(&rbEcgMaskTx, &bleBuffer[offset + 0], 1);
            ringbuffer_pop(&rbEcgRawTx, &ecgRaw, 1);
            ringbuffer_pop(&rbEcgNosTx, &ecgNos, 1);
            ringbuffer_pop(&rbEcgDenTx, &ecgDen, 1);
            bleBuffer[offset + 1] = (int16_t)CLIP(BLE_SLOT0_SCALE*ecgRaw, -32768, 32767);
            bleBuffer[offset + 2] = (int16_t)CLIP(BLE_SLOT0_SCALE*ecgNos, -32768, 32767);
            bleBuffer[offset + 3] = (int16_t)CLIP(BLE_SLOT0_SCALE*ecgDen, -32768, 32767);
            offset += 4;
        }
        ns_ble_send_value(bleCtx.slot0SigChar, NULL);
    }
}


void ble_send_slot1_signals() {
    // uint32_t offset;
    // float32_t ecg;
    // int16_t *bleBuffer = (int16_t*)bleCtx.slot1SigBuffer;
    // size_t numSamples = MIN(
    //     ringbuffer_len(&rbEcgNosTx),
    //     ringbuffer_len(&rbEcgNosMaskTx)
    // );
    // if (numSamples >= BLE_SLOT1_SIG_NUM_VALS) {
    //     bleBuffer[0] = BLE_SLOT1_SIG_NUM_VALS;
    //     offset = 1;
    //     for (size_t i = 0; i < BLE_SLOT1_SIG_NUM_VALS; i++) {
    //         ringbuffer_pop(&rbEcgNosMaskTx, &bleBuffer[offset + 0], 1);
    //         ringbuffer_pop(&rbEcgNosTx, &ecg, 1);
    //         bleBuffer[offset + 1] = (int16_t)CLIP(BLE_SLOT1_SCALE*ecg, -32768, 32767);
    //         offset += 2;
    //     }
    //     ns_ble_send_value(bleCtx.slot1SigChar, NULL);
    // }
}

void ble_send_slot2_signals() {
    // uint32_t offset;
    // float32_t ecg;
    // int16_t *bleBuffer = (int16_t*)bleCtx.slot2SigBuffer;
    // size_t numSamples = MIN(
    //     ringbuffer_len(&rbEcgDenTx),
    //     ringbuffer_len(&rbEcgDenMaskTx)
    // );
    // if (numSamples >= BLE_SLOT2_SIG_NUM_VALS) {
    //     bleBuffer[0] = BLE_SLOT2_SIG_NUM_VALS;
    //     offset = 1;
    //     for (size_t i = 0; i < BLE_SLOT2_SIG_NUM_VALS; i++) {
    //         ringbuffer_pop(&rbEcgDenMaskTx, &bleBuffer[offset + 0], 1);
    //         ringbuffer_pop(&rbEcgDenTx, &ecg, 1);
    //         bleBuffer[offset + 1] = (int16_t)CLIP(BLE_SLOT2_SCALE*ecg, -32768, 32767);
    //         offset += 2;
    //     }
    //     ns_ble_send_value(bleCtx.slot2SigChar, NULL);
    // }
}

void ble_send_slot3_signals() {
}

void ble_send_slot0_metrics() {
    uint16_t *metBuffer = (uint16_t*)bleCtx.slot0MetBuffer;
    float32_t *metResults = (float32_t *)(&metBuffer[1]);
    metBuffer[0] = 7;
    metResults[0] = ecgMetResults.hr;
    metResults[1] = ecgMetResults.hrv;
    metResults[2] = ecgMetResults.rawCosim;
    metResults[3] = ecgMetResults.rawMse;
    metResults[4] = ecgMetResults.denCosim;
    metResults[5] = ecgMetResults.denMse;
    metResults[6] = ecgMetResults.ips;
    ns_ble_send_value(bleCtx.slot0MetChar, NULL);
}

void ble_send_slot1_metrics() {
}

void ble_send_slot2_metrics() {
}

int ble_notify_slot0_sig_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c) {
    ble_send_slot0_signals();
    return NS_STATUS_SUCCESS;
}

int ble_notify_slot1_sig_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c) {
    ble_send_slot1_signals();
    return NS_STATUS_SUCCESS;
}

int ble_notify_slot2_sig_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c) {
    ble_send_slot2_signals();
    return NS_STATUS_SUCCESS;
}

int ble_notify_slot3_sig_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c) {
    ble_send_slot3_signals();
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

int ble_notify_slot2_met_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c) {
    // ble_send_slot2_metrics();
    return NS_STATUS_SUCCESS;
}

int ble_notify_slot3_met_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c) {
    // ble_send_slot3_metrics();
    return NS_STATUS_SUCCESS;
}


int ble_service_init(void) {
    // Initialize BLE service
    char bleName[] = "HK Denoise";
    NS_TRY(ns_ble_char2uuid(TIO_SLOT_SVC_UUID, &(bleCtx.service->uuid128)), "Failed to convert UUID\n");
    memcpy(bleCtx.service->name, bleName, sizeof(bleName));
    bleCtx.service->nameLen = strlen(bleName);
    bleCtx.service->baseHandle = 0x0800;
    bleCtx.service->poolConfig = bleCtx.pool;
    bleCtx.service->numAttributes = 0;

    // SLOT0: Raw ECG
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
        NS_BLE_READ | NS_BLE_WRITE,
        &ble_read_handler, &ble_write_handler, NULL,
        0, true, &(bleCtx.service->numAttributes)
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
    while (1) { wsfOsDispatcher(); }
}

void SensorTask(void *pvParameters) {
    int32_t val;
    size_t numSamples;
    float32_t ecgVal, ppg1Val, ppg2Val;
    while (true) {
        if (sensorCtx.is_live ^ uioState.io0) {
            sensorCtx.is_live = uioState.io0;
            ns_lp_printf("I/O 0 pressed (%d)\n", sensorCtx.is_live);
        }
        // Capture sensor data, filter, and enqueue
        numSamples = sensor_capture_data(&sensorCtx);
        for (size_t i = 0; i < numSamples; i++) {
            val = sensorCtx.buffer[sensorCtx.maxCfg->numSlots * i + SENSOR_ECG_SLOT];
            ecgVal = val & (1 << 17) ? val - (1 << 18) : val; // 2's complement
            ringbuffer_push(&rbEcgSensor, &ecgVal, 1);
        }
        // Downsample sensor data to slots
        numSamples = ringbuffer_len(&rbEcgSensor);
        for (size_t i = 0; i < numSamples/ECG_DS_RATE; i++) {
            ringbuffer_seek(&rbEcgSensor, ECG_DS_RATE - 1);
            ringbuffer_transfer(&rbEcgSensor, &rbEcgDen, 1);
        }
        // Delay for ~5 samples
        vTaskDelay(pdMS_TO_TICKS(5*(1000/SENSOR_RATE)));
    }
}

void DenoiseTask(void *pvParameters) {
    uint32_t lastTickUs = 0;
    uint32_t err;
    float32_t ecgMin, ecgMax;
    size_t numSamples;
    while (true) {
        ns_timer_clear(&tickTimerCfg);
        // Perform ECG denoise
        numSamples = ringbuffer_len(&rbEcgDen);
        if (numSamples >= ECG_DEN_WINDOW_LEN) {
            float32_t bwNoise = (float32_t)(uioState.io1)*2e-5;
            float32_t emNoise = (float32_t)(uioState.io2)*2e-5;
            float32_t maNoise = (float32_t)(uioState.io3)*2e-5;

            // Grab data from ringbuffer
            ringbuffer_peek(&rbEcgDen, ecgDenInout, ECG_DEN_WINDOW_LEN);

            // Preprocess signal
            arm_min_f32(ecgDenInout, ECG_DEN_WINDOW_LEN, &ecgMin, &lastTickUs);
            arm_max_f32(ecgDenInout, ECG_DEN_WINDOW_LEN, &ecgMax, &lastTickUs);
            pk_standardize_f32(ecgDenInout, ecgDenInout, ECG_DEN_WINDOW_LEN, NORM_STD_EPS);

            // Copy raw signal to Tx and metrics
            ringbuffer_push(&rbEcgRawTx, &ecgDenInout[ECG_DEN_PAD_LEN], ECG_DEN_VALID_LEN);
            ringbuffer_push(&rbEcgRawMet, &ecgDenInout[ECG_DEN_PAD_LEN], ECG_DEN_VALID_LEN);

            // Add noise to signal
            nstdb_add_bw_noise(ecgDenInout, ECG_DEN_WINDOW_LEN, bwNoise);
            nstdb_add_em_noise(ecgDenInout, ECG_DEN_WINDOW_LEN, emNoise);
            nstdb_add_ma_noise(ecgDenInout, ECG_DEN_WINDOW_LEN, maNoise);

            // Copy noisy signal to Tx and metrics
            ringbuffer_push(&rbEcgNosTx, &ecgDenInout[ECG_DEN_PAD_LEN], ECG_DEN_VALID_LEN);
            ringbuffer_push(&rbEcgNosMet, &ecgDenInout[ECG_DEN_PAD_LEN], ECG_DEN_VALID_LEN);

            // Denoise signal
            err = ecg_denoise_inference(&ecgDenModelCtx, ecgDenInout, ecgDenInout, ecgDenMask, 0, ECG_DEN_THRESHOLD);

            // Copy denoised signal to Tx and metrics
            ringbuffer_push(&rbEcgDenTx, &ecgDenInout[ECG_DEN_PAD_LEN], ECG_DEN_VALID_LEN);
            ringbuffer_push(&rbEcgDenMet, &ecgDenInout[ECG_DEN_PAD_LEN], ECG_DEN_VALID_LEN);
            ringbuffer_fill(&rbEcgMaskMet, 0, ECG_DEN_VALID_LEN);
            ringbuffer_fill(&rbEcgMaskTx, 0, ECG_DEN_VALID_LEN);

            // Seek ringbuffers
            ringbuffer_seek(&rbEcgDen, ECG_DEN_VALID_LEN);
            lastTickUs = ns_us_ticker_read(&tickTimerCfg);
            ecgMetResults.ips = 1000000/lastTickUs;
            ns_lp_printf("Denoise Time: %d (err=%d) min=%0.2f, max=%0.2f\n", lastTickUs, err, ecgMin, ecgMax);
        }
        // Perform raw metrics
        numSamples = MIN4(
            ringbuffer_len(&rbEcgRawMet),
            ringbuffer_len(&rbEcgNosMet),
            ringbuffer_len(&rbEcgDenMet),
            ringbuffer_len(&rbEcgMaskMet)
        );

        if (numSamples >= ECG_MET_WINDOW_LEN) {
            ns_timer_clear(&tickTimerCfg);
            // Grab data from ringbuffers
            ringbuffer_peek(&rbEcgRawMet, ecgRawMetData, ECG_MET_WINDOW_LEN);
            ringbuffer_peek(&rbEcgNosMet, ecgNosMetData, ECG_MET_WINDOW_LEN);
            ringbuffer_peek(&rbEcgDenMet, ecgDenMetData, ECG_MET_WINDOW_LEN);
            ringbuffer_peek(&rbEcgMaskMet, ecgMaskMetData, ECG_MET_WINDOW_LEN);

            // Compute metrics
            metrics_capture_ecg(
                &metricsCfg,
                ecgRawMetData,
                ecgNosMetData,
                ecgDenMetData,
                ecgMaskMetData,
                ECG_MET_WINDOW_LEN,
                &ecgMetResults
            );

            // Broadcast metrics
            ble_send_slot0_metrics();

            // Seek ringbuffers
            ringbuffer_seek(&rbEcgRawMet, ECG_MET_VALID_LEN);
            ringbuffer_seek(&rbEcgNosMet, ECG_MET_VALID_LEN);
            ringbuffer_seek(&rbEcgDenMet, ECG_MET_VALID_LEN);
            ringbuffer_seek(&rbEcgMaskMet, ECG_MET_VALID_LEN);
            lastTickUs = ns_us_ticker_read(&tickTimerCfg);
            ns_lp_printf("Metrics Time: %d\n", lastTickUs);
        }
        vTaskDelay(pdMS_TO_TICKS(1000*ECG_DEN_VALID_LEN/ECG_SAMPLE_RATE/4));
    }
}

void setup_task(void *pvParameters) {
    ns_ble_pre_init();

    ledstick_set_all_colors(&nsI2cCfg, LEDSTICK_ADDR, 0, 207, 193);
    ledstick_set_all_brightness(&nsI2cCfg, LEDSTICK_ADDR, 15);
    sensor_start(&sensorCtx);

    xTaskCreate(RadioTask, "RadioTask", 512, 0, 3, &radioTaskHandle);
    xTaskCreate(SensorTask, "SensorTask", 512, 0, 3, &sensorTaskHandle);
    xTaskCreate(DenoiseTask, "DenoiseTask", 2048, 0, 3, &denoiseTaskHandle);
    vTaskSuspend(NULL);
    while (1) { };
}

int main(void) {
    NS_TRY(ns_core_init(&nsCoreCfg), "Core Init failed.\b");
    NS_TRY(ns_power_config(&nsPwrCfg), "Power Init Failed\n");
    NS_TRY(ns_i2c_interface_init(&nsI2cCfg, I2C_SPEED_HZ), "I2C Init Failed\n");
    NS_TRY(ns_timer_init(&tickTimerCfg), "Timer Init failed.\n");
    NS_TRY(ns_peripheral_button_init(&nsBtnCfg), "Button Init failed.\n");
    am_devices_led_array_init(am_bsp_psLEDs, AM_BSP_NUM_LEDS);
    set_leds_state();
    NS_TRY(ledstick_init(&nsI2cCfg, LEDSTICK_ADDR), "Led Stick Init Failed\n");
    NS_TRY(ecg_denoise_init(&ecgDenModelCtx), "Denoise Init Failed\n");
    NS_TRY(metrics_init(&metricsCfg), "Metrics Init Failed\n");
    NS_TRY(sensor_init(&sensorCtx), "Sensor Init Failed\n");

    ns_itm_printf_enable();
    ns_interrupt_master_enable();

    xTaskCreate(setup_task, "Setup", 512, 0, 3, &appSetupTask);
    vTaskStartScheduler();
    while (1) { };
}

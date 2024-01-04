/**
 * @file main.c
 * @author Adam Page (adam.page@ambiq.com)
 * @brief PhysioKit app
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
#include "physiokit/pk_filter.h"
#include "physiokit/pk_ecg.h"
#include "ecg_segmentation.h"
#include "metrics.h"
#include "ringbuffer.h"
#include "main.h"

#if (configAPPLICATION_ALLOCATED_HEAP == 1)
uint8_t ucHeap[NS_BLE_DEFAULT_MALLOC_K * 4 * 1024] __attribute__((aligned(4)));
#endif

// RTOS Tasks
TaskHandle_t radioTaskHandle;
TaskHandle_t sensorTaskHandle;
TaskHandle_t slot0TaskHandle;
TaskHandle_t slot1TaskHandle;
TaskHandle_t appSetupTask;


void
set_leds_state() {
    uint32_t ledVal = uioState.led2 << 2 | uioState.led1 << 1 | uioState.led0;
    am_devices_led_array_out(am_bsp_psLEDs, AM_BSP_NUM_LEDS, ledVal);
}

void
fetch_leds_state() {
    uioState.led0 = am_devices_led_get(am_bsp_psLEDs, 0);
    uioState.led1 = am_devices_led_get(am_bsp_psLEDs, 1);
    uioState.led2 = am_devices_led_get(am_bsp_psLEDs, 2);
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
        set_leds_state();
    }
    return NS_STATUS_SUCCESS;
}

void ble_send_slot0_signals() {
    uint32_t offset;
    float32_t ecgVal;
    int16_t *bleBuffer = (int16_t*)bleCtx.slot0SigBuffer;
    size_t numSamples = MIN(ringbuffer_len(&rbEcgTx), ringbuffer_len(&rbEcgMaskTx));
    if (numSamples >= BLE_SLOT0_SIG_NUM_VALS) {
        bleBuffer[0] = BLE_SLOT0_SIG_NUM_VALS;
        offset = 1;
        for (size_t i = 0; i < BLE_SLOT0_SIG_NUM_VALS; i++) {
            ringbuffer_pop(&rbEcgMaskTx, &bleBuffer[offset + 0], 1);
            ringbuffer_pop(&rbEcgTx, &ecgVal, 1);
            bleBuffer[offset + 1] = (int16_t)CLIP(BLE_SLOT0_SCALE*ecgVal, -32768, 32767);
            offset += 2;
        }
        ns_ble_send_value(bleCtx.slot0SigChar, NULL);
    }
}

void ble_send_slot1_signals() {
    uint32_t offset;
    float32_t ppg1Val, ppg2Val;
    int16_t *bleBuffer = (int16_t*)bleCtx.slot1SigBuffer;
    size_t numSamples = MIN3(
        ringbuffer_len(&rbPpg1Tx),
        ringbuffer_len(&rbPpg2Tx),
        ringbuffer_len(&rbPpgMaskTx)
    );
    if (numSamples >= BLE_SLOT1_SIG_NUM_VALS) {
        bleBuffer[0] = BLE_SLOT1_SIG_NUM_VALS;
        offset = 1;
        for (size_t i = 0; i < BLE_SLOT1_SIG_NUM_VALS; i++) {
            ringbuffer_pop(&rbPpgMaskTx, &bleBuffer[offset + 0], 1);
            ringbuffer_pop(&rbPpg1Tx, &ppg1Val, 1);
            ringbuffer_pop(&rbPpg2Tx, &ppg2Val, 1);
            bleBuffer[offset + 1] = (int16_t)CLIP(BLE_SLOT1_SCALE*ppg1Val, -32768, 32767);
            bleBuffer[offset + 2] = (int16_t)CLIP(BLE_SLOT1_SCALE*ppg2Val, -32768, 32767);
            offset += 3;
        }
        ns_ble_send_value(bleCtx.slot1SigChar, NULL);
    }
}

void ble_send_slot0_metrics() {
    uint16_t *metBuffer = (uint16_t*)bleCtx.slot0MetBuffer;
    float32_t *metResults = (float32_t *)(&metBuffer[1]);
    metBuffer[0] = 1;
    metResults[0] = ecgMetResults.hr;
    ns_ble_send_value(bleCtx.slot0MetChar, NULL);
}

void ble_send_slot1_metrics() {
    uint16_t *metBuffer = (uint16_t*)bleCtx.slot1MetBuffer;
    float32_t *metResults = (float32_t *)(&metBuffer[1]);
    metBuffer[0] = 2;
    metResults[0] = ppgMetResults.hr;
    metResults[1] = ppgMetResults.spo2;
    ns_ble_send_value(bleCtx.slot1MetChar, NULL);
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
    char bleName[] = "PhysioKit";
    NS_TRY(ns_ble_char2uuid(PK_SLOT_SVC_UUID, &(bleCtx.service->uuid128)), "Failed to convert UUID\n");
    memcpy(bleCtx.service->name, bleName, sizeof(bleName));
    bleCtx.service->nameLen = strlen(bleName);
    bleCtx.service->baseHandle = 0x0800;
    bleCtx.service->poolConfig = bleCtx.pool;
    bleCtx.service->numAttributes = 0;

    // SLOT0
    ns_ble_create_characteristic(
        bleCtx.slot0SigChar, PK_SLOT0_SIG_CHAR_UUID, bleCtx.slot0SigBuffer, BLE_SLOT_SIG_BUF_LEN,
        NS_BLE_READ | NS_BLE_NOTIFY,
        NULL, NULL, ble_notify_slot0_sig_handler,
        1000/BLE_SLOT0_FS, true, &(bleCtx.service->numAttributes)
    );
    ns_ble_create_characteristic(
        bleCtx.slot0MetChar, PK_SLOT0_MET_CHAR_UUID, bleCtx.slot0MetBuffer, BLE_SLOT_MET_BUF_LEN,
        NS_BLE_READ | NS_BLE_NOTIFY,
        NULL, NULL, ble_notify_slot0_met_handler,
        1000*MET_CAPTURE_SEC, true, &(bleCtx.service->numAttributes)
    );

    // SLOT1
    ns_ble_create_characteristic(
        bleCtx.slot1SigChar, PK_SLOT1_SIG_CHAR_UUID, bleCtx.slot1SigBuffer, BLE_SLOT_SIG_BUF_LEN,
        NS_BLE_READ | NS_BLE_NOTIFY,
        NULL, NULL, ble_notify_slot1_sig_handler,
        1000/BLE_SLOT1_FS, true, &(bleCtx.service->numAttributes)
    );
    ns_ble_create_characteristic(
        bleCtx.slot1MetChar, PK_SLOT1_MET_CHAR_UUID, bleCtx.slot1MetBuffer, BLE_SLOT_MET_BUF_LEN,
        NS_BLE_READ | NS_BLE_NOTIFY,
        NULL, NULL, ble_notify_slot1_met_handler,
        1000*MET_CAPTURE_SEC, true, &(bleCtx.service->numAttributes)
    );

    // UIO
    ns_ble_create_characteristic(
        bleCtx.uioChar, PK_UIO_CHAR_UUID, bleCtx.uioBuffer, BLE_UIO_BUF_LEN,
        NS_BLE_READ | NS_BLE_WRITE,
        &ble_read_handler, &ble_write_handler, NULL,
        0, true, &(bleCtx.service->numAttributes)
    );

    bleCtx.service->numCharacteristics = BLE_NUM_CHARS;
    ns_ble_create_service(bleCtx.service);
    ns_ble_add_characteristic(bleCtx.service, bleCtx.slot0SigChar);
    ns_ble_add_characteristic(bleCtx.service, bleCtx.slot0MetChar);
    ns_ble_add_characteristic(bleCtx.service, bleCtx.slot1SigChar);
    ns_ble_add_characteristic(bleCtx.service, bleCtx.slot1MetChar);
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
        if (sensorCtx.is_stimulus ^ uioState.btn0) {
            sensorCtx.is_stimulus = uioState.btn0;
            ns_lp_printf("Button 0 pressed (%d)\n", sensorCtx.is_stimulus);
        }
        // Capture sensor data, filter, and enqueue
        numSamples = sensor_capture_data(&sensorCtx);
        for (size_t i = 0; i < numSamples; i++) {
            ppg1Val = sensorCtx.buffer[sensorCtx.maxCfg->numSlots * i + SENSOR_PPG1_SLOT];
            ppg2Val = sensorCtx.buffer[sensorCtx.maxCfg->numSlots * i + SENSOR_PPG2_SLOT];
            val  = sensorCtx.buffer[sensorCtx.maxCfg->numSlots * i + SENSOR_ECG_SLOT];
            ecgVal  = val & (1 << 17) ? val - (1 << 18) : val; // 2's complement
            // Filter sensor data
            // arm_biquad_cascade_df1_f32(&ppg1FilterCtx, &ppg1Val, &ppg1Val, 1);
            // arm_biquad_cascade_df1_f32(&ppg2FilterCtx, &ppg2Val, &ppg2Val, 1);
            // arm_biquad_cascade_df1_f32(&ecgFilterCtx, &ecgVal, &ecgVal, 1);
            ringbuffer_push(&rbPpg1Sensor, &ppg1Val, 1);
            ringbuffer_push(&rbPpg2Sensor, &ppg2Val, 1);
            ringbuffer_push(&rbEcgSensor, &ecgVal, 1);
        }
        // Downsample sensor data to slots
        numSamples = ringbuffer_len(&rbEcgSensor);
        for (size_t i = 0; i < numSamples/ECG_DS_RATE; i++) {
            ringbuffer_seek(&rbEcgSensor, ECG_DS_RATE - 1);
            ringbuffer_transfer(&rbEcgSensor, &rbEcgSeg, 1);
        }
        numSamples = ringbuffer_len(&rbPpg1Sensor);
        for (size_t i = 0; i < numSamples/PPG_DS_RATE; i++) {
            ringbuffer_seek(&rbPpg1Sensor, PPG_DS_RATE - 1);
            ringbuffer_seek(&rbPpg2Sensor, PPG_DS_RATE - 1);
            ringbuffer_transfer(&rbPpg1Sensor, &rbPpg1Seg, 1);
            ringbuffer_transfer(&rbPpg2Sensor, &rbPpg2Seg, 1);
        }
        // Delay for ~16 samples
        vTaskDelay(pdMS_TO_TICKS(16*(1000/SENSOR_RATE)));
    }
}

void Slot0Task(void *pvParameters) {
    uint32_t lastTickUs = 0;
    uint32_t err;
    size_t numSamples;
    while (true) {
        // Perform ECG segmentation
        numSamples = ringbuffer_len(&rbEcgSeg);
        if (numSamples >= ECG_SEG_WINDOW_LEN) {
            ns_timer_clear(&tickTimerCfg);
            ringbuffer_peek(&rbEcgSeg, ecgSegInputs, ECG_SEG_WINDOW_LEN);
            pk_apply_biquad_filtfilt_f32(&ecgFilterCtx, ecgSegInputs, ecgSegInputs, ECG_SEG_WINDOW_LEN, ecgSegScratch);
            pk_standardize_f32(ecgSegInputs, ecgSegInputs, ECG_SEG_WINDOW_LEN, NORM_STD_EPS);
            err = ecg_segmentation_inference(&ecgSegModelCtx, ecgSegInputs, ecgSegMask, 0, ECG_SEG_THRESHOLD);
            ringbuffer_push(&rbEcgMet, &ecgSegInputs[ECG_SEG_PAD_LEN], ECG_SEG_VALID_LEN);
            ringbuffer_push(&rbEcgMaskMet, &ecgSegMask[ECG_SEG_PAD_LEN], ECG_SEG_VALID_LEN);
            ringbuffer_seek(&rbEcgSeg, ECG_SEG_VALID_LEN);
            ns_lp_printf("Segmentation Time: %d\n", lastTickUs);
        }
        // Perform metrics
        numSamples = MIN(ringbuffer_len(&rbEcgMet), ringbuffer_len(&rbEcgMaskMet));
        if (numSamples >= ECG_MET_WINDOW_LEN) {
            ns_timer_clear(&tickTimerCfg);
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
            ringbuffer_push(&rbEcgTx, ecgMetData, ECG_MET_VALID_LEN);
            ringbuffer_push(&rbEcgMaskTx, ecgMaskMetData, ECG_MET_VALID_LEN);
            ringbuffer_seek(&rbEcgMet, ECG_MET_VALID_LEN);
            ringbuffer_seek(&rbEcgMaskMet, ECG_MET_VALID_LEN);
            lastTickUs = ns_us_ticker_read(&tickTimerCfg);
            ns_lp_printf("Metrics Time: %d\n", lastTickUs);
        }
        vTaskDelay(pdMS_TO_TICKS(1000*ECG_SEG_VALID_LEN/ECG_SAMPLE_RATE/4));
    }
}

void Slot1Task(void *pvParameters) {
    uint32_t lastTickUs = 0;
    uint32_t err;
    size_t numSamples;
    while (true) {
        // Perform PPG segmentation
        numSamples = MIN(ringbuffer_len(&rbPpg1Seg), ringbuffer_len(&rbPpg2Seg));
        if (numSamples >= PPG_SEG_WINDOW_LEN) {
            ringbuffer_peek(&rbPpg1Seg, ppgSegInputs, PPG_SEG_WINDOW_LEN);
            // pk_apply_biquad_filter_f32(&ppg1FilterCtx, segInputs, segInputs, SEG_WINDOW_LEN);
            // pk_standardize_f32(segInputs, segInputs, SEG_WINDOW_LEN, NORM_STD_EPS);
            memset(ppgSegMask, 0, PPG_SEG_WINDOW_LEN*sizeof(uint16_t)); // TODO: Run PPG segmentation
            ringbuffer_push(&rbPpg1Met, &ppgSegInputs[PPG_SEG_PAD_LEN], PPG_SEG_VALID_LEN);
            ringbuffer_push(&rbPpgMaskMet, &ppgSegMask[PPG_SEG_PAD_LEN], PPG_SEG_VALID_LEN);
            ringbuffer_seek(&rbPpg1Seg, PPG_SEG_VALID_LEN);

            // Perform PPG2 segmentation
            ringbuffer_peek(&rbPpg2Seg, ppgSegInputs, PPG_SEG_WINDOW_LEN);
            // pk_apply_biquad_filter_f32(&ppg2FilterCtx, segInputs, segInputs, SEG_WINDOW_LEN);
            // pk_standardize_f32(segInputs, segInputs, SEG_WINDOW_LEN, NORM_STD_EPS);
            ringbuffer_push(&rbPpg2Met, &ppgSegInputs[PPG_SEG_PAD_LEN], PPG_SEG_VALID_LEN);
            ringbuffer_seek(&rbPpg2Seg, PPG_SEG_VALID_LEN);
            ns_lp_printf("Slot1 Segmentation\n");
        }

        // Perform metrics
        numSamples = MIN3(
            ringbuffer_len(&rbPpg1Met),
            ringbuffer_len(&rbPpg2Met),
            ringbuffer_len(&rbPpgMaskMet)
        );
        if (numSamples >= PPG_MET_WINDOW_LEN) {
            // Grab data from ringbuffers
            ringbuffer_peek(&rbPpg1Met, ppg1MetData, PPG_MET_WINDOW_LEN);
            ringbuffer_peek(&rbPpg2Met, ppg2MetData, PPG_MET_WINDOW_LEN);
            ringbuffer_peek(&rbPpgMaskMet, ppgMaskMetData, PPG_MET_WINDOW_LEN);

            // Compute metrics
            metrics_capture_ppg(
                &metricsCfg, ppg1MetData, ppg2MetData,
                ppgMaskMetData, PPG_MET_WINDOW_LEN,
                &ppgMetResults
            );
            // // Broadcast metrics
            ble_send_slot1_metrics();
            // Store metrics
            ringbuffer_push(&rbPpg1Tx, ppg1MetData, PPG_MET_VALID_LEN);
            ringbuffer_push(&rbPpg2Tx, ppg2MetData, PPG_MET_VALID_LEN);
            ringbuffer_push(&rbPpgMaskTx, ppgMaskMetData, PPG_MET_VALID_LEN);

            ringbuffer_seek(&rbPpg1Met, PPG_MET_VALID_LEN);
            ringbuffer_seek(&rbPpg2Met,PPG_MET_VALID_LEN);
            ringbuffer_seek(&rbPpgMaskMet, PPG_MET_VALID_LEN);

            lastTickUs = ns_us_ticker_read(&tickTimerCfg);
            ns_lp_printf("Slot1 Metrics\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1000*PPG_SEG_VALID_LEN/PPG_SAMPLE_RATE/4));
    }
}


void setup_task(void *pvParameters) {
    ns_ble_pre_init();
    xTaskCreate(RadioTask, "RadioTask", 512, 0, 3, &radioTaskHandle);
    xTaskCreate(SensorTask, "SensorTask", 512, 0, 3, &sensorTaskHandle);
    xTaskCreate(Slot0Task, "Slot0Task", 2048, 0, 3, &slot0TaskHandle);
    xTaskCreate(Slot1Task, "Slot1Task", 512, 0, 3, &slot1TaskHandle);
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
    am_devices_led_array_out(am_bsp_psLEDs, AM_BSP_NUM_LEDS, 7);
    NS_TRY(ledstick_init(&nsI2cCfg, LEDSTICK_ADDR), "Led Stick Init Failed\n");
    NS_TRY(sensor_init(&sensorCtx), "Sensor Init Failed\n");
    NS_TRY(ecg_segmentation_init(&ecgSegModelCtx), "Segmentation Init Failed\n");
    NS_TRY(metrics_init(&metricsCfg), "Metrics Init Failed\n");

    ledstick_set_all_colors(&nsI2cCfg, LEDSTICK_ADDR, 0, 207, 193);
    ledstick_set_all_brightness(&nsI2cCfg, LEDSTICK_ADDR, 15);
    sensor_start(&sensorCtx);

    ns_itm_printf_enable();
    ns_interrupt_master_enable();
    xTaskCreate(setup_task, "Setup", 512, 0, 3, &appSetupTask);
    vTaskStartScheduler();
    while (1) { };
}

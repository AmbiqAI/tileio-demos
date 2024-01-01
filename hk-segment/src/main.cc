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
#include "segmentation.h"
#include "metrics.h"
#include "ringbuffer.h"
#include "main.h"

#if (configAPPLICATION_ALLOCATED_HEAP == 1)
uint8_t ucHeap[NS_BLE_DEFAULT_MALLOC_K * 4 * 1024] __attribute__((aligned(4)));
#endif

// RTOS Tasks
TaskHandle_t radioTaskHandle;
TaskHandle_t sensorTaskHandle;
TaskHandle_t MetricsTaskHandle;
TaskHandle_t appSetupTask;

size_t
get_sensor_len() {
    return MIN3(
        ringbuffer_len(&rbEcgSensor),
        ringbuffer_len(&rbPpg1Sensor),
        ringbuffer_len(&rbPpg2Sensor)
    );
}

size_t
get_segmentation_len() {
    return MIN3(
        ringbuffer_len(&rbEcgSeg),
        ringbuffer_len(&rbPpg1Seg),
        ringbuffer_len(&rbPpg2Seg)
    );
}

size_t
get_metrics_len() {
    return MIN(
        MIN3(
            ringbuffer_len(&rbEcgMetrics),
            ringbuffer_len(&rbPpg1Metrics),
            ringbuffer_len(&rbPpg2Metrics)
        ),
        MIN3(
            ringbuffer_len(&rbEcgMaskMetrics),
            ringbuffer_len(&rbPpg1MaskMetrics),
            ringbuffer_len(&rbPpg2MaskMetrics)
        )
    );
}

size_t
get_transmit_len() {
    return MIN(
        MIN3(
            ringbuffer_len(&rbEcgTx),
            ringbuffer_len(&rbPpg1Tx),
            ringbuffer_len(&rbPpg2Tx)
        ),
        MIN3(
            ringbuffer_len(&rbEcgMaskTx),
            ringbuffer_len(&rbPpg1MaskTx),
            ringbuffer_len(&rbPpg2MaskTx)
        )
    );
}

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
    ns_lp_printf("BLE Write: %d\n", *(uint8_t*)src);
    // Update user i/o (buttons, leds, etc.)
    if (c == bleCtx.uioChar) {
        set_leds_state();
        ns_lp_printf("UIO: %d vs %d\n", *(uint8_t*)src, uioState.byte);
        uint32_t ledVal = uioState.led2 << 2 | uioState.led1 << 1 | uioState.led0;
        am_devices_led_array_out(am_bsp_psLEDs, AM_BSP_NUM_LEDS, ledVal);
        // TODO: Flag button state
    }
    return NS_STATUS_SUCCESS;
}

void ble_send_signals() {
    uint32_t offset;
    float32_t valf32;
    int16_t vali16;
    if (get_transmit_len() >= BLE_SIG_NUM_OBJ) {
        offset = 0;
        for (size_t i = 0; i < BLE_SIG_NUM_OBJ; i++) {
            // ECG
            ringbuffer_pop(&rbEcgTx, &valf32, 1);
            vali16 = (int16_t)CLIP(BLE_ECG_SCALE*valf32, -32768, 32767);
            memcpy(&bleCtx.sigBuffer[offset + BLE_SIG_ECG_OFFSET], &vali16, 2);
            // PPG1
            ringbuffer_pop(&rbPpg1Tx, &valf32, 1);
            vali16 = (int16_t)CLIP(BLE_PPG_SCALE*valf32, -32768, 32767);
            memcpy(&bleCtx.sigBuffer[offset + BLE_SIG_PPG1_OFFSET], &vali16, 2);
            // PPG2
            ringbuffer_pop(&rbPpg2Tx, &valf32, 1);
            vali16 = (int16_t)CLIP(BLE_PPG_SCALE*valf32, -32768, 32767);
            memcpy(&bleCtx.sigBuffer[offset + BLE_SIG_PPG2_OFFSET], &vali16, 2);
            // Masks
            ringbuffer_pop(&rbEcgMaskTx, &bleCtx.sigBuffer[offset + BLE_SIG_ECG_MASK_OFFSET], 1);
            ringbuffer_pop(&rbPpg1MaskTx, &bleCtx.sigBuffer[offset + BLE_SIG_PPG1_MASK_OFFSET], 1);
            ringbuffer_pop(&rbPpg2MaskTx, &bleCtx.sigBuffer[offset + BLE_SIG_PPG2_MASK_OFFSET], 1);
            offset += BLE_SIG_OBJ_BYTE_LEN;
        }
        ns_ble_send_value(bleCtx.sigChar, NULL);
    }
}

void ble_send_metrics() {
    ns_ble_send_value(bleCtx.metricsChar, NULL);
}

int ble_notify_signal_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c) {
    ble_send_signals();
    return NS_STATUS_SUCCESS;
}

int ble_notify_metrics_handler(ns_ble_service_t *s, struct ns_ble_characteristic *c) {
    ble_send_metrics();
    return NS_STATUS_SUCCESS;
}

int ble_service_init(void) {
    // Initialize BLE service
    char bleName[] = "PhysioKit";
    NS_TRY(ns_ble_char2uuid(PK_SIG_SVC_UUID, &(bleCtx.service->uuid128)), "Failed to convert UUID\n");
    memcpy(bleCtx.service->name, bleName, sizeof(bleName));
    bleCtx.service->nameLen = strlen(bleName);
    bleCtx.service->baseHandle = 0x0800;
    bleCtx.service->poolConfig = bleCtx.pool;
    bleCtx.service->numAttributes = 0;

    // Define characteristics to add to service.
    ns_ble_create_characteristic(bleCtx.sigChar, PK_SIG_CHAR_UUID, bleCtx.sigBuffer, BLE_SIG_BUF_LEN,
                                 NS_BLE_READ | NS_BLE_NOTIFY, NULL, NULL,
                                 ble_notify_signal_handler, 1000/BLE_SIG_SAMPLE_RATE, true, &(bleCtx.service->numAttributes));

    ns_ble_create_characteristic(bleCtx.metricsChar, PK_SIG_CHAR_UUID, bleCtx.metricsResults, sizeof(metrics_results_t),
                                 NS_BLE_READ, NULL, NULL,
                                 NULL, 1000*MET_CAPTURE_SEC, true, &(bleCtx.service->numAttributes));

    ns_ble_create_characteristic(bleCtx.uioChar, PK_UIO_CHAR_UUID, bleCtx.uioBuffer, sizeof(uint8_t),
                                 NS_BLE_READ | NS_BLE_WRITE, &ble_read_handler, &ble_write_handler,
                                 NULL, 0, true, &(bleCtx.service->numAttributes));

    bleCtx.service->numCharacteristics = BLE_NUM_CHARS;
    ns_ble_create_service(bleCtx.service);
    ns_ble_add_characteristic(bleCtx.service, bleCtx.sigChar);
    ns_ble_add_characteristic(bleCtx.service, bleCtx.metricsChar);
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
    uint32_t numSamples;
    float32_t ecgVal, ppg1Val, ppg2Val;
    while (true) {
        if (sensorCtx.is_stimulus ^ uioState.btn0) {
            sensorCtx.is_stimulus = uioState.btn0;
            ns_lp_printf("Button 0 pressed (%d)\n", sensorCtx.is_stimulus);
        }
        // if (*nsBtnCfg.button_0_flag) {
        //     sensorCtx.is_stimulus = !sensorCtx.is_stimulus;
        //     *nsBtnCfg.button_0_flag = false;
        //     ns_lp_printf("Button 0 pressed (%d)\n", sensorCtx.is_stimulus);
        // }
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
        // Downsample sensor data
        numSamples = ringbuffer_len(&rbEcgSensor);
        for (size_t i = 0; i < numSamples/DOWNSAMPLE_RATE; i++) {
            ringbuffer_seek(&rbEcgSensor, DOWNSAMPLE_RATE - 1);
            ringbuffer_seek(&rbPpg1Sensor, DOWNSAMPLE_RATE - 1);
            ringbuffer_seek(&rbPpg2Sensor, DOWNSAMPLE_RATE - 1);
            ringbuffer_transfer(&rbEcgSensor, &rbEcgSeg, 1);
            ringbuffer_transfer(&rbPpg1Sensor, &rbPpg1Seg, 1);
            ringbuffer_transfer(&rbPpg2Sensor, &rbPpg2Seg, 1);
        }
        // Delay for ~16 samples
        vTaskDelay(pdMS_TO_TICKS(16*(1000/SENSOR_RATE)));
    }
}

void MetricsTask(void *pvParameters) {
    uint32_t lastTickUs = 0;
    uint32_t err;
    while (true) {

        // Perform segmentation
        if (get_segmentation_len() >= SEG_WINDOW_LEN) {
             ns_timer_clear(&tickTimerCfg);
            // Perform ECG segmentation
            ringbuffer_peek(&rbEcgSeg, segInputs, SEG_WINDOW_LEN);
            pk_apply_biquad_filtfilt_f32(&ecgFilterCtx, segInputs, segInputs, SEG_WINDOW_LEN, segFilterBuffer);
            pk_standardize_f32(segInputs, segInputs, SEG_WINDOW_LEN, NORM_STD_EPS);
            err = segmentation_inference(&segModelCtx, segInputs, segMask, 0, SEG_THRESHOLD);
            ringbuffer_push(&rbEcgMetrics, &segInputs[SEG_PAD_LEN], SEG_VALID_LEN);
            ringbuffer_push(&rbEcgMaskMetrics, &segMask[SEG_PAD_LEN], SEG_VALID_LEN);
            ringbuffer_seek(&rbEcgSeg, SEG_VALID_LEN);

            // Perform PPG1 segmentation
            ringbuffer_peek(&rbPpg1Seg, segInputs, SEG_WINDOW_LEN);
            // pk_apply_biquad_filter_f32(&ppg1FilterCtx, segInputs, segInputs, SEG_WINDOW_LEN);
            // pk_standardize_f32(segInputs, segInputs, SEG_WINDOW_LEN, NORM_STD_EPS);
            memset(segMask, 0, SEG_WINDOW_LEN*sizeof(uint8_t)); // TODO: Run PPG segmentation
            ringbuffer_push(&rbPpg1Metrics, &segInputs[SEG_PAD_LEN], SEG_VALID_LEN);
            ringbuffer_push(&rbPpg1MaskMetrics, &segMask[SEG_PAD_LEN], SEG_VALID_LEN);
            ringbuffer_seek(&rbPpg1Seg, SEG_VALID_LEN);

            // Perform PPG2 segmentation
            ringbuffer_peek(&rbPpg2Seg, segInputs, SEG_WINDOW_LEN);
            // pk_apply_biquad_filter_f32(&ppg2FilterCtx, segInputs, segInputs, SEG_WINDOW_LEN);
            // pk_standardize_f32(segInputs, segInputs, SEG_WINDOW_LEN, NORM_STD_EPS);
            memset(segMask, 0, SEG_WINDOW_LEN*sizeof(uint8_t)); // TODO: Run PPG segmentation
            ringbuffer_push(&rbPpg2Metrics, &segInputs[SEG_PAD_LEN], SEG_VALID_LEN);
            ringbuffer_push(&rbPpg2MaskMetrics, &segMask[SEG_PAD_LEN], SEG_VALID_LEN);
            ringbuffer_seek(&rbPpg2Seg, SEG_VALID_LEN);
            lastTickUs = ns_us_ticker_read(&tickTimerCfg);
            ns_lp_printf("Segmentation Time: %d\n", lastTickUs);
        }

        // Perform metrics
        if (get_metrics_len() >= MET_WINDOW_LEN) {
            ns_timer_clear(&tickTimerCfg);
            // Grab data from ringbuffers
            ringbuffer_peek(&rbEcgMetrics, ecgMetricsData, MET_WINDOW_LEN);
            ringbuffer_peek(&rbPpg1Metrics, ppg1MetricsData, MET_WINDOW_LEN);
            ringbuffer_peek(&rbPpg2Metrics, ppg2MetricsData, MET_WINDOW_LEN);
            ringbuffer_peek(&rbEcgMaskMetrics, ecgMaskMetricsData, MET_WINDOW_LEN);
            ringbuffer_peek(&rbPpg1MaskMetrics, ppg1MaskMetricsData, MET_WINDOW_LEN);
            ringbuffer_peek(&rbPpg2MaskMetrics, ppg2MaskMetricsData, MET_WINDOW_LEN);

            // Compute metrics
            metrics_capture_ppg(
                &metricsCfg,
                ppg1MetricsData, ppg1MaskMetricsData,
                ppg2MetricsData, ppg2MaskMetricsData, MET_WINDOW_LEN,
                &metricsResults
            );
            metrics_capture_ecg(
                &metricsCfg,
                ecgMetricsData, ecgMaskMetricsData, MET_WINDOW_LEN,
                &metricsResults
            );
            // Store metrics
            ringbuffer_push(&rbEcgTx, ecgMetricsData, MET_VALID_LEN);
            ringbuffer_push(&rbPpg1Tx, ppg1MetricsData, MET_VALID_LEN);
            ringbuffer_push(&rbPpg2Tx, ppg2MetricsData, MET_VALID_LEN);
            ringbuffer_push(&rbEcgMaskTx, ecgMaskMetricsData, MET_VALID_LEN);
            ringbuffer_push(&rbPpg1MaskTx, ppg1MaskMetricsData, MET_VALID_LEN);
            ringbuffer_push(&rbPpg2MaskTx, ppg2MaskMetricsData, MET_VALID_LEN);

            ringbuffer_seek(&rbEcgMetrics, MET_VALID_LEN);
            ringbuffer_seek(&rbPpg1Metrics, MET_VALID_LEN);
            ringbuffer_seek(&rbPpg2Metrics, MET_VALID_LEN);
            ringbuffer_seek(&rbEcgMaskMetrics, MET_VALID_LEN);
            ringbuffer_seek(&rbPpg1MaskMetrics, MET_VALID_LEN);
            ringbuffer_seek(&rbPpg2MaskMetrics, MET_VALID_LEN);

            lastTickUs = ns_us_ticker_read(&tickTimerCfg);
            ns_lp_printf("Metrics Time: %d\n", lastTickUs);
        }
        vTaskDelay(pdMS_TO_TICKS(1000*SEG_VALID_LEN/SAMPLE_RATE/4));
    }
}


void setup_task(void *pvParameters) {
    ns_ble_pre_init();
    xTaskCreate(RadioTask, "RadioTask", 512, 0, 3, &radioTaskHandle);
    xTaskCreate(SensorTask, "SensorTask", 512, 0, 3, &sensorTaskHandle);
    xTaskCreate(MetricsTask, "MetricsTask", 2048, 0, 3, &MetricsTaskHandle);
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
    NS_TRY(segmentation_init(&segModelCtx), "Segmentation Init Failed\n");
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

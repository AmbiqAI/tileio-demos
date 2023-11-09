/**
 * @file main.cc
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Main application
 * @version 1.0
 * @date 2023-03-27
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "arm_math.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// neuralSPOT
#include "ns_ambiqsuite_harness.h"
#include "ns_malloc.h"
#include "ns_peripherals_button.h"
#include "ns_peripherals_power.h"
#include "ns_rpc_generic_data.h"
#include "ns_usb.h"

// Locals
#include "constants.h"
#include "metrics.h"
#include "main.h"
#include "sensor.h"
#include "physiokit.h"

// Application globals
static uint32_t numSamples = 0;

static bool usbAvailable = false;
static int volatile sensorCollectBtnPressed = false;
static int volatile clientCollectBtnPressed = false;

static float32_t ppg1Data[PK_SENSOR_LEN];
static float32_t ppg2Data[PK_SENSOR_LEN];
static float32_t ecgData[PK_SENSOR_LEN];


static AppState state = IDLE_STATE;
static DataCollectMode collectMode = SENSOR_DATA_COLLECT;

const ns_power_config_t ns_pwr_config = {.api = &ns_power_V1_0_0,
                                         .eAIPowerMode = NS_MINIMUM_PERF,
                                         .bNeedAudAdc = false,
                                         .bNeedSharedSRAM = false,
                                         .bNeedCrypto = true,
                                         .bNeedBluetooth = false,
                                         .bNeedUSB = true,
                                         .bNeedIOM = false, // We will manually enable IOM0
                                         .bNeedAlternativeUART = false,
                                         .b128kTCM = false};

//*****************************************************************************
//*** Peripheral Configs
ns_button_config_t button_config = {.api = &ns_button_V1_0_0,
                                    .button_0_enable = true,
                                    .button_1_enable = true,
                                    .button_0_flag = &sensorCollectBtnPressed,
                                    .button_1_flag = &clientCollectBtnPressed};

// Handle TinyUSB events
void
tud_mount_cb(void) {
    usbAvailable = true;
}
void
tud_resume_cb(void) {
    usbAvailable = true;
}
void
tud_umount_cb(void) {
    usbAvailable = false;
}
void
tud_suspend_cb(bool remote_wakeup_en) {
    usbAvailable = false;
}

void
gpio_init(uint32_t pin, uint32_t mode) {
    am_hal_gpio_pincfg_t config = mode == 0 ?
        am_hal_gpio_pincfg_disabled : mode == 1 ?
        am_hal_gpio_pincfg_output : am_hal_gpio_pincfg_input;
    am_hal_gpio_pinconfig(pin, config);
}

uint32_t
gpio_write(uint32_t pin, uint8_t value) {
    return am_hal_gpio_state_write(pin, (am_hal_gpio_write_type_e)value);
}

uint32_t
gpio_read(uint32_t pin, uint32_t mode, uint32_t value) {
    am_hal_gpio_read_type_e readMode = mode == 0 ?
        AM_HAL_GPIO_INPUT_READ : mode == 1 ?
        AM_HAL_GPIO_OUTPUT_READ : AM_HAL_GPIO_INPUT_READ;
    return am_hal_gpio_state_read(pin, readMode, &value);
}

void
background_task() {
    /**
     * @brief Run background tasks
     *
     */
}

void
sleep_us(uint32_t time) {
    /**
     * @brief Enable longer sleeps while also running background tasks on interval
     * @param time Sleep duration in microseconds
     */
    uint32_t chunk;
    while (time > 0) {
        chunk = MIN(10000, time);
        ns_delay_us(chunk);
        time -= chunk;
        background_task();
    }
}

void
init_rpc(void) {
    /**
     * @brief Initialize RPC and USB
     *
     */
    ns_rpc_config_t rpcConfig = {.api = &ns_rpc_gdo_V1_0_0,
                                 .mode = NS_RPC_GENERICDATA_CLIENT,
                                 .sendBlockToEVB_cb = NULL,
                                 .fetchBlockFromEVB_cb = NULL,
                                 .computeOnEVB_cb = NULL};
    ns_rpc_genericDataOperations_init(&rpcConfig);
}

void
print_to_pc(const char *msg) {
    /**
     * @brief Print to PC over RPC
     *
     */
    if (usbAvailable) {
        ns_rpc_data_remotePrintOnPC(msg);
    }
    ns_printf(msg);
}

void
start_collecting(void) {
    /**
     * @brief Setup sensor for collecting
     *
     */
    if (collectMode == SENSOR_DATA_COLLECT) {
        start_sensor();
        // Discard first second for sensor warm up time
        for (size_t i = 0; i < 25; i++) {
            capture_sensor_data(ppg1Data, ppg2Data, ecgData, NULL, 32);
            sleep_us(40000);
        }
    }
    numSamples = 0;
}

void
stop_collecting(void) {
    /**
     * @brief Disable sensor
     *
     */
    if (collectMode == SENSOR_DATA_COLLECT) {
        stop_sensor();
    }
    numSamples = 0;
}

uint32_t
fetch_samples_from_pc(float32_t *samples, uint32_t offset, uint32_t numSamples) {
    /**
     * @brief Fetch samples from PC over RPC
     * @param samples Buffer to store samples
     * @param offset Buffer offset
     * @param numSamples # requested samples
     * @return # samples actually fetched
     */
    static char rpcFetchSamplesDesc[] = "FETCH_SAMPLES";
    int err;
    if (!usbAvailable) {
        return 0;
    }
    binary_t binaryBlock = {
        .data = (uint8_t *)(&samples[offset]),
        .dataLength = numSamples * sizeof(float32_t),
    };
    dataBlock resultBlock = {
        .length = numSamples, .dType = float32_e, .description = rpcFetchSamplesDesc, .cmd = generic_cmd, .buffer = binaryBlock};
    err = ns_rpc_data_computeOnPC(&resultBlock, &resultBlock);
    if (resultBlock.description != rpcFetchSamplesDesc) {
        ns_free(resultBlock.description);
    }
    if (resultBlock.buffer.data != (uint8_t *)&samples[offset]) {
        ns_free(resultBlock.buffer.data);
    }
    if (err) {
        ns_printf("Failed fetching from PC w/ error: %x\n", err);
        return 0;
    }
    memcpy(&samples[offset], resultBlock.buffer.data, resultBlock.buffer.dataLength);
    return resultBlock.buffer.dataLength / sizeof(float32_t);
}

void
send_samples_to_pc(float32_t *samples, uint32_t offset, uint32_t numSamples) {
    /**
     * @brief Send sensor samples to PC
     * @param samples Samples to send
     * @param offset Buffer offset
     * @param numSamples # samples to send
     */
    static char rpcSendSamplesDesc[] = "SEND_SAMPLES";
    if (!usbAvailable) {
        return;
    }
    binary_t binaryBlock = {
        .data = (uint8_t *)(&samples[offset]),
        .dataLength = numSamples * sizeof(float32_t),
    };
    dataBlock commandBlock = {
        .length = offset, .dType = float32_e, .description = rpcSendSamplesDesc, .cmd = generic_cmd, .buffer = binaryBlock};
    ns_rpc_data_sendBlockToPC(&commandBlock);
}

uint32_t
collect_samples() {
    /**
     * @brief Collect samples from sensor or PC
     * @return # new samples collected
     */
    uint32_t newSamples = 0;
    uint32_t reqSamples = MIN(PK_SENSOR_LEN - numSamples, 32);
    if (numSamples == PK_SENSOR_LEN) {
        return newSamples;
    }
    if (collectMode == CLIENT_DATA_COLLECT) {
        newSamples = reqSamples;
        // newSamples = fetch_samples_from_pc(hkData, numSamples, reqSamples);
    } else if (collectMode == SENSOR_DATA_COLLECT) {
        newSamples = capture_sensor_data(&ppg1Data[numSamples], &ppg2Data[numSamples], &ecgData[numSamples], NULL, reqSamples);
        sleep_us(40000);
        if (newSamples) {
            // send_samples_to_pc(hkData, numSamples, newSamples);
        }
    }
    numSamples += newSamples;
    return newSamples;
}

void
wakeup() {
    am_bsp_itm_printf_enable();
    am_bsp_debug_printf_enable();
}

void
deepsleep() {
    am_bsp_itm_printf_disable();
    am_bsp_debug_printf_disable();
    am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
}

void
setup() {
    /**
     * @brief Application setup
     *
     */
    // Power configuration (mem, cache, peripherals, clock)
    uint32_t err = 0;
    ns_core_config_t ns_core_cfg = {.api = &ns_core_V1_0_0};
    ns_core_init(&ns_core_cfg);
    ns_power_config(&ns_pwr_config);
    am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_IOM0);
    gpio_init(GPIO_TRIGGER, 1);
    gpio_write(GPIO_TRIGGER, 0);
    // Enable Interrupts
    am_hal_interrupt_master_enable();
    // Enable SWO/USB
    wakeup();
    // Initialize blocks
    init_rpc();
    err |= init_sensor();
    err |= init_metrics();
    err |= ns_peripheral_button_init(&button_config);
    ns_printf("🩺PhysioKit Demo\n\n");
    ns_printf("Please select data collection options:\n\n\t1. BTN1=sensor\n\t2. BTN2=stimulus\n");
}

void
loop() {
    /**
     * @brief Application loop
     *
     */
    static uint32_t app_err = 0;
    switch (state) {
    case IDLE_STATE:
        if (sensorCollectBtnPressed | clientCollectBtnPressed) {
            collectMode = sensorCollectBtnPressed ? SENSOR_DATA_COLLECT : CLIENT_DATA_COLLECT;
            wakeup();
            state = START_COLLECT_STATE;
        } else {
            ns_printf("IDLE_STATE\n");
            deepsleep();
        }
        break;

    case START_COLLECT_STATE:
        print_to_pc("COLLECT_STATE\n");
        sensorCollectBtnPressed = false; // DEBOUNCE
        clientCollectBtnPressed = false; // DEBOUNCE
        start_collecting();
        state = COLLECT_STATE;
        break;

    case COLLECT_STATE:
        collect_samples();
        if (numSamples >= PK_SENSOR_LEN) {
            state = STOP_COLLECT_STATE;
        }
        break;

    case STOP_COLLECT_STATE:
        stop_collecting();
        // am_hal_pwrctrl_mcu_mode_select(AM_HAL_PWRCTRL_MCU_MODE_HIGH_PERFORMANCE);
        state = PREPROCESS_STATE;
        break;

    case PREPROCESS_STATE:
        print_to_pc("PREPROCESS_STATE\n");
        gpio_write(GPIO_TRIGGER, 1);
        if (collectMode == SENSOR_DATA_COLLECT) {
            pk_linear_downsample(ppg1Data, PK_SENSOR_LEN, SENSOR_RATE, ppg1Data, PK_DATA_LEN, SAMPLE_RATE);
            pk_linear_downsample(ppg2Data, PK_SENSOR_LEN, SENSOR_RATE, ppg2Data, PK_DATA_LEN, SAMPLE_RATE);
            pk_linear_downsample(ecgData, PK_SENSOR_LEN, SENSOR_RATE, ecgData, PK_DATA_LEN, SAMPLE_RATE);
        }
        pk_preprocess(ppg1Data, ppg2Data, ecgData, PK_DATA_LEN);
        gpio_write(GPIO_TRIGGER, 0);
        state = INFERENCE_STATE;
        break;

    case INFERENCE_STATE:
        print_to_pc("INFERENCE_STATE\n");
        // app_err = hk_run(hkData, hkSegMask, &hkResults);
        // am_hal_pwrctrl_mcu_mode_select(AM_HAL_PWRCTRL_MCU_MODE_LOW_POWER);
        state = app_err == 1 ? FAIL_STATE : DISPLAY_STATE;
        break;

    case DISPLAY_STATE:
        // for (size_t i = 0; i < PK_DATA_LEN; i += SAMPLE_RATE) {
        //     uint32_t maskLen = MIN(PK_DATA_LEN - i, SAMPLE_RATE);
        //     send_mask_to_pc(&hkSegMask[i], i, maskLen);
        // }
        // send_results_to_pc(&hkResults);
        ns_delay_us(10000);
        print_to_pc("DISPLAY_STATE\n");
        ns_delay_us(DISPLAY_LEN_USEC);

        // am_hal_pwrctrl_mcu_mode_select(AM_HAL_PWRCTRL_MCU_MODE_LOW_POWER);
        if (sensorCollectBtnPressed | clientCollectBtnPressed) {
            sensorCollectBtnPressed = false;
            clientCollectBtnPressed = false;
            state = IDLE_STATE;
        } else {
            state = IDLE_STATE; // START_COLLECT_STATE;
        }
        break;

    case FAIL_STATE:
        ns_printf("FAIL_STATE err=%d\n", app_err);
        state = IDLE_STATE;
        app_err = 0;
        break;

    default:
        state = IDLE_STATE;
        break;
    }
    background_task();
}

int
main(void) {
    /**
     * @brief Main function
     * @return int
     */
    setup();
    while (1) { loop(); }
}

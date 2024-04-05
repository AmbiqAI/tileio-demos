/**
 * @file metrics.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Compute physiokit metrics
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __PK_METRICS_H
#define __PK_METRICS_H

#include "arm_math.h"

/**
 * @brief Metrics configuration
 *
 */
typedef struct {
} metrics_config_t;

/**
 * @brief Metrics ECG results
 *
 */
typedef struct {
    float32_t hr;  // Heart rate (bpm)
    float32_t hrv;  // Heart rate variability (ms)
    float32_t ips; // Inference per second
} metrics_ecg_results_t;


/**
 * @brief Metrics PPG results
 *
 */
typedef struct {
    float32_t pr;  // Pulse rate (bpm)
    float32_t spo2;  // Blood oxygen saturation (%)
} metrics_ppg_results_t;

/**
 * @brief Initialize metrics
 *
 * @param ctx Metrics context
 * @return uint32_t
 */
uint32_t
metrics_init(metrics_config_t *ctx);


uint32_t
metrics_capture_ppg(
    metrics_config_t *ctx,
    float32_t *ppg1,
    float32_t *ppg2,
    uint16_t *ppgMask,
    size_t len,
    float32_t ppg1Mean,
    float32_t ppg2Mean,
    metrics_ppg_results_t *results
);

uint32_t
metrics_capture_ecg(
    metrics_config_t *ctx,
    float32_t *ecg,
    uint16_t *ecgMask,
    size_t len,
    metrics_ecg_results_t *results
);

#endif // __PK_METRICS_H

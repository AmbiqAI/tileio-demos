/**
 * @file metrics.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Compute heartkit metrics
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __HK_METRICS_H
#define __HK_METRICS_H

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
    float32_t hr;
    float32_t hrv;
    float32_t denoiseCossim;
    float32_t arrhythmiaLabel;
    float32_t denoiseIps;
    float32_t segmentIps;
    float32_t arrhythmiaIps;
    float32_t cpuPercUtil;
} metrics_app_results_t;


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
metrics_capture_ecg(
    metrics_config_t *ctx,
    float32_t *ecg,
    uint16_t *ecgMask,
    size_t len,
    metrics_app_results_t *results
);

#endif // __HK_METRICS_H

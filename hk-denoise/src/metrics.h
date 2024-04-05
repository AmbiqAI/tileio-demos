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
    float32_t rawCosim;  // Cosine similarity
    float32_t rawMse;  // Mean squared error
    float32_t denCosim;  // Cosine similarity
    float32_t denMse;  // Mean squared error
    float32_t ips;
} metrics_ecg_results_t;


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
    float32_t *ecgRaw,
    float32_t *ecgNos,
    float32_t *ecgDen,
    uint16_t *ecgMask,
    size_t len,
    metrics_ecg_results_t *results
);

#endif // __PK_METRICS_H

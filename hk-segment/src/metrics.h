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

typedef struct {
} metrics_config_t;

typedef struct {
    float32_t hr;
    float32_t spo2;
} __attribute__((packed)) metrics_results_t;

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
    uint8_t *ppg1Mask,
    float32_t *ppg2,
    uint8_t *ppg2Mask,
    size_t len,
    metrics_results_t *results
);

uint32_t
metrics_capture_ecg(
    metrics_config_t *ctx,
    float32_t *ecg,
    uint8_t *ecgMask,
    size_t len,
    metrics_results_t *results
);

#endif // __PK_METRICS_H

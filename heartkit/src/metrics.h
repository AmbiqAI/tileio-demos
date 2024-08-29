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
    float32_t denoise_ips;
    float32_t segment_ips;
    float32_t denoise_cossim;
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
 * @brief Compute cosine similarity
 *
 * @param ref Reference signal
 * @param sig Signal to compare
 * @param len Length of signals
 * @param result Result of comparison
 * @return uint32_t
 */
uint32_t cosine_similarity_f32(
    float32_t *ref,
    float32_t *sig,
    size_t len,
    float32_t *result
);

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

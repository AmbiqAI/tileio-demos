/**
 * @file metrics.c
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Compute physiokit metrics
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <stdbool.h>
#include "arm_math.h"

#include "physiokit/pk_math.h"
#include "physiokit/pk_ecg.h"
#include "physiokit/pk_ppg.h"
#include "physiokit/pk_hrv.h"
#include "physiokit/pk_filter.h"

#include "constants.h"
#include "metrics.h"
#include "store.h"


uint32_t
metrics_init(metrics_config_t *ctx) {
    uint32_t err = 0;
    return err;
}

uint32_t cosine_similarity_f32(
    float32_t *ref,
    float32_t *sig,
    size_t len,
    float32_t *result
) {
    float32_t dot = 0.0f, normA = 0.0f, normB = 0.0f;
    for (size_t i = 0; i < len; i++) {
        dot += ref[i] * sig[i];
        normA += ref[i] * ref[i];
        normB += sig[i] * sig[i];
    }
    *result = dot / (sqrtf(normA) * sqrtf(normB));
    return 0;
}

uint32_t mean_absolute_percent_error(
    float32_t *ref,
    float32_t *sig,
    size_t len,
    float32_t *result,
    float32_t epsilon
) {
    float32_t sum = 0.0f;
    for (size_t i = 0; i < len; i++) {
        sum += abs(ref[i] - sig[i]) / (abs(ref[i]) + epsilon);
    }
    *result = 100.0f * sum / (float32_t)len;
    return 0;
}

uint32_t mean_squared_error(
    float32_t *ref,
    float32_t *sig,
    size_t len,
    float32_t *result
) {
    float32_t sum = 0.0f;
    for (size_t i = 0; i < len; i++) {
        sum += (ref[i] - sig[i]) * (ref[i] - sig[i]);
    }
    *result = sum / (float32_t)len;
    return 0;
}

uint32_t
metrics_capture_ecg(
    metrics_config_t *ctx,
    float32_t *ecgRaw,
    float32_t *ecgNos,
    float32_t *ecgDen,
    uint16_t *ecgMask,
    size_t len,
    metrics_ecg_results_t *results
) {
    float32_t cosim = 0, mape = 0, mse = 0;
    size_t numPeaks = 0, numBeats = 0;
    float32_t heartRate = 0;
    float32_t qos = 0;
    uint32_t err = 0;

    // Find peaks
    numPeaks = pk_ecg_find_peaks_f32(&ecgFindPeakCtx, ecgDen, len, peaksMetrics);
    pk_ecg_compute_rr_intervals(peaksMetrics, numPeaks, rriMetrics);
    pk_ecg_filter_rr_intervals(rriMetrics, numPeaks, rriMask, ECG_SAMPLE_RATE, MIN_RR_SEC, MAX_RR_SEC, MIN_RR_DELTA);

    // Compute heart rate
    size_t numValid = 0;
    for (size_t i = 0; i < numPeaks; i++){
        if (rriMask[i] == 0) {
            heartRate += 60.0f / (rriMetrics[i] / (float32_t)ECG_SAMPLE_RATE);
            numValid += 1;
        }
    }
    heartRate /= MAX(1.0f, numValid);
    results->hr = heartRate;

    qos = 100*(float32_t)numValid/(float32_t)numPeaks;

    cosine_similarity_f32(ecgRaw, ecgNos, len, &cosim);
    mean_squared_error(ecgRaw, ecgNos, len, &mse);
    results->rawCosim = 100*cosim;
    results->rawMse = mse;

    cosine_similarity_f32(ecgRaw, ecgDen, len, &cosim);
    mean_squared_error(ecgRaw, ecgDen, len, &mse);
    results->denCosim = 100*cosim;
    results->denMse = mse;

    ns_lp_printf("ECG HR: %0.2f, NPOINTS: %d, NVALID: %d, QOS: %0.2f\n", heartRate, numPeaks, numValid, qos);
    ns_lp_printf("NOS MSE: %0.2f, COSIM: %0.2f\n", results->rawMse, results->rawCosim);
    ns_lp_printf("DEN MSE: %0.2f, COSIM: %0.2f\n", results->denMse, results->denCosim);

    results->hr = heartRate;
    results->hrv = 0;
    return err;
}

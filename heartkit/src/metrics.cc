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

uint32_t
metrics_init(metrics_config_t *ctx) {
    uint32_t err = 0;
    return err;
}

uint32_t
metrics_capture_ecg(
    metrics_config_t *ctx,
    float32_t *ecg,
    uint16_t *ecgMask,
    size_t len,
    metrics_app_results_t *results
) {
    uint32_t err = 0;
    uint16_t peakVal, beatVal;
    // float32_t badPeakPerc = 0;
    size_t numPPeaks = 0, numQrsPeaks = 0, numTPeaks = 0, numBeats = 0, numNoiseBeats = 0;
    float32_t hr = 0;

    // Extract fiducial candidates from mask
    numQrsPeaks = 0;
    for (size_t i = 0; i < len; i++) {
        peakVal = (ecgMask[i] >> ECG_MASK_FID_PEAK_OFFSET) & ECG_MASK_FID_PEAK_MASK;
        if (peakVal == ECG_FID_PEAK_QRS) {
            peaksMetrics[numQrsPeaks] = i;
            numQrsPeaks++;
        } else if (peakVal == ECG_FID_PEAK_PPEAK) {
            numPPeaks++;
        } else if (peakVal == ECG_FID_PEAK_TPEAK) {
            numTPeaks++;
        }
    }

    // Filter RR intervals and peaks
    pk_ecg_compute_rr_intervals(peaksMetrics, numQrsPeaks, rriMetrics);
    pk_ecg_filter_rr_intervals(rriMetrics, numQrsPeaks, rriMask, ECG_SAMPLE_RATE, MIN_RR_SEC, MAX_RR_SEC, MIN_RR_DELTA);
    pk_hrv_compute_time_metrics_from_rr_intervals(rriMetrics, numQrsPeaks, rriMask, &ecgHrvMetrics, ECG_SAMPLE_RATE);

    // Annotate mask with beats
    for (size_t i = 0; i < numQrsPeaks; i++) {
        if (rriMask[i] == 1) {
            beatVal = ECG_FID_BEAT_NOISE;
            numNoiseBeats += 1;
        } else {
            hr += 60.0f / (rriMetrics[i] / (float32_t)ECG_SAMPLE_RATE);
            beatVal = ECG_FID_BEAT_NSR;
            numBeats += 1;
        }
        ecgMask[peaksMetrics[i]] |= ((beatVal & ECG_MASK_FID_BEAT_MASK) << ECG_MASK_FID_BEAT_OFFSET);
    }
    hr /= MAX(1, numBeats);

    results->hr = hr;
    results->hrv = ecgHrvMetrics.rmsSD;

    // ns_lp_printf("nPWAVE=%d, nQRS=%d, nTWAVE=%d\n", numPPeaks, numQrsPeaks, numTPeaks);
    // ns_lp_printf("nBEATS=%d, nNOISE=%d, HR=%0.2f, HRV=%0.2f\n\n", numBeats, numNoiseBeats, results->hr, results->hrv);
    return err;
}

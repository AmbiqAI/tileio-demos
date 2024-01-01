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

uint32_t
metrics_capture_ppg(
    metrics_config_t *ctx,
    float32_t *ppg1,
    uint8_t *ppg1Mask,
    float32_t *ppg2,
    uint8_t *ppg2Mask,
    size_t len,
    metrics_results_t *results
) {
    uint32_t err = 0;
    uint8_t fidVal, beatVal;
    float32_t spo2;
    float32_t badPeakPerc = 0;
    float32_t ppg1Mean, ppg2Mean;
    size_t numPeaks = 0, numBeats = 0;

    // Filter PPG signal
    err |= pk_mean_f32(ppg1, &ppg1Mean, len);
    pk_apply_biquad_filter_f32(&ppg1FilterCtx, ppg1, ppg1, len);
    pk_standardize_f32(ppg1, ppg1, len, NORM_STD_EPS);

    // Find peaks in PPG signal
    numPeaks = pk_ppg_find_peaks_f32(&ppgFindPeakCtx, ppg1, len, peaksMetrics);
    pk_ppg_compute_rr_intervals(peaksMetrics, numPeaks, rriMetrics);
    pk_ppg_filter_rr_intervals(rriMetrics, numPeaks, metricsMask, SAMPLE_RATE, MIN_RR_SEC, MAX_RR_SEC, MIN_RR_DELTA);

    // Annotate ppg mask with systolic beats
    for (size_t i = 0; i < numPeaks; i++) {
        if (metricsMask[i] == 1) {
            beatVal = PPG_BEAT_NOISE;
            fidVal = PPG_FID_NONE;
        } else {
            beatVal = PPG_BEAT_NSR;
            fidVal = PPG_FID_SPEAK;
        }
        ppg1Mask[peaksMetrics[i]] |= ((beatVal & SIG_MASK_BEAT_MASK) << SIG_MASK_BEAT_OFFSET);
        ppg1Mask[peaksMetrics[i]] |= ((fidVal & SIG_MASK_FID_MASK) << SIG_MASK_FID_OFFSET);
    }

    // Annotate PPG mask with QoS
    uint8_t ppg1Qos = ppg1Mean < MET_PPG_MIN_VAL ? SIG_QOS_BAD : SIG_QOS_GOOD;
    for (size_t i = 0; i < len; i++) {
        ppg1Mask[i] |= ((ppg1Qos & SIG_MASK_QOS_MASK) << SIG_MASK_QOS_OFFSET);
    }
    ns_lp_printf("PPG1: npeaks=%d, mean=%0.2f, qos=%0.2f\n", numPeaks, ppg1Mean, ppg1Qos);

    // Filter PPG signal
    err |= pk_mean_f32(ppg2, &ppg2Mean, len);
    pk_apply_biquad_filter_f32(&ppg2FilterCtx, ppg2, ppg2, len);
    pk_standardize_f32(ppg2, ppg2, len, NORM_STD_EPS);

    // Find peaks in PPG signal
    numPeaks = pk_ppg_find_peaks_f32(&ppgFindPeakCtx, ppg2, len, peaksMetrics);
    pk_ppg_compute_rr_intervals(peaksMetrics, numPeaks, rriMetrics);
    pk_ppg_filter_rr_intervals(rriMetrics, numPeaks, metricsMask, SAMPLE_RATE, MIN_RR_SEC, MAX_RR_SEC, MIN_RR_DELTA);

    // Annotate ppg mask with systolic beats
    for (size_t i = 0; i < numPeaks; i++) {
        if (metricsMask[i] == 1) {
            beatVal = PPG_BEAT_NOISE;
            fidVal = PPG_FID_NONE;
        } else {
            beatVal = PPG_BEAT_NSR;
            fidVal = PPG_FID_SPEAK;
        }
        ppg2Mask[peaksMetrics[i]] |= ((beatVal & SIG_MASK_BEAT_MASK) << SIG_MASK_BEAT_OFFSET);
        ppg2Mask[peaksMetrics[i]] |= ((fidVal & SIG_MASK_FID_MASK) << SIG_MASK_FID_OFFSET);
    }

    // Annotate PPG mask with QoS
    uint8_t ppg2Qos = ppg2Mean < MET_PPG_MIN_VAL ? SIG_QOS_BAD : SIG_QOS_GOOD;
    for (size_t i = 0; i < len; i++) {
        ppg2Mask[i] |= ((ppg2Qos & SIG_MASK_QOS_MASK) << SIG_MASK_QOS_OFFSET);
    }
    ns_lp_printf("PPG2: npeaks=%d, mean=%0.2f, qos=%0.2f\n", numPeaks, ppg2Mean, ppg2Qos);

    // Compute SpO2
    if (ppg1Qos <= SIG_QOS_POOR || ppg2Qos <= SIG_QOS_POOR) {
        spo2 = 0;
    } else {
        spo2 = pk_ppg_compute_spo2_in_time_f32(ppg1, ppg2, ppg1Mean, ppg2Mean, len, spo2Coefs, SAMPLE_RATE);
    }
    results->spo2 = spo2;
    ns_lp_printf("SpO2=%0.2f\n", spo2);
    return err;
}


uint32_t
metrics_capture_ecg(
    metrics_config_t *ctx,
    float32_t *ecg,
    uint8_t *ecgMask,
    size_t len,
    metrics_results_t *results
) {
    uint32_t err = 0;
    uint8_t fidVal, beatVal;
    float32_t badPeakPerc = 0;
    size_t numPPeaks = 0, numQrsPeaks = 0, numTPeaks = 0, numBeats = 0;

    // Extract fiducial candidates from mask
    numQrsPeaks = 0;
    for (size_t i = 0; i < len; i++) {
        fidVal = (ecgMask[i] >> SIG_MASK_FID_OFFSET) & SIG_MASK_FID_MASK;
        if (fidVal == ECG_FID_QRS) {
            peaksMetrics[numQrsPeaks] = i;
            numQrsPeaks++;
        } else if (fidVal == ECG_FID_PPEAK) {
            numPPeaks++;
        } else if (fidVal == ECG_FID_TPEAK) {
            numTPeaks++;
        }
    }

    // Filter RR intervals and peaks
    pk_ecg_compute_rr_intervals(peaksMetrics, numQrsPeaks, rriMetrics);
    pk_ecg_filter_rr_intervals(rriMetrics, numQrsPeaks, metricsMask, SAMPLE_RATE, MIN_RR_SEC, MAX_RR_SEC, MIN_RR_DELTA);

    // Annotate ecgMask with beats
    results->hr = 0;
    for (size_t i = 0; i < numQrsPeaks; i++) {
        if (metricsMask[i] == 1) {
            beatVal = ECG_BEAT_NOISE;
        } else {
            results->hr += 60 / (rriMetrics[i] / SAMPLE_RATE);
            beatVal = ECG_BEAT_NSR;
            numBeats += 1;
        }
        ecgMask[peaksMetrics[i]] |= ((beatVal & SIG_MASK_BEAT_MASK) << SIG_MASK_BEAT_OFFSET);
    }
    results->hr /= MAX(1, numBeats);

    // Annotate ecgMask with QoS
    // // If too many noise beats, hr out of range, then mark as bad QoS
    // badPeakPerc = numBeats/MAX(1, numQrsPeaks);
    // if (numBeats <= 2 || badPeakPerc > 0.5 || results->hr < 40 || results->hr > 200) {
    //     for (size_t i = 0; i < len; i++) {
    //         ecgMask[i] |= ((SIG_QOS_BAD & SIG_MASK_QOS_MASK) << SIG_MASK_QOS_OFFSET);
    //     }
    // }

    ns_lp_printf("nPpeaks=%d, nQrs=%d, nTpeaks=%d\n", numPPeaks, numQrsPeaks, numTPeaks);
    ns_lp_printf("nbeats=%d, HR=%0.2f\n\n", numBeats, results->hr);

    return err;
}

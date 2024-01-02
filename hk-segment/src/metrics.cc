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
    float32_t *ppg2,
    uint16_t *ppgMask,
    size_t len,
    metrics_ppg_results_t *results
) {
    uint32_t err = 0;
    uint16_t peakVal, beatVal;
    float32_t ppg1Mean, ppg2Mean;
    size_t numPeaks = 0, numBeats = 0;
    float32_t *ppgSum = ppgFindPeakCtx.state;
    uint16_t ppgQos = SIG_QOS_GOOD;
    float32_t spo2 = 0;
    float32_t hr = 0;

    // Filter PPG signals
    err |= pk_mean_f32(ppg1, &ppg1Mean, len);
    pk_apply_biquad_filter_f32(&ppg1FilterCtx, ppg1, ppg1, len);
    pk_standardize_f32(ppg1, ppg1, len, NORM_STD_EPS);

    err |= pk_mean_f32(ppg2, &ppg2Mean, len);
    pk_apply_biquad_filter_f32(&ppg2FilterCtx, ppg2, ppg2, len);
    pk_standardize_f32(ppg2, ppg2, len, NORM_STD_EPS);

    // Compute sum of PPG signals
    for (size_t i = 0; i < len; i++) {
        ppgSum[i] = ppg1[i] + ppg2[i];
    }

    // Find peaks
    numPeaks = pk_ppg_find_peaks_f32(&ppgFindPeakCtx, ppgSum, len, peaksMetrics);
    pk_ppg_compute_rr_intervals(peaksMetrics, numPeaks, rriMetrics);
    pk_ppg_filter_rr_intervals(rriMetrics, numPeaks, rriMask, PPG_SAMPLE_RATE, MIN_RR_SEC, MAX_RR_SEC, MIN_RR_DELTA);

    // Annotate ppg mask with systolic beats
    for (size_t i = 0; i < numPeaks; i++) {
        if (rriMask[i] == 1) {
            peakVal = PPG_FID_PEAK_NONE;
            beatVal = PPG_FID_BEAT_NOISE;
        } else {
            peakVal = PPG_FID_PEAK_SPEAK;
            beatVal = PPG_FID_BEAT_NSR;
            hr += 60 / (rriMetrics[i] / PPG_SAMPLE_RATE);
            numBeats += 1;
        }
        ppgMask[peaksMetrics[i]] |= ((peakVal & PPG_MASK_FID_PEAK_MASK) << PPG_MASK_FID_PEAK_OFFSET);
        ppgMask[peaksMetrics[i]] |= ((beatVal & PPG_MASK_FID_BEAT_MASK) << PPG_MASK_FID_BEAT_OFFSET);
    }
    hr /= MAX(1, numBeats);

    // Annotate PPG mask with QoS
    if (ppg1Mean < PPG_MET_MIN_VAL || ppg2Mean < PPG_MET_MIN_VAL) {
        ppgQos = SIG_QOS_BAD;
    }
    for (size_t i = 0; i < len; i++) {
        ppgMask[i] |= ((ppgQos & SIG_MASK_QOS_MASK) << SIG_MASK_QOS_OFFSET);
    }
    ns_lp_printf("PPG: npeaks=%d, mu1=%0.2f, mu2=%0.2f, qos=%0.2f\n", numPeaks, ppg1Mean, ppg2Mean, ppgQos);

    // Compute SpO2
    if (ppgQos > SIG_QOS_POOR) {
        spo2 = pk_ppg_compute_spo2_in_time_f32(ppg1, ppg2, ppg1Mean, ppg2Mean, len, spo2Coefs, PPG_SAMPLE_RATE);
    }
    results->hr = hr;
    results->spo2 = spo2;
    return err;
}


uint32_t
metrics_capture_ecg(
    metrics_config_t *ctx,
    float32_t *ecg,
    uint16_t *ecgMask,
    size_t len,
    metrics_ppg_results_t *results
) {
    uint32_t err = 0;
    uint16_t peakVal, beatVal;
    // float32_t badPeakPerc = 0;
    size_t numPPeaks = 0, numQrsPeaks = 0, numTPeaks = 0, numBeats = 0;
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

    // Annotate mask with beats
    for (size_t i = 0; i < numQrsPeaks; i++) {
        if (rriMask[i] == 1) {
            beatVal = ECG_FID_BEAT_NOISE;
        } else {
            hr += 60 / (rriMetrics[i] / ECG_SAMPLE_RATE);
            beatVal = ECG_FID_BEAT_NSR;
            numBeats += 1;
        }
        ecgMask[peaksMetrics[i]] |= ((beatVal & ECG_MASK_FID_BEAT_MASK) << ECG_MASK_FID_BEAT_OFFSET);
    }
    hr /= MAX(1, numBeats);

    // Annotate ecgMask with QoS
    // // If too many noise beats, hr out of range, then mark as bad QoS
    // badPeakPerc = numBeats/MAX(1, numQrsPeaks);
    // if (numBeats <= 2 || badPeakPerc > 0.5 || results->hr < 40 || results->hr > 200) {
    //     for (size_t i = 0; i < len; i++) {
    //         ecgMask[i] |= ((SIG_QOS_BAD & SIG_MASK_QOS_MASK) << SIG_MASK_QOS_OFFSET);
    //     }
    // }

    ns_lp_printf("nPpeaks=%d, nQrs=%d, nTpeaks=%d\n", numPPeaks, numQrsPeaks, numTPeaks);
    ns_lp_printf("nbeats=%d, HR=%0.2f\n\n", numBeats, hr);
    results->hr = hr;
    return err;
}

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
    arm_rfft_fast_init_f32(&ppgFftCtx, PPG_MET_FFT_WINDOW_LEN);
    pk_blackman_window_f32(ppgFftWindow, PPG_MET_WINDOW_LEN);
    return err;
}

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
) {
    uint32_t err = 0;
    uint16_t peakVal, beatVal;
    uint32_t maxIdx = 0;
    float32_t maxVal = 0;
    float32_t psSum = 0;
    size_t numPeaks = 0, numBeats = 0;
    float32_t *ppgSum = ppgFindPeakCtx.state;
    uint16_t ppgQos = SIG_QOS_GOOD;
    float32_t spo2 = 0;
    float32_t hr = 0;
    float32_t pr = 0;


    // Compute sum of PPG signals
    for (size_t i = 0; i < len; i++) {
        ppgSum[i] = ppg1[i] + ppg2[i];
        ppgFftData[i] = ppgSum[i] * ppgFftWindow[i];
    }
    // Zero pad
    for (size_t i = len; i < PPG_MET_FFT_WINDOW_LEN; i++) {
        ppgFftData[i] = 0;
    }

    // Compute FFT and magnitude
    psSum = 0;
    arm_rfft_fast_f32(&ppgFftCtx, ppgFftData, ppgFftData, 0);
    arm_cmplx_mag_squared_f32(ppgFftData, ppgFftData, PPG_MET_FFT_WINDOW_LEN / 2);
    // Find dominant frequency
    for (size_t i = PPG_FFT_MIN_IDX; i < PPG_FFT_MAX_IDX; i++) {
        psSum += ppgFftData[i];
        if (ppgFftData[i] > maxVal) {
            maxIdx = i;
            maxVal = ppgFftData[i];
        }
    }
    pr = 60.0f * maxIdx * PPG_SAMPLE_RATE / (float32_t)PPG_MET_FFT_WINDOW_LEN;

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
            hr += 60.0f / (rriMetrics[i] / (float32_t)PPG_SAMPLE_RATE);
            numBeats += 1;
        }
        ppgMask[peaksMetrics[i]] |= ((peakVal & PPG_MASK_FID_PEAK_MASK) << PPG_MASK_FID_PEAK_OFFSET);
        ppgMask[peaksMetrics[i]] |= ((beatVal & PPG_MASK_FID_BEAT_MASK) << PPG_MASK_FID_BEAT_OFFSET);
    }
    hr /= MAX(1.0f, numBeats);


    // Annotate PPG mask with QoS
    if (ppg1Mean < PPG_MET_MIN_VAL || ppg2Mean < PPG_MET_MIN_VAL) {
        ppgQos = SIG_QOS_BAD;
    }
    for (size_t i = 0; i < len; i++) {
        ppgMask[i] |= ((ppgQos & SIG_MASK_QOS_MASK) << SIG_MASK_QOS_OFFSET);
    }
    // ns_lp_printf("PPG: npeaks=%d, mu1=%0.2f, mu2=%0.2f, qos=%0.2f\n", numPeaks, ppg1Mean, ppg2Mean, ppgQos);

    // Compute SpO2
    if (ppgQos > SIG_QOS_POOR) {
        spo2 = CLIP(pk_ppg_compute_spo2_in_time_f32(ppg1, ppg2, ppg1Mean, ppg2Mean, len, spo2Coefs, PPG_SAMPLE_RATE), 90, 100);
    }
    ns_lp_printf("PR=%0.2f, HR=%0.2f, SpO2=%0.2f fftQoS=%0.2f\n\n", pr, hr, spo2, maxVal/psSum);
    results->pr = pr;
    results->spo2 = spo2;
    return err;
}

uint32_t
metrics_capture_ecg(
    metrics_config_t *ctx,
    float32_t *ecg,
    uint16_t *ecgMask,
    size_t len,
    metrics_ecg_results_t *results
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

    ns_lp_printf("nPWAVE=%d, nQRS=%d, nTWAVE=%d\n", numPPeaks, numQrsPeaks, numTPeaks);
    ns_lp_printf("nBEATS=%d, nNOISE=%d, HR=%0.2f, HRV=%0.2f\n\n", numBeats, numNoiseBeats, results->hr, results->hrv);
    return err;
}

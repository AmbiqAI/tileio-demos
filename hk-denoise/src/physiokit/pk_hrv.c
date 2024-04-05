/**
 * @file pk_hrv.c
 * @author Adam Page (adam.page@ambiq.com)
 * @brief PhysioKit: Heart Rate Variability
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <math.h>
#include "arm_math.h"

#include "pk_filter.h"
#include "pk_hrv.h"


uint32_t
pk_hrv_compute_time_metrics_from_rr_intervals(uint32_t *rrIntervals, uint32_t numPeaks, uint8_t *mask, hrv_td_metrics_t *metrics, uint32_t sampleRate) {
    // Deviation-based
    metrics->meanNN = 0;
    metrics->sdNN = 0;
    uint32_t numValid = 0;
    float32_t val;
    for (size_t i = 0; i < numPeaks; i++){
        if (mask[i] == 0) {
            val = 1000.0*rrIntervals[i]/sampleRate;
            metrics->meanNN += val;
            metrics->sdNN += val*val;
            numValid++;
        }
    }
    metrics->meanNN /= numValid;
    metrics->sdNN = sqrt(metrics->sdNN/numValid - metrics->meanNN*metrics->meanNN);

    // Difference-based
    float32_t meanSD = 0;
    metrics->rmsSD = 0;
    metrics->sdSD = 0;
    metrics->nn20 = 0;
    metrics->nn50 = 0;
    metrics->minNN = -1;
    metrics->maxNN = -1;
    float32_t v1, v2, v3, v4, v5;
    numValid = 0;
    for (size_t i = 1; i < numPeaks; i++)
    {
        if (mask[i] == 1 || mask[i - 1] == 1) {
            continue;
        }
        numValid++;
        v1 = 1000.0*rrIntervals[i - 1]/(float32_t)sampleRate;
        v2 = 1000.0*rrIntervals[i]/(float32_t)sampleRate;
        v3 = (v2 - v1);
        v4 = v3*v3;
        v5 = fabsf(v3);
        meanSD += v3;
        metrics->rmsSD += v4;
        metrics->sdSD += v4;
        if (v5 > 20) {
            metrics->nn20++;
        }
        if (v5 > 50) {
            metrics->nn50++;
        }
        if (rrIntervals[i] < metrics->minNN || metrics->minNN == -1) {
            metrics->minNN = rrIntervals[i];
        }
        if (rrIntervals[i] > metrics->maxNN || metrics->maxNN == -1) {
            metrics->maxNN = rrIntervals[i];
        }
    }
    if (numValid > 1) {
        meanSD /= numValid;
        metrics->rmsSD = sqrtf(metrics->rmsSD/numValid);
        metrics->sdSD = sqrtf(metrics->sdSD/numValid - meanSD*meanSD);
        metrics->pnn20 = 100.0f*metrics->nn20/numValid;
        metrics->pnn50 = 100.0f*metrics->nn50/numValid;
    }
    // Normalized
    metrics->cvNN = metrics->sdNN/metrics->meanNN;
    metrics->cvSD = metrics->rmsSD/metrics->meanNN;

    // Robust
    metrics->medianNN = 0;
    metrics->madNN = 0;
    metrics->mcvNN = 0;
    // Use mean & std for IQR
    float32_t q1 = metrics->meanNN - 0.6745*metrics->sdNN;
    float32_t q3 = metrics->meanNN + 0.6745*metrics->sdNN;
    metrics->iqrNN = q3 - q1;
    metrics->prc20NN = 0;
    metrics->prc80NN = 0;
    return 0;
}

uint32_t
pk_hrv_compute_freq_metrics_from_rr_intervals(uint32_t *rrIntervals, uint32_t numPeaks, uint8_t *mask, hrv_fd_metrics_t *metrics){
    return 1;
}

#include "ppg_segmentation.h"
#include "arm_math.h"
#include "constants.h"
#include "store.h"
#include "physiokit/pk_math.h"
#include "physiokit/pk_ppg.h"
#include "physiokit/pk_filter.h"


uint32_t
ppg_segmentation_init() {
    arm_rfft_fast_init_f32(&ppgSegFftCtx, PPG_SEG_FFT_WINDOW_LEN);
    pk_blackman_window_f32(ppgSegFftWindow, PPG_SEG_WINDOW_LEN);
    return 0;
}

uint32_t
ppg_segmentation_inference(float32_t *ppg1, float32_t *ppg2, uint16_t *mask, uint32_t padLen, float32_t threshold){

    uint32_t err = 0;
    uint32_t fftMaxIdx = 0;
    float32_t fftMaxVal = 0;
    float32_t fftSum;
    float32_t *ppgSum = ppgSegScratch;
    uint16_t qos;
    float32_t pr;

    size_t len = PPG_SEG_WINDOW_LEN;

    memset(mask, 0, PPG_SEG_WINDOW_LEN*sizeof(uint16_t));

    // Compute sum of PPG signals
    for (size_t i = 0; i < len; i++) {
        ppgSum[i] = ppg1[i] + ppg2[i];
        ppgSegFftData[i] = ppgSum[i] * ppgSegFftWindow[i];
    }
    // Zero pad
    for (size_t i = len; i < PPG_SEG_FFT_WINDOW_LEN; i++) {
        ppgSegFftData[i] = 0;
    }

    // Compute FFT and magnitude
    fftSum = 0;
    arm_rfft_fast_f32(&ppgSegFftCtx, ppgSegFftData, ppgSegFftData, 0);
    arm_cmplx_mag_squared_f32(ppgSegFftData, ppgSegFftData, PPG_SEG_FFT_WINDOW_LEN / 2);
    // Find dominant frequency
    for (size_t i = PPG_SEG_FFT_MIN_IDX; i < PPG_SEG_FFT_MAX_IDX; i++) {
        fftSum += ppgSegFftData[i];
        if (ppgSegFftData[i] > fftMaxVal) {
            fftMaxIdx = i;
            fftMaxVal = ppgSegFftData[i];
        }
    }
    pr = 60.0f * fftMaxIdx * PPG_SAMPLE_RATE / (float32_t)PPG_SEG_FFT_WINDOW_LEN;

    fftMaxVal /= (fftSum + NORM_STD_EPS);
    qos = fftMaxVal > 0.35 ? SIG_QOS_GOOD : fftMaxVal > 0.3 ? SIG_QOS_FAIR : fftMaxVal > 0.2 ? SIG_QOS_POOR : SIG_QOS_BAD;

    for (size_t i = 0; i < len; i++) {
        mask[i] |= ((qos & SIG_MASK_QOS_MASK) << SIG_MASK_QOS_OFFSET);
    }

    ns_lp_printf("PPG Segmentation QoS: %f PR=%0.2f\n", fftMaxVal, pr);

    return err;

}

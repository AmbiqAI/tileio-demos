/**
 * @file nstdb_noise.cc
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Add noise to the sensor data
 * @version 1.0
 * @date 2023-03-27
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "arm_math.h"
#include "stimulus.h"
#include "nstdb_noise.h"
#include "constants.h"

uint32_t
nstdb_add_em_noise(float32_t *pSrc, float32_t *pDst, size_t len, float32_t ratio){
    static size_t _em_noise_idx = 0;
    float32_t noise_val;
    for (size_t i = 0; i < len; i++){
        noise_val = em_noise_stimulus[_em_noise_idx] * ratio;
        _em_noise_idx = (_em_noise_idx + 1) % em_noise_stimulus_len;
        pDst[i] = pSrc[i] + noise_val;
    }
    return 0;
}


uint32_t
nstdb_add_ma_noise(float32_t *pSrc, float32_t *pDst, size_t len, float32_t ratio){
    static size_t _ma_noise_idx = 0;
    float32_t noise_val;
    for (size_t i = 0; i < len; i++){
        noise_val = ma_noise_stimulus[_ma_noise_idx] * ratio;
        _ma_noise_idx = (_ma_noise_idx + 1) % ma_noise_stimulus_len;
        pDst[i] = pSrc[i] + noise_val;
    }
    return 0;
}


uint32_t
nstdb_add_bw_noise(float32_t *pSrc, float32_t *pDst, size_t len, float32_t ratio){
    static size_t _bw_noise_idx = 0;
    float32_t noise_val;
    for (size_t i = 0; i < len; i++){
        noise_val = bw_noise_stimulus[_bw_noise_idx] * ratio;
        _bw_noise_idx = (_bw_noise_idx + 1) % bw_noise_stimulus_len;
        pDst[i] = pSrc[i] + noise_val;
    }
    return 0;
}

uint32_t
nstdb_add_noises(float32_t *pSrc, float32_t *pDst, size_t len, int32_t noiseLevel, float32_t scale) {
    noiseLevel = MAX(3, noiseLevel);
    int32_t bwNoise = noiseLevel / 3; // rand() % noiseLevel;
    int32_t emNoise = noiseLevel / 3; // rand() % (noiseLevel - bwNoise);
    int32_t maNoise = noiseLevel / 3; // MAX(0, noiseLevel - bwNoise - emNoise);
    nstdb_add_bw_noise(pSrc, pDst, len, bwNoise*scale);
    nstdb_add_em_noise(pSrc, pDst, len, emNoise*scale);
    nstdb_add_ma_noise(pSrc, pDst, len, maNoise*scale);
    return 0;
}

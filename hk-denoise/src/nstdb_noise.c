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

uint32_t
nstdb_add_em_noise(float32_t *ecg, size_t len, float32_t ratio){
    static size_t _em_noise_idx = 0;
    float32_t noise_val;
    for (size_t i = 0; i < len; i++){
        noise_val = em_noise_stimulus[_em_noise_idx] * ratio;
        _em_noise_idx = (_em_noise_idx + 1) % em_noise_stimulus_len;
        ecg[i] += noise_val;
    }
    return 0;
}


uint32_t
nstdb_add_ma_noise(float32_t *ecg, size_t len, float32_t ratio){
    static size_t _ma_noise_idx = 0;
    float32_t noise_val;
    for (size_t i = 0; i < len; i++){
        noise_val = ma_noise_stimulus[_ma_noise_idx] * ratio;
        _ma_noise_idx = (_ma_noise_idx + 1) % ma_noise_stimulus_len;
        ecg[i] += noise_val;
    }
    return 0;
}


uint32_t
nstdb_add_bw_noise(float32_t *ecg, size_t len, float32_t ratio){
    static size_t _bw_noise_idx = 0;
    float32_t noise_val;
    for (size_t i = 0; i < len; i++){
        noise_val = bw_noise_stimulus[_bw_noise_idx] * ratio;
        _bw_noise_idx = (_bw_noise_idx + 1) % bw_noise_stimulus_len;
        ecg[i] += noise_val;
    }
    return 0;
}

/**
 * @file nstdb_noise.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Add noise to the sensor data
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __NSTD_NOISE_H
#define __NSTD_NOISE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "arm_math.h"
#include <stdint.h>

uint32_t
nstdb_add_em_noise(float32_t *ecg, size_t len, float32_t ratio);
uint32_t
nstdb_add_ma_noise(float32_t *ecg, size_t len, float32_t ratio);
uint32_t
nstdb_add_bw_noise(float32_t *ecg, size_t len, float32_t ratio);

#ifdef __cplusplus
}
#endif

#endif // __NSTD_NOISE_H

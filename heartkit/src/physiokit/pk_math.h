/**
 * @file pk_math.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief PhysioKit: Math
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __PK_MATH_H
#define __PK_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "arm_math.h"

uint32_t
pk_mean_f32(float32_t *pSrc, float32_t *pResult, uint32_t blockSize);

uint32_t
pk_std_f32(float32_t *pSrc, float32_t *pResult, uint32_t blockSize);

uint32_t
pk_gradient_f32(float32_t *pSrc, float32_t *pResult, uint32_t blockSize);

uint32_t
pk_rms_f32(float32_t *pSrc, float32_t *pResult, uint32_t blockSize);

uint32_t
pk_next_power_of_2(uint32_t val);

/**
 * @brief Compute cosine similarity
 *
 * @param ref Reference signal
 * @param sig Signal to compare
 * @param len Length of signals
 * @param result Result of comparison
 * @return uint32_t
 */
uint32_t
cosine_similarity_f32(float32_t *ref, float32_t *sig, size_t len, float32_t *result);

#ifdef __cplusplus
}
#endif

#endif // __PK_MATH_H

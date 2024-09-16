/**
 * @file ecg_denoise.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief TFLM ECG denoise
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifndef __HK_ECG_ARRHYTHMIA_H
#define __HK_ECG_ARRHYTHMIA_H

#include <stdint.h>
#include "arm_math.h"
#include "tflm.h"


/**
 * @brief Initialize ECG arrhythmia model
 *
 * @param ctx TFLM model context
 * @return uint32_t
 */
uint32_t
ecg_arrhythmia_init(tf_model_context_t *ctx);

/**
 * @brief Run ECG arrhythmia model
 *
 * @param ctx TFLM model context
 * @param ecgIn ECG input
 * @param threshold Threshold
 * @return uint32_t class label
 */
uint32_t
ecg_arrhythmia_inference(tf_model_context_t *ctx, float32_t *ecgIn, float32_t threshold);

#endif // __HK_ECG_ARRHYTHMIA_H

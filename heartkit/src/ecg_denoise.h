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

#ifndef __HK_ECG_DENOISE_H
#define __HK_ECG_DENOISE_H

#include <stdint.h>
#include "arm_math.h"
#include "tflm.h"


/**
 * @brief Initialize ECG denoise model
 *
 * @param ctx TFLM model context
 * @return uint32_t
 */
uint32_t
ecg_denoise_init(tf_model_context_t *ctx);

/**
 * @brief Run ECG denoise model
 *
 * @param ctx TFLM model context
 * @param ecgIn ECG input
 * @param ecgOut Denoised ECG output
 * @param padLen Padding length
 * @param threshold Threshold
 * @return uint32_t
 */
uint32_t
ecg_denoise_inference(tf_model_context_t *ctx, float32_t *ecgIn, float32_t *ecgOut, uint32_t padLen, float32_t threshold);

#endif // __HK_ECG_DENOISE_H

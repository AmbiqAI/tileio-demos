/**
 * @file ecg_segmentation.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief TFLM ECG segmentation
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __PK_ECG_DENOISE_H
#define __PK_ECG_DENOISE_H

#include <stdint.h>
#include "arm_math.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"

typedef struct {
    size_t arenaSize;
    uint8_t *arena;
    const unsigned char *buffer;
    const tflite::Model *model;
    TfLiteTensor *input;
    TfLiteTensor *output;
    tflite::MicroInterpreter *interpreter;
} tf_model_context_t;


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
 * @param ecgMask ECG mask
 * @param padLen Padding length
 * @param threshold Segmentation threshold
 * @return uint32_t
 */
uint32_t
ecg_denoise_inference(tf_model_context_t *ctx, float32_t *ecgIn, float32_t *ecgOut, uint16_t *ecgMask, uint32_t padLen, float32_t threshold);

#endif // __PK_ECG_DENOISE_H

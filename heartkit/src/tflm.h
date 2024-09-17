/**
 * @file tflm.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief TFLM helper functions
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __HK_TFLM_H
#define __HK_TFLM_H

#include <stdint.h>
#include "arm_math.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_profiler.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/schema/schema_generated.h"


using TflmOpResolver = tflite::MicroMutableOpResolver<113>;
using TflmErrorReport = tflite::MicroErrorReporter;
using TflmProfiler = tflite::MicroProfiler;

typedef struct {
    size_t arenaSize;
    uint8_t *arena;
    const unsigned char *buffer;
    const tflite::Model *model;
    TfLiteTensor *input;
    TfLiteTensor *output;
    TflmErrorReport *reporter;
    TflmProfiler *profiler;
    TflmOpResolver *resolver;
    tflite::MicroInterpreter *interpreter;
} tf_model_context_t;


uint32_t
tflm_init();

uint32_t
tflm_init_model(tf_model_context_t *ctx);


#endif // __HK_TFLM_H

/**
 * @file ecg_denoise.cc
 * @author Adam Page (adam.page@ambiq.com)
 * @brief TFLM ECG denoise
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "arm_math.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// neuralSPOT
#include "ns_ambiqsuite_harness.h"
// TFLM
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_op_resolver.h"
#include "tensorflow/lite/micro/micro_profiler.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/schema/schema_generated.h"
// Locals
#include "tflm.h"
#include "physiokit/pk_ecg.h"
#include "store.h"
#include "constants.h"
#include "ecg_denoise.h"

uint32_t
ecg_denoise_init(tf_model_context_t *ctx) {

    size_t bytesUsed;
    TfLiteStatus allocateStatus;

    tflm_init_model(ctx);

    ctx->model = tflite::GetModel(ctx->buffer);

    if (ctx->model->version() != TFLITE_SCHEMA_VERSION) {
        TF_LITE_REPORT_ERROR(ctx->reporter, "Schema mismatch: given=%d != expected=%d.", ctx->model->version(), TFLITE_SCHEMA_VERSION);
        return 1;
    }
    if (ctx->interpreter != nullptr) {
        ctx->interpreter->Reset();
    }
    static tflite::MicroInterpreter denoise_interpreter(ctx->model, *(ctx->resolver), ctx->arena, ctx->arenaSize, nullptr, ctx->profiler);

    ctx->interpreter = &denoise_interpreter;

    allocateStatus = ctx->interpreter->AllocateTensors();
    if (allocateStatus != kTfLiteOk) {
        TF_LITE_REPORT_ERROR(ctx->reporter, "AllocateTensors() failed");
        return 1;
    }

    bytesUsed = ctx->interpreter->arena_used_bytes();
    if (bytesUsed > ctx->arenaSize) {
        TF_LITE_REPORT_ERROR(ctx->reporter, "Arena mismatch: given=%d < expected=%d bytes.", ctx->arenaSize, bytesUsed);
        return 1;
    }

    ctx->input = ctx->interpreter->input(0);
    ctx->output = ctx->interpreter->output(0);
    return 0;
}

uint32_t
ecg_denoise_inference(tf_model_context_t *ctx, float32_t *ecgIn, float32_t *ecgOut, uint32_t padLen, float32_t threshold) {

    // Copy input
    for (size_t i = 0; i < ECG_DEN_WINDOW_LEN; i++) {
        if (ctx->input->quantization.type == kTfLiteAffineQuantization) {
            ctx->input->data.int8[i] = ecgIn[i] / ctx->input->params.scale + ctx->input->params.zero_point;
        } else {
            ctx->input->data.f[i] = ecgIn[i];
        }
    }

    // Invoke model
    TfLiteStatus invokeStatus = ctx->interpreter->Invoke();
    if (invokeStatus != kTfLiteOk) {
        return invokeStatus;
    }

    // Copy output
    for (int i = padLen; i < ctx->output->dims->data[1] - (int)padLen; i++) {
        if (ctx->output->quantization.type == kTfLiteAffineQuantization) {
            ecgOut[i] = ((float32_t)ctx->output->data.int8[i] - ctx->output->params.zero_point) * ctx->output->params.scale;
        } else {
            ecgOut[i] = ctx->output->data.f[i];
        }
    }
    return 0;
}

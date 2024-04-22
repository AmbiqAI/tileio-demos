/**
 * @file ecg_segmentation.cc
 * @author Adam Page (adam.page@ambiq.com)
 * @brief TFLM ECG segmentation
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
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
#include "tensorflow/lite/micro/micro_profiler.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/schema/schema_generated.h"
// Locals
#include "physiokit/pk_ecg.h"
#include "physiokit/pk_hrv.h"
#include "store.h"
#include "constants.h"
#include "ecg_segmentation.h"

uint32_t
ecg_segmentation_init(tf_model_context_t *ctx) {

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
    static tflite::MicroInterpreter static_interpreter(ctx->model, *(ctx->resolver), ctx->arena, ctx->arenaSize, nullptr, ctx->profiler);

    ctx->interpreter = &static_interpreter;

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
ecg_segmentation_inference(tf_model_context_t *ctx, float32_t *data, uint16_t *segMask, uint32_t padLen, float32_t threshold) {
    uint32_t yIdx = 0;
    uint8_t yMaxIdx = 0;
    float32_t yVal = 0;
    float32_t yMax = 0;
    uint16_t qos = 0;
    float32_t avgQos = 0;

    // Copy data to input
    for (size_t i = 0; i < ECG_SEG_WINDOW_LEN; i++) {
        if (ctx->input->quantization.type == kTfLiteAffineQuantization) {
            ctx->input->data.int8[i] = data[i] / ctx->input->params.scale + ctx->input->params.zero_point;
        } else {
            ctx->input->data.f[i] = data[i];
        }
    }

    // Invoke model
    TfLiteStatus invokeStatus = ctx->interpreter->Invoke();
    if (invokeStatus != kTfLiteOk) { return invokeStatus; }

    // Extract output and segmentation mask
    for (int i = padLen; i < ctx->output->dims->data[1] - (int)padLen; i++) {
        for (int j = 0; j < ctx->output->dims->data[2]; j++) { // CLASSES
            yIdx = i * ctx->output->dims->data[2] + j;
            if (ctx->output->quantization.type == kTfLiteAffineQuantization) {
                yVal = ((float32_t)ctx->output->data.int8[yIdx] - ctx->output->params.zero_point) * ctx->output->params.scale;
            } else  {
                yVal = ctx->output->data.f[yIdx];
            }
            if ((j == 0) || (yVal > yMax)) {
                yMax = yVal;
                yMaxIdx = j;
            }
        }
        qos = yMax > ECG_QOS_GOOD_THRESH ? 3 : yMax > ECG_QOS_FAIR_THRESH ? 2 : yMax > ECG_QOS_POOR_THRESH ? 1 : 0;
        avgQos += yMax;
        if (false && yMaxIdx > 0) {
            ns_lp_printf("Segment (%d, %d): QoS (%d, %f)\n", yMaxIdx, i, qos, yMax);
        }
        segMask[i] = yMax >= threshold ? yMaxIdx : 0;
        segMask[i] |= ((qos & SIG_MASK_QOS_MASK) << SIG_MASK_QOS_OFFSET);
    }
    avgQos /= (ctx->output->dims->data[1] - 2 * padLen);
    ns_lp_printf("ECG Segmentation QoS: %f\n", avgQos);

    if (avgQos < ECG_QOS_BAD_AVG_THRESH) {
        for (int i = padLen; i < ctx->output->dims->data[1] - (int)padLen; i++) {
            segMask[i] = 0;
            data[i] = 0;
        }
    }

    // TODO: Fix gaps in segmentation. Merge same segment types that are separated by less than N samples

    // Extract fiducial points
    uint16_t prevSegVal = segMask[0] & SIG_MASK_SEG_MASK;
    uint16_t segVal = 0;
    int startIdx = 0;
    float32_t maxVal = abs(data[0]);
    int maxIdx = 0;
    for (size_t i = 1; i < ECG_SEG_WINDOW_LEN; i++) {
        // If start of segment, reset max
        segVal = segMask[i] & SIG_MASK_SEG_MASK;
        if ((segVal != 0) && (prevSegVal == 0)) {
            startIdx = i;
            maxVal = abs(data[i]);
            maxIdx = i;
        }
        // If end of segment, mark fiducial
        else if ((segVal == 0) && (prevSegVal != 0)) {
            if (startIdx >= 0 && (i - startIdx > 2)) {
                // Fiducial peak value (e.g p-peak) will be same as segmentation value (e.g. p-wave)
                segMask[maxIdx] |= (prevSegVal << ECG_MASK_FID_PEAK_OFFSET);
                // ns_lp_printf("Segment (%d, %d, %d): Fiducial (%d, %f)\n", segMask[maxIdx], startIdx, i, maxIdx, maxVal);
            }
            startIdx = -1;
        } else if (startIdx >= 0) {
            if (abs(data[i]) > maxVal) {
                maxVal = abs(data[i]);
                maxIdx = i;
            }
        }
        prevSegVal = segVal;
    }
    return 0;
}

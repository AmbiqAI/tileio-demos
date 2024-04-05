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
#include "store.h"
#include "constants.h"
#include "ecg_denoise.h"


static tflite::ErrorReporter *errorReporter = nullptr;
static tflite::MicroMutableOpResolver<113> *opResolver = nullptr;
static tflite::MicroProfiler *profiler = nullptr;

uint32_t
ecg_denoise_init(tf_model_context_t *ctx) {

    size_t bytesUsed;
    TfLiteStatus allocateStatus;

    tflite::MicroErrorReporter micro_error_reporter;
    errorReporter = &micro_error_reporter;

    tflite::InitializeTarget();

    static tflite::MicroMutableOpResolver<113> resolver;
    opResolver = &resolver;

    // Add all the ops to the resolver
    resolver.AddAbs();
    resolver.AddAdd();
    resolver.AddAddN();
    resolver.AddArgMax();
    resolver.AddArgMin();
    resolver.AddAssignVariable();
    resolver.AddAveragePool2D();
    resolver.AddBatchMatMul();
    resolver.AddBatchToSpaceNd();
    resolver.AddBroadcastArgs();
    resolver.AddBroadcastTo();
    resolver.AddCallOnce();
    resolver.AddCast();
    resolver.AddCeil();
    resolver.AddCircularBuffer();
    resolver.AddConcatenation();
    resolver.AddConv2D();
    resolver.AddCos();
    resolver.AddCumSum();
    resolver.AddDelay();
    resolver.AddDepthToSpace();
    resolver.AddDepthwiseConv2D();
    resolver.AddDequantize();
    resolver.AddDetectionPostprocess();
    resolver.AddDiv();
    resolver.AddEmbeddingLookup();
    resolver.AddEnergy();
    resolver.AddElu();
    resolver.AddEqual();
    resolver.AddEthosU();
    resolver.AddExp();
    resolver.AddExpandDims();
    resolver.AddFftAutoScale();
    resolver.AddFill();
    resolver.AddFilterBank();
    resolver.AddFilterBankLog();
    resolver.AddFilterBankSquareRoot();
    resolver.AddFilterBankSpectralSubtraction();
    resolver.AddFloor();
    resolver.AddFloorDiv();
    resolver.AddFloorMod();
    resolver.AddFramer();
    resolver.AddFullyConnected();
    resolver.AddGather();
    resolver.AddGatherNd();
    resolver.AddGreater();
    resolver.AddGreaterEqual();
    resolver.AddHardSwish();
    resolver.AddIf();
    resolver.AddIrfft();
    resolver.AddL2Normalization();
    resolver.AddL2Pool2D();
    resolver.AddLeakyRelu();
    resolver.AddLess();
    resolver.AddLessEqual();
    resolver.AddLog();
    resolver.AddLogicalAnd();
    resolver.AddLogicalNot();
    resolver.AddLogicalOr();
    resolver.AddLogistic();
    resolver.AddLogSoftmax();
    resolver.AddMaximum();
    resolver.AddMaxPool2D();
    resolver.AddMirrorPad();
    resolver.AddMean();
    resolver.AddMinimum();
    resolver.AddMul();
    resolver.AddNeg();
    resolver.AddNotEqual();
    resolver.AddOverlapAdd();
    resolver.AddPack();
    resolver.AddPad();
    resolver.AddPadV2();
    resolver.AddPCAN();
    resolver.AddPrelu();
    resolver.AddQuantize();
    resolver.AddReadVariable();
    resolver.AddReduceMax();
    resolver.AddRelu();
    resolver.AddRelu6();
    resolver.AddReshape();
    resolver.AddResizeBilinear();
    resolver.AddResizeNearestNeighbor();
    resolver.AddRfft();
    resolver.AddRound();
    resolver.AddRsqrt();
    resolver.AddSelectV2();
    resolver.AddShape();
    resolver.AddSin();
    resolver.AddSlice();
    resolver.AddSoftmax();
    resolver.AddSpaceToBatchNd();
    resolver.AddSpaceToDepth();
    resolver.AddSplit();
    resolver.AddSplitV();
    resolver.AddSqueeze();
    resolver.AddSqrt();
    resolver.AddSquare();
    resolver.AddSquaredDifference();
    resolver.AddStridedSlice();
    resolver.AddStacker();
    resolver.AddSub();
    resolver.AddSum();
    resolver.AddSvdf();
    resolver.AddTanh();
    resolver.AddTransposeConv();
    resolver.AddTranspose();
    resolver.AddUnpack();
    resolver.AddUnidirectionalSequenceLSTM();
    resolver.AddVarHandle();
    resolver.AddWhile();
    resolver.AddWindow();
    resolver.AddZerosLike();

    ctx->model = tflite::GetModel(ctx->buffer);

    if (ctx->model->version() != TFLITE_SCHEMA_VERSION) {
        TF_LITE_REPORT_ERROR(errorReporter, "Schema mismatch: given=%d != expected=%d.", ctx->model->version(), TFLITE_SCHEMA_VERSION);
        return 1;
    }
    if (ctx->interpreter != nullptr) {
        ctx->interpreter->Reset();
    }
    static tflite::MicroInterpreter static_interpreter(ctx->model, *opResolver, ctx->arena, ctx->arenaSize, nullptr, profiler);

    ctx->interpreter = &static_interpreter;

    allocateStatus = ctx->interpreter->AllocateTensors();
    if (allocateStatus != kTfLiteOk) {
        TF_LITE_REPORT_ERROR(errorReporter, "AllocateTensors() failed");
        return 1;
    }

    bytesUsed = ctx->interpreter->arena_used_bytes();
    if (bytesUsed > ctx->arenaSize) {
        TF_LITE_REPORT_ERROR(errorReporter, "Arena mismatch: given=%d < expected=%d bytes.", ctx->arenaSize, bytesUsed);
        return 1;
    }

    ctx->input = ctx->interpreter->input(0);
    ctx->output = ctx->interpreter->output(0);
    return 0;
}

uint32_t
ecg_denoise_inference(tf_model_context_t *ctx, float32_t *ecgIn, float32_t *ecgOut, uint16_t *ecgMask, uint32_t padLen, float32_t threshold) {

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

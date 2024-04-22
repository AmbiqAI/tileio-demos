
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
#include "tflm.h"

static TflmErrorReport *errorReporter = nullptr;
static TflmOpResolver *appOpResolver = nullptr;
static TflmProfiler *profiler = nullptr;

uint32_t
tflm_init() {

    tflite::MicroErrorReporter micro_error_reporter;
    errorReporter = &micro_error_reporter;

    tflite::InitializeTarget();

    static TflmOpResolver resolver;

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

    appOpResolver = &resolver;
    return 0;
}

uint32_t
tflm_init_model(tf_model_context_t *ctx){
    ctx->resolver = appOpResolver;
    ctx->reporter = errorReporter;
    ctx->profiler = profiler;
    return 0;
}

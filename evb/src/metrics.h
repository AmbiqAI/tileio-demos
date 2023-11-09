/**
 * @file metrics.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Perform preprocessing of sensor data (standardize and bandpass filter)
 * @version 1.0
 * @date 2023-03-27
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __PK_METRICS_H
#define __PK_METRICS_H

#include "arm_math.h"

uint32_t
init_metrics();
uint32_t
pk_preprocess(float32_t *ppg1Data, float32_t *ppg2Data, float32_t *ecgData, size_t dataLen);

#endif // __PK_METRICS_H

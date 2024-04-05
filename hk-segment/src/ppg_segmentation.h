/**
 * @file ppg_segmentation.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief TFLM ECG segmentation
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __HK_PPG_SEGMENTATION_H
#define __HK_PPG_SEGMENTATION_H

#include <stdint.h>
#include "arm_math.h"


/**
 * @brief Initialize PPG segmentation model
 *
 * @param ctx TFLM model context
 * @return uint32_t
 */
uint32_t
ppg_segmentation_init();

/**
 * @brief Run PPG segmentation model
 *
 * @param ppg1 PPG data
 * @param ppg2 PPG data
 * @param mask Segmentation mask
 * @param padLen Padding length
 * @param threshold Segmentation threshold
 * @return uint32_t
 */
uint32_t
ppg_segmentation_inference(float32_t *ppg1, float32_t *ppg2, uint16_t *mask, uint32_t padLen, float32_t threshold);

#endif // __HK_PPG_SEGMENTATION_H

/**
 * @file stimulus.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Test stimulus
 * @version 0.1
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __APP_STIMULUS_H
#define __APP_STIMULUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern const int16_t ecg_stimulus[];
extern const unsigned int ecg_stimulus_len;

extern const int16_t em_noise_stimulus[];
extern const unsigned int em_noise_stimulus_len;

extern const int16_t ma_noise_stimulus[];
extern const unsigned int ma_noise_stimulus_len;

extern const int16_t bw_noise_stimulus[];
extern const unsigned int bw_noise_stimulus_len;



#ifdef __cplusplus
}
#endif

#endif // __APP_STIMULUS_H

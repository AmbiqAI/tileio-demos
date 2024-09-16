/**
 * @file utils.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Utility functions
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __PK_UTILS_H
#define __PK_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "arm_math.h"

/**
 * @brief Print a float32_t array
 *
 * @param arr Array to print
 * @param len Length of array
 * @param name Name of array
 */
void
print_array_f32(float32_t *arr, size_t len, char *name);

/**
 * @brief Print a uint32_t array
 *
 * @param arr Array to print
 * @param len Length of array
 * @param name Name of array
 */
void
print_array_u32(uint32_t *arr, size_t len, char *name);


#ifdef __cplusplus
}
#endif

#endif // __PK_UTILS_H

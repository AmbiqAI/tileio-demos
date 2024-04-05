/**
 * @file utils.c
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Utility functions
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "arm_math.h"
#include "ns_ambiqsuite_harness.h"
#include "utils.h"

void
print_array_f32(float32_t *arr, size_t len, char *name) {
    ns_lp_printf("%s = np.array([", name);
    for (size_t i = 0; i < len; i++) {
        ns_lp_printf("%f, ", arr[i]);
    }
    ns_lp_printf("])\n");
}

void
print_array_u32(uint32_t *arr, size_t len, char *name) {
    ns_lp_printf("%s = np.array([", name);
    for (size_t i = 0; i < len; i++) {
        ns_lp_printf("%lu, ", arr[i]);
    }
    ns_lp_printf("])\n");
}

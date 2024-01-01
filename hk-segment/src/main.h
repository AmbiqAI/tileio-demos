/**
 * @file main.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief PhysioKit app
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __MAIN_H
#define __MAIN_H

#include <stdint.h>
#include "arm_math.h"

size_t
get_sensor_len();

size_t
get_segmentation_len();

size_t
get_metrics_len();

size_t
get_transmit_len();


#endif // __MAIN_H

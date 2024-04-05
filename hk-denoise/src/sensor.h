/**
 * @file sensor.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Initializes and collects sensor data from MAX86150
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __SENSOR_H
#define __SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "arm_math.h"
#include "max86150_addons.h"
#include "ns_max86150_driver.h"
#include <stdint.h>

/**
 * @brief Sensor context structure
 *
 */
typedef struct {
    max86150_context_t *maxCtx;
    max86150_config_t *maxCfg;
    uint32_t *buffer;
    bool is_live;
} sensor_context_t;

/**
 * @brief Initialize sensor frontend
 * @param ctx Sensor context
 * @return uint32_t
 */
uint32_t
sensor_init(sensor_context_t *ctx);

/**
 * @brief Enable sensor frontend
 *
 * @param ctx Sensor context
 */
void
sensor_start(sensor_context_t *ctx);

/**
 * @brief Capture sensor data from FIFO
 *
 * @param ctx Sensor context
 * @return uint32_t
 */
uint32_t
sensor_capture_data(sensor_context_t *ctx);

/**
 * @brief Disable sensor frontend
 *
 * @param ctx Sensor context
 */
void
sensor_stop(sensor_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif // __SENSOR_H

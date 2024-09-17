/**
 * @file tileio.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Handles all I/O communcation for Tileio
 * @version 0.1
 * @date 2024-08-30
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifndef __TILEIO_H
#define __TILEIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "arm_math.h"
#include "constants.h"


typedef void (*pfnSlotUpdate)(uint8_t slot, uint8_t slot_type, const uint8_t *data, uint32_t length);
typedef void (*pfnUioUpdate)(const uint8_t *data, uint32_t length);

typedef struct {
    pfnUioUpdate uio_update_cb;
    pfnSlotUpdate slot_update_cb;
} tio_context_t;

uint32_t tio_init(tio_context_t *ctx);
void tio_start(tio_context_t *ctx);
void tio_send_slot_data(uint8_t slot, uint8_t slot_type, const uint8_t *data, uint32_t length);
void tio_send_uio_state(const uint8_t *data, uint32_t length);

void
TioTask(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // __TILEIO_H

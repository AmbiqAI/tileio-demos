/**
 * @file ringbuffer.h
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Basic ring buffer implementation
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __RINGBUFFER_H
#define __RINGBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

typedef struct {
    void *buffer;
    size_t dlen;
    uint32_t size;
    uint32_t head;
    uint32_t tail;
} rb_config_t;


/**
 * @brief Ringbuffer length
 *
 * @param ctx Ringbuffer context
 * @return size_t
 */
size_t
ringbuffer_len(rb_config_t *ctx);

/**
 * @brief Ringbuffer space
 *
 * @param ctx Ringbuffer context
 * @return size_t
 */
size_t
ringbuffer_space(rb_config_t *ctx);

/**
 * @brief Push data to ringbuffer
 *
 * @param ctx Ringbuffer context
 * @param data Data to push
 * @param len Length of data
 * @return size_t
 */
size_t
ringbuffer_push(rb_config_t *ctx, void *data, size_t len);

/**
 * @brief Fill ringbuffer with data
 *
 * @param ctx Ringbuffer context
 * @param value Value to fill
 * @param len Length of data
 * @return size_t
 */
size_t ringbuffer_fill(rb_config_t *ctx, void* value, size_t len);

/**
 * @brief Pop data from ringbuffer
 *
 * @param ctx Ringbuffer context
 * @param data Buffer to store data
 * @param len Length of data
 * @return size_t
 */
size_t
ringbuffer_pop(rb_config_t *ctx, void *data, size_t len);

/**
 * @brief Read data w/o removing (peek)
 *
 * @param ctx Ringbuffer context
 * @param data Buffer to store data
 * @param len Length of data
 * @return size_t
 */
size_t
ringbuffer_peek(rb_config_t *ctx, void *data, size_t len);

/**
 * @brief Increment pointer without reading
 *
 * @param ctx Ringbuffer context
 * @param len Length of data
 * @return size_t
 */
size_t
ringbuffer_seek(rb_config_t *ctx, size_t len);

/**
 * @brief Transfer data from one ringbuffer to another
 *
 * @param src Source ringbuffer
 * @param dst Destination ringbuffer
 * @param len Length of data
 * @return size_t
 */
size_t
ringbuffer_transfer(rb_config_t *src, rb_config_t *dst, size_t len);

/**
 * @brief Flush ringbuffer
 *
 * @param ctx Ringbuffer context
 * @return size_t
 */
size_t
ringbuffer_flush(rb_config_t *ctx);

#ifdef __cplusplus
}
#endif

#endif // __RINGBUFFER_H

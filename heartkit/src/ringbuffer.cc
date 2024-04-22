
/**
 * @file ringbuffer.cc
 * @author Adam Page (adam.page@ambiq.com)
 * @brief Basic ring buffer implementation
 * @version 1.0
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "ringbuffer.h"

size_t ringbuffer_len(rb_config_t *ctx) {
    if (ctx->head >= ctx->tail) {
        return ctx->head - ctx->tail;
    } else {
        return ctx->size - ctx->tail + ctx->head;
    }
}

size_t ringbuffer_space(rb_config_t *ctx) {
    return ctx->size - ringbuffer_len(ctx);
}

size_t ringbuffer_push(rb_config_t *ctx, void *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (ringbuffer_space(ctx) == 0) {
            return i;
        }
        memcpy(((char *)ctx->buffer) + ctx->head * ctx->dlen, ((char *)data) + i * ctx->dlen, ctx->dlen);
        ctx->head = (ctx->head + 1) % ctx->size;
    }
    return len;
}

size_t ringbuffer_fill(rb_config_t *ctx, void* value, size_t len) {
    size_t space = ringbuffer_space(ctx);
    size_t amt = len > space ? space : len;
    for (size_t i = 0; i < amt; i++) {
        memcpy(((char *)ctx->buffer) + ctx->head * ctx->dlen, value, ctx->dlen);
        ctx->head = (ctx->head + 1) % ctx->size;
    }
    return amt;
}

size_t ringbuffer_pop(rb_config_t *ctx, void *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (ringbuffer_len(ctx) == 0) {
            return i;
        }
        memcpy(((char *)data) + i * ctx->dlen, ((char *)ctx->buffer) + ctx->tail * ctx->dlen, ctx->dlen);
        ctx->tail = (ctx->tail + 1) % ctx->size;
    }
    return len;
}

void
ringbuffer_replace(rb_config_t *ctx, void *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        memcpy(((char *)ctx->buffer) + ctx->tail * ctx->dlen, ((char *)data) + i * ctx->dlen, ctx->dlen);
        ctx->tail = (ctx->tail + 1) % ctx->size;
    }
}

void
ringbuffer_reset(rb_config_t *ctx) {
    ctx->head = 0;
    ctx->tail = 0;
}

size_t
ringbuffer_peek(rb_config_t *ctx, void *data, size_t len) {
    size_t tail = ctx->tail;
    for (size_t i = 0; i < len; i++) {
        if (ringbuffer_len(ctx) == 0) {
            return i;
        }
        memcpy(((char *)data) + i * ctx->dlen, ((char *)ctx->buffer) + tail * ctx->dlen, ctx->dlen);
        tail = (tail + 1) % ctx->size;
    }
    return len;
}

size_t
ringbuffer_seek(rb_config_t *ctx, size_t len) {
    size_t size = ringbuffer_len(ctx);
    size_t amt = len > size ? size : len;
    ctx->tail = (ctx->tail + amt) % ctx->size;
    return amt;
}

size_t
ringbuffer_transfer(rb_config_t *src, rb_config_t *dst, size_t len) {
    if (src->dlen != dst->dlen) {
        return 0;
    }
    size_t size = ringbuffer_len(src);
    size_t amt = len > size ? size : len;
    for (size_t i = 0; i < amt; i++) {
        if (ringbuffer_space(dst) == 0) {
            return i;
        }
        memcpy(((char *)dst->buffer) + dst->head * dst->dlen, ((char *)src->buffer) + src->tail * src->dlen, src->dlen);
        src->tail = (src->tail + 1) % src->size;
        dst->head = (dst->head + 1) % dst->size;
    }
    return amt;
}

size_t
ringbuffer_flush(rb_config_t *ctx) {
    size_t len = ringbuffer_len(ctx);
    ctx->tail = ctx->head;
    return len;
}

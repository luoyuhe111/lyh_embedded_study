#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__
#include <stdint.h>
#include <stdbool.h>

struct ringbuffer;
typedef struct ringbuffer *ringbuffer_t;

ringbuffer_t rb_new(uint8_t *buffer ,uint32_t length);
static inline uint16_t next_head(ringbuffer_t rb);
static inline uint16_t next_tail(ringbuffer_t rb);
bool rb_empty(ringbuffer_t rb);
bool rb_full(ringbuffer_t rb);
bool rb_put(ringbuffer_t rb, uint8_t data);
bool rb_get(ringbuffer_t rb, uint8_t *data);
bool rb_puts(ringbuffer_t rb, const uint8_t *data, uint32_t length);
bool rb_gets(ringbuffer_t rb, uint8_t *data, uint32_t length);

#endif /* __RINGBUFFER_H */

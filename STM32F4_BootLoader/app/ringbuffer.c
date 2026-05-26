#include <stdint.h>
#include "ringbuffer.h"
#include <string.h>
#include <stdbool.h>

struct ringbuffer
{
    uint16_t head; //环形缓冲区头索引
    uint16_t tail; //环形缓冲区尾索引
    uint16_t size; //环形缓冲区大小
    uint8_t buffer[]; //环形缓冲区数据存储,柔性数组

};

ringbuffer_t rb_new(uint8_t *buffer ,uint32_t length)
{
    if(length < sizeof(struct ringbuffer) + 1) //环形缓冲区至少需要一个字节的存储空间，所以总大小必须大于结构体大小加1
    {
        return NULL;
    }
    ringbuffer_t rb = (ringbuffer_t)buffer; //将外部提供的缓冲区地址转换为ringbuffer_t类型
    rb->head = 0; //初始化头索引
    rb->tail = 0; //初始化尾索引
    rb->size = length; //设置环形缓冲区大小
    //memset(rb->buffer, 0, length); //清空缓冲区
    return rb; //返回创建的环形缓冲区实例

}

static inline uint16_t next_head(ringbuffer_t rb) //计算写入数据后，下一个头索引指在哪个位置的函数
{
    return rb->head + 1 < rb->size ? rb->head + 1 : 0 ;
}

static inline uint16_t next_tail(ringbuffer_t rb) //计算读取数据后，下一个尾索引指在哪个位置的函数
{
    return rb->tail + 1 < rb->size ? rb->tail + 1 : 0 ;
}

bool rb_empty(ringbuffer_t rb)
{
    return rb->head == rb->tail; //当头索引和尾索引相等时，说明环形缓冲区为空
}

bool rb_full(ringbuffer_t rb)
{
    return next_head(rb) == rb->tail; //当头索引的下一个位置等于尾索引时，说明环形缓冲区已满
}

bool rb_put(ringbuffer_t rb, uint8_t data)  //向环形缓冲区写入一个字节数据的函数，返回值表示写入是否成功
{
    if(rb_full(rb)) //如果环形缓冲区已满，无法写入数据
    {
        return false;
    }
    rb->buffer[rb->head] = data; //将数据写入当前头索引位置
    rb->head = next_head(rb); //更新头索引到下一个位置
    return true; //写入成功
}

bool rb_get(ringbuffer_t rb, uint8_t *data)  //从环形缓冲区读取一个字节数据的函数，返回值表示读取是否成功
{
    if(rb_empty(rb)) //如果环形缓冲区为空，无法读取数据
    {
        return false;
    }
    *data = rb->buffer[rb->tail]; //将当前尾索引位置的数据读出到外部提供的data指针
    rb->tail = next_tail(rb); //更新尾索引到下一个位置
    return true; //读取成功
}

bool rb_puts(ringbuffer_t rb, const uint8_t *data, uint32_t length) //向环形缓冲区写入多个字节数据的函数，返回值表示写入是否成功
{
    while(length--)
    {
        if(!rb_put(rb, *data++)) //逐字节写入数据，如果写入失败（环形缓冲区已满），则返回false
        {
            return false;
        }
    }
    return true; //所有数据写入成功
}

bool rb_gets(ringbuffer_t rb, uint8_t *data, uint32_t length) //从环形缓冲区读取多个字节数据的函数，返回值表示读取是否成功
{
    while(length--)
    {
        if(!rb_get(rb, data++)) //逐字节读取数据，如果读取失败（环形缓冲区为空），则返回false
        {
            return false;
        }
    }
    return true; //所有数据读取成功
}

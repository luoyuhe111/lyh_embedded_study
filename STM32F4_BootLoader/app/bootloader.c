#include "bootloader_usart.h"
#include "stdbool.h"
#include "stdint.h"
#include <stdio.h>
#include "ringbuffer.h"

//中文解释

#define PACKET_SIZE_MAX 4096   // Maximum packet size (header + opcode(操作码) + length + payload(要传输的数据))
#define RX_BUFFER_SIZE 1024

typedef enum
{
    PACKET_STATE_HEADER,
    PACKET_STATE_OPCODE,
    PACKET_STATE_LENGTH,
    PACKET_STATE_PAYLOAD,
} packet_state_machine_t;   //数据包状态机的状态

typedef enum
{
    PACKET_OPCODE_ERASE = 0x01,
    PACKET_OPCODE_PROGRAM = 0x02,
    PACKET_OPCODE_VERIFY  = 0x03,
    PACKET_OPCODE_BOOT = 0x04,
} packet_opcode_t;      //数据包操作码

typedef enum
{
    PACKET_ERRCODE_OK = 0x00,
    PACKET_ERRCODE_OPCODE,
    PACKET_ERRCODE_OVERFLOW,
    PACKET_ERRCODE_TIMEOUT,
    PACKET_ERRCODE_FORMAT,
    PACKET_ERRCODE_VERIFY,
    PACKET_ERRCODE_PARAM,
    PACKET_ERRCODE_UNKNOWN = 0xff,
} packet_errcode_t;  //数据包错误码

static uint8_t rb_buffer[RX_BUFFER_SIZE]; //环形缓冲区底层存储空间
static ringbuffer_t rx_ringbuffer; //串口接收环形缓冲区实例

static packet_state_machine_t packet_state = PACKET_STATE_HEADER;  //当前数据包状态
static uint8_t packet_buffer[PACKET_SIZE_MAX]; //数据包缓冲区
static uint32_t packet_index = 0; //数据包当前索引
static packet_opcode_t packet_opcode; //当前数据包操作码
static uint16_t packet_payload_length; //当前数据包载荷长度


static void bootloader_byte_handler(uint8_t byte)   //串口接收字节处理函数，每当接收到一个字节时被调用
{
    printf("received byte: 0x%02X\n", byte); //打印接收到的字节，格式为十六进制
    packet_buffer[packet_index++] = byte; //将接收到的数据存入数据包缓冲区
    switch(packet_state)
    {
        case PACKET_STATE_HEADER:
            if(packet_buffer[0] == 0xAA) //假设0xAA为数据包的起始标志
            {
                //printf("packet header ok\n");
                packet_state = PACKET_STATE_OPCODE;
            }
            else
            {
                packet_index = 0; //如果起始标志不正确，重置索引
                packet_state = PACKET_STATE_HEADER; //保持在等待数据包头的状态
            }
            break;
        case PACKET_STATE_OPCODE:
            if(packet_buffer[1] == PACKET_OPCODE_ERASE ||
               packet_buffer[1] == PACKET_OPCODE_PROGRAM ||
               packet_buffer[1] == PACKET_OPCODE_VERIFY ||
               packet_buffer[1] == PACKET_OPCODE_BOOT)  //检查操作码是否有效
            {
                printf("packet opcode ok\n");
                packet_opcode = (packet_opcode_t)packet_buffer[1]; //保存当前操作码
                packet_state = PACKET_STATE_LENGTH;  //如果操作码正确，进入等待数据包长度的状态
            }
            else
            {
                packet_index = 0; //如果操作码不正确，重置索引
                packet_state = PACKET_STATE_HEADER; //回到等待数据包头的状态
            }
            break;
        case PACKET_STATE_LENGTH:
            if(packet_index == 4) //包头1+操作码1+长度2=4字节
            {
                uint8_t payload_length = (packet_buffer[3] << 8) | packet_buffer[2]; //把两个字节组合成payload长度，且为小端格式
                if(payload_length <= (PACKET_SIZE_MAX -4)) //检查长度是否超过缓冲区剩余空间，因为前 4 个字节已经被包头、操作码、长度占用了，所以 payload 最大只能是4096-4
                {
                    packet_payload_length = payload_length; //保存当前数据包载荷长度
                    packet_state = PACKET_STATE_PAYLOAD; //如果长度合法，进入等待数据包载荷的状态
                    printf("packet length ok: %u\n", packet_payload_length);
                }
                else
                {
                    packet_index = 0; //如果长度不合法，重置索引
                    packet_state = PACKET_STATE_HEADER; //回到等待数据包头的状态
                }
            }
            break;
        case PACKET_STATE_PAYLOAD:
            if(packet_index == 4 + packet_payload_length) //当接收到的数据长度等于头部长度加载荷长度时，说明一个完整的数据包已经接收完毕
            {
                printf("packet payload ok");

                printf("packet received: opcode=0x%02X, length=%u\n", packet_opcode, packet_payload_length);
                printf("packet payload: ");
                for(uint32_t i=0; i<packet_payload_length; i++)
                {
                    printf("%02X ", packet_buffer[4+i]); //打印数据包载荷内容，格式为十六进制
                }
                printf("\n");


                packet_index = 0; //重置索引
                packet_state = PACKET_STATE_HEADER; //回到等待数据包头的状态



            }
            break;
        default:
            packet_state = PACKET_STATE_HEADER; // Reset state machine on error
            break;
    }

}

static void bootloader_usart_rx_handler(const uint8_t *data,uint32_t length)  //串口接收回调函数，每当串口接收到数据时被调用，参数data指向接收到的数据，length表示数据长度
{
    rb_puts(rx_ringbuffer, data, length); //将接收到的数据写入串口接收环形缓冲区
}

void bootloader_main()
{
    printf("bootloader main start\n");

    rx_ringbuffer = rb_new(rb_buffer, RX_BUFFER_SIZE); //创建串口接收环形缓冲区实例
    bootloader_usart_init(); //初始化串口
    bootloader_usart_register_rx_callback(bootloader_usart_rx_handler); //注册串口接收回调函数

    while(1)
    {
        if(!rb_empty(rx_ringbuffer)) //如果串口接收环形缓冲区不为空，说明有数据可读
        {
            uint8_t byte;
            rb_get(rx_ringbuffer, &byte);//从串口接收环形缓冲区读取一个字节数据
            bootloader_byte_handler(byte); //调用串口接收字节处理函数处理这个字节
        }

    }
}

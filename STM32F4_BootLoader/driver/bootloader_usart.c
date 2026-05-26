#include "stm32f4xx_conf.h"
#include "bootloader_usart.h"
#include "string.h"
#include "misc.h"

//USE USART3 TX: PD8  RX:PD9
//Baudrate:115200
//MODE:8-N-1
//DMA:TX/RX
//IT:RX/DMA_TX_TC/DMA_RX_TC

static bootloader_usart_rx_callback_t rx_callback = NULL;  //接收回调函数指针

static void usart_io_init(void)   //串口引脚输入输出配置
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_USART3);
}

// static void usart_dma_config(void)    //串口DMA配置
// {
//     DMA_InitTypeDef DMA_InitStructure;
//     DMA_StructInit(&DMA_InitStructure);

//     //RX
//     DMA_InitStructure.DMA_Channel = DMA_Channel_4;
//     DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART3->DR;
//     DMA_InitStructure.DMA_Memory0BaseAddr = 0; //TODO
//     DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
//     DMA_InitStructure.DMA_BufferSize = 0; //TODO
//     DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//     DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//     DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//     DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//     DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
//     DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
//     DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
//     DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
//     DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_INC16;
//     DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
//     DMA_Init(DMA1_Stream1, &DMA_InitStructure);
//     //DMA_ITConfig(DMA1_Stream1, DMA_IT_TC, ENABLE);
//     DMA_Cmd(DMA1_Stream1, ENABLE);

//     //TX
//     DMA_InitStructure.DMA_Channel = DMA_Channel_4;
//     DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART3->DR);
//     DMA_InitStructure.DMA_Memory0BaseAddr = 0; //TODO
//     DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
//     DMA_InitStructure.DMA_BufferSize = 0; //TODO
//     DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//     DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//     DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//     DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//     DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
//     DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
//     DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
//     DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
//     DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_INC16;
//     DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
//     //DMA_Init(DMA1_Stream3, &DMA_InitStructure);
//     DMA_ITConfig(DMA1_Stream3, DMA_IT_TC, ENABLE);
//     //USART_DMACmd(DMA1_Stream3, ENABLE);
// }

static void usart_it_config(void)    //串口中断配置
{
    NVIC_InitTypeDef NVIC_InitStructure;
    //NVIC_StructInit(&NVIC_InitStructure);

    //RX
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_SetPriority(USART3_IRQn, 5);

    // //DMA TX
    // NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream3_IRQn;
    // NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    // NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    // NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    // NVIC_Init(&NVIC_InitStructure);
    // NVIC_SetPriority(DMA1_Stream3_IRQn,5);

    // //DMA RX
    // NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream1_IRQn;
    // NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    // NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    // NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    // NVIC_Init(&NVIC_InitStructure);
    // NVIC_SetPriority(DMA1_Stream1_IRQn,5);

}

static void usart_lowlevel_init(void)      //串口底层配置
{
    USART_InitTypeDef USART_InitStructure;
    USART_StructInit(&USART_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART3, &USART_InitStructure);
    USART_Cmd(USART3, ENABLE);
    // USART_DMACmd(USART3,USART_DMAReq_Tx,ENABLE);
    // USART_DMACmd(USART3,USART_DMAReq_Rx,ENABLE);
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART3, ENABLE);
}


void bootloader_usart_init(void)  //串口初始化
{
    usart_io_init();
    // usart_dma_config();
    usart_it_config();
    usart_lowlevel_init();
}

void bootloader_usart_write(uint8_t* data, uint32_t size)   //串口写数据
{
    while (size--)
    {
        while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
        USART_SendData(USART3, *data++);
    }


    //DMA Tranfer 65536 bytes at most
    // while(size >0)
    // {
    //     uint32_t transfer_size = size > 65536 ? 65536 : size;
    //     DMA1_Stream3->M0AR = (uint32_t)data; //将数据地址写入DMA内存地址寄存器
    //     DMA1_Stream3->NDTR = transfer_size;  //将数据长度写入DMA数据传输计数寄存器
    //     DMA_Cmd(DMA1_Stream3, ENABLE);
    //     while(DMA_GetCmdStatus(DMA1_Stream3) != DISABLE); //等待DMA传输完成
    //     size -= transfer_size;  //更新剩余数据长度
    //     data += transfer_size;  //更新数据指针
    // }

}

void bootloader_usart_read(uint8_t* buffer, uint32_t size)    //串口读数据
{
    //TODO
    // Implementation for reading data via USART using DMA should be added here.
    // This may involve setting up the DMA for reception and handling the received data in the USART
}

void bootloader_usart_register_rx_callback(bootloader_usart_rx_callback_t callback)   //注册串口接收回调函数
{
    rx_callback = callback;
}


void USART3_IRQHandler(void)     //USART3中断服务函数
{
    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        if(rx_callback) //如果接收回调函数指针不为空   这里执行的回调函数bootloader_rx_handler
        {
            uint8_t data = USART_ReceiveData(USART3) & 0xFF; //读取接收到的数据
            rx_callback(&data, 1); //调用回调函数处理接收到的数据
        }
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}

//DMA1 Stream3 for USART3 TX
void DMA1_Stream3_IRQHandler(void)    //DMA1 Stream3 USART3 TX中断服务函数
{
    if(DMA_GetITStatus(DMA1_Stream3, DMA_IT_TCIF3) != RESET)
    {
        //TODO
        DMA_ClearITPendingBit(DMA1_Stream3, DMA_IT_TCIF3);
    }
}

//DMA1 Stream1 for USART3 RX
void DMA1_Stream1_IRQHandler(void)  //DMA1 Stream1 USART3 RX中断服务函数
{
    if(DMA_GetITStatus(DMA1_Stream1, DMA_IT_TCIF1) != RESET)
    {
        //TODO
        DMA_ClearITPendingBit(DMA1_Stream1, DMA_IT_TCIF1);
    }
}

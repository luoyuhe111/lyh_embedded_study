#include <stdint.h>
#include <string.h>
#include "stm32f4xx_conf.h"
#include "console.h"

//PA9TX,PA10RX usart1
static console_received_func_t received_func;   

static void console_io_init(void)
{
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void console_usart_init(void)
{
	USART_InitTypeDef USART_InitStructure;
    USART_StructInit(&USART_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200u;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	
	USART_Init(USART1, &USART_InitStructure);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);
    USART_Cmd(USART1, ENABLE);
}

static void console_dma_init(void)
{
	DMA_InitTypeDef DMA_InitStructure;
	DMA_StructInit(&DMA_InitStructure);
	DMA_InitStructure.DMA_Channel = DMA_Channel_4;  //// 通道，根据芯片手册里的映射表
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR; //外设数据寄存器地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral; //传输方向：内存->外设
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; //不自增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; 
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;     // 
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; 
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_INC8;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA1_Stream7,&DMA_InitStructure);
	DMA_Cmd(DMA1_Stream7,ENABLE);
}

static void console_interrupt_init(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void console_init(void)
{
	console_usart_init();
	console_dma_init();
	console_interrupt_init();
	console_io_init();
}

void console_write(const char str[])
{
	uint32_t len =strlen(str);
	do
	{
		uint32_t chunk_size = len < 65535 ? len :65535;
		DMA_InitTypeDef DMA_InitStructure;
		DMA_StructInit(&DMA_InitStructure);
		
//		DMA_Cmd(DMA2_Stream7, DISABLE);
//      while (DMA2_Stream7->CR & DMA_SxCR_EN);  
		
		//DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)data;  //接收缓冲区数组首地址
		DMA2_Stream7->M0AR = (uint32_t)str;
		//DMA_InitStructure.DMA_BufferSize = chunk_size ;    //缓冲区元素个数
		DMA2_Stream7->NDTR  = chunk_size;
		
		DMA_Cmd(DMA2_Stream7,ENABLE);
		while (DMA_GetFlagStatus(DMA2_Stream7,DMA_FLAG_TCIF7) == RESET); //等DMA把数据搬完
		DMA_ClearFlag(DMA2_Stream7,DMA_FLAG_TCIF7);
		
		str +=chunk_size * 2 ;
		len -=chunk_size;
		
	}while(len > 0);
	
	while (DMA_GetFlagStatus(DMA2_Stream7,DMA_FLAG_TCIF7) == RESET); //等DMA把数据搬完
	USART_ClearFlag(USART1,USART_FLAG_TC);
}

void console_received_register(console_received_func_t func)
{
    received_func = func;
}

void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //判断是否有新的数据
    {
        if (received_func != NULL)
        {
            uint8_t data = USART_ReceiveData(USART1);   //读取数据寄存器，拿到这个传进来的字节
            received_func(data);  
        }
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

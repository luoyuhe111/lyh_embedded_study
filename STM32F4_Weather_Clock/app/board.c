#include "stm32f4xx_conf.h"
#include <stdint.h>  //uint8_t
#include <stdio.h>  //FILE
#include "console.h"
#include "esp_at.h"
#include "st7789.h"
#include "aht20.h"
#include "cpu_tick.h"
#include "FreeRTOS.h"
#include "task.h"


void board_lowlevel_init(void)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2,ENABLE);
    PWR_BackupAccessCmd(ENABLE);
    RCC_LSEConfig(RCC_LSE_ON);
    while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
}

void board_init(void)
{
	
	st7789_init();
	console_init();
	aht20_init();
	esp_at_init();
	printf("[SYS] Build Date: %s %s\n", __DATE__, __TIME__);  // 
	
	
	
}



// 重定向 printf 到 USART1
int fputc(int ch, FILE *f)
{
    // 等待发送寄存器为空
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);

    // 发送一个字节
    USART_SendData(USART1, (uint8_t)ch);

    // 等待发送完成
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);

    return ch;
}

void vAssertCalled( const char *file, int line)
{
	printf("Assert Called: %s(%d)\n",file,line);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	printf("Stack Overflowed: %s\n",pcTaskName);
	configASSERT(0); //主动触发一个 FreeRTOS 的致命错误，让系统进入错误调试模式。
}

void vApplicationMallocFailedHook(void)
{
	printf("Malloc Failed\n");
	configASSERT(0);
}
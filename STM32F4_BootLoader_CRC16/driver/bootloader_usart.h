#ifndef __BL_USART_H__
#define __BL_USART_H__

#include <stdint.h>

typedef void (*bootloader_usart_rx_callback_t)(const uint8_t *data,uint32_t size); //

void bootloader_usart_init(void);
void bootloader_usart_write(uint8_t* data, uint32_t size);
void bootloader_usart_register_rx_callback(bootloader_usart_rx_callback_t callback);

#endif

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <stdint.h>

typedef void (*console_received_func_t)(uint8_t data);   //console_received_func_t 是一种类型，
														//表示“指向一个参数为 uint8_t、返回 void 的函数的指针”。

void console_init(void);
void console_received_register(console_received_func_t func);
void console_write(const char str[]);

#endif /* __CONSOLE_H__ */

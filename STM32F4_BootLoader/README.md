# STM32F4_BootLoader

STM32F4 BootLoader 学习项目。

## 本次记录

- 添加串口接收 ringbuffer，用于把 USART3 中断接收和主循环协议解析解耦。
- 修正 ringbuffer 批量读写时 length 递减导致的异常写入问题。
- 保留 Keil MDK 工程文件，构建输出目录不纳入版本记录。

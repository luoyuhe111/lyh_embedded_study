# STM32F4 BootLoader CRC16

这是 STM32F407VETx BootLoader 学习项目的 CRC16 增强版。在上一版串口接收 ringbuffer 的基础上，本版本增加了数据包 CRC16 校验和接收超时审查机制，用于提升串口升级协议的完整性检查与异常恢复能力。

## 本次记录

- 保留 ringbuffer 接收结构，将 USART3 中断接收与主循环协议解析解耦。
- 新增 CRC16/XMODEM 校验，数据包末尾携带 2 字节小端 CRC16。
- 新增字节间超时审查，接收过程中超过 `RX_TIMEOUT_MS` 会重置状态机，避免半包卡死。
- 使用状态机解析 `Header -> Opcode -> Length -> Payload -> CRC16`。
- 保留 Keil MDK 工程文件，构建输出目录不纳入版本记录。

## 协议格式

```text
+--------+--------+-------------+---------+-------------+
| Header | Opcode | Length (LE) | Payload | CRC16 (LE)  |
+--------+--------+-------------+---------+-------------+
| 0xAA   | 1 byte | 2 bytes     | N bytes | 2 bytes     |
+--------+--------+-------------+---------+-------------+
```

CRC16 计算范围为 `Header + Opcode + Length + Payload`，不包含末尾的 CRC16 字段本身。

## 操作码

| Opcode | 含义 |
| --- | --- |
| `0x01` | 擦除 |
| `0x02` | 编程 |
| `0x03` | 校验 |
| `0x04` | 启动应用 |

## 目录结构

```text
.
├── app/       BootLoader 主逻辑、ringbuffer、CRC、延时与跳转代码
├── driver/    USART3 BootLoader 通信与调试串口驱动
├── firmware/  CMSIS 与 STM32F4 标准外设库
├── mdk/       Keil MDK-ARM 工程文件
└── third_lib/ 第三方库预留目录
```

## 快速开始

1. 安装 Keil MDK-ARM，并准备 STM32F4 Device Pack。
2. 打开 `mdk/stm32f407.uvprojx`。
3. 编译并下载 BootLoader 到 STM32F407VETx。
4. 使用 USART3 作为 BootLoader 协议通信口，默认 `115200 8N1`。
5. 发送带 CRC16 的协议包，观察串口日志中的状态机、CRC 和超时审查输出。

## 关键文件

- `app/bootloader.c`: 协议状态机、ringbuffer 消费、CRC16 校验和超时审查。
- `app/ringbuffer.c`: 环形缓冲区实现。
- `app/crc16.c`: CRC16/XMODEM 查表实现。
- `app/tim_delay.c`: 毫秒时间戳，用于接收超时判断。
- `driver/bootloader_usart.c`: USART3 初始化、中断接收和发送接口。

## 注意事项

- `PACKET_SIZE_MAX` 当前为 4096 字节，接收缓冲区 `RX_BUFFER_SIZE` 当前为 1024 字节。
- `RX_TIMEOUT_MS` 当前为 20 ms，可根据上位机发送间隔和波特率调整。
- 当前工程重点记录协议接收、CRC 校验与异常恢复流程，擦除/编程/启动应用等动作可继续在操作码处理分支中完善。

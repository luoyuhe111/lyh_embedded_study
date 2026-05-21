# STM32F4 Weather Clock

基于 STM32F407VETx 的智能天气时钟项目。工程使用 Keil MDK-ARM 管理，运行 FreeRTOS，通过 ESP-AT WiFi 模块联网，同步网络时间并从心知天气接口获取室外天气，同时读取 AHT20 室内温湿度，在 ST7789 彩屏上显示时间、日期、WiFi 状态、室内温湿度和天气图标。

## 功能特性

- FreeRTOS 多任务和软件定时器调度
- ESP-AT WiFi 连接、SNTP 网络校时、HTTP GET 获取天气数据
- AHT20 室内温湿度采集
- STM32 RTC 维护本地时间
- ST7789 SPI 彩屏显示主页面、欢迎页、WiFi 页和错误页
- USART1 printf 调试输出
- 内置字体和天气图标资源

## 硬件与外设

- 主控: STM32F407VETx
- 屏幕: ST7789 SPI TFT
- 温湿度传感器: AHT20, I2C
- 联网模块: 支持 ESP-AT 固件的 WiFi 模块
- 调试串口: USART1, 115200 baud
- 外部低速晶振: LSE, 用于 RTC

## 目录结构

```text
.
├── app/                 应用层逻辑、页面、字体和图片资源
├── driver/              板级外设驱动, 包含 ESP-AT、ST7789、AHT20、RTC、串口等
├── firmware/            CMSIS 与 STM32F4 标准外设库
├── third_lib/freertos/  FreeRTOS 内核与移植层
└── mdk/                 Keil MDK 工程文件
```

## 快速开始

1. 安装 Keil MDK-ARM，并准备 STM32F4 相关 Device Pack。
2. 打开 `mdk/stm32f407.uvprojx`。
3. 在 `app/app.h` 中配置:
   - `WIFI_SSID`
   - `WIFI_PASSWORD`
   - `WEATHER_URL` 中的心知天气 key 与 location
4. 编译工程，使用 ST-LINK 或兼容下载器烧录到 STM32F407VETx。
5. 打开串口助手，设置 USART1 为 `115200 8N1`，查看启动、WiFi、SNTP 和天气请求日志。

## 配置说明

公开仓库中的网络配置均已替换为占位符，避免泄露 WiFi 密码和天气 API key。实际使用时请在本地修改 `app/app.h`:

```c
#define WEATHER_URL "https://api.seniverse.com/v3/weather/now.json?key=YOUR_SENIVERSE_KEY&location=YOUR_LOCATION&language=en&unit=c"
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
```

`location` 可以使用城市拼音、城市名称，或心知天气提供的位置 ID。建议不要把真实密码或 API key 提交到公开仓库。

## 主要流程

系统启动后先完成底层时钟、GPIO、串口、屏幕、AHT20 和 ESP-AT 初始化，然后创建 FreeRTOS 主初始化任务。初始化任务依次显示欢迎页、初始化 WiFi、等待联网、进入主页面，并启动主循环定时更新:

- 每 1 秒刷新 RTC 时间显示
- 每 3 秒读取室内温湿度
- 每 5 秒检查 WiFi 连接状态
- 每 1 小时同步一次网络时间
- 每 1 分钟请求一次室外天气

## 注意事项

- `mdk/Objects/` 和 `mdk/Listings/` 为 Keil 构建产物，已通过 `.gitignore` 排除。
- `*.uvguix.*` 是 Keil 用户界面状态文件，不影响工程编译，已排除。
- 如果 ESP-AT 初始化或联网失败，程序会进入错误页面并停留，便于现场排查。
- 如果天气接口返回格式变化，需同步检查 `app/weather.c` 中的解析逻辑。

## 许可

该仓库用于嵌入式学习与课程/项目记录。第三方库如 FreeRTOS、CMSIS、STM32 标准外设库遵循其各自许可。

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "freertos.h"
#include "task.h"
#include "stm32f4xx_conf.h"
#include "esp_at.h"
#include "cpu_tick.h"
#include "time.h"
#include "FreeRTOS.h"
#include "task.h"

//PA2TX,PA3RX  usart2

#define ESP_AT_DEBUG    1
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef enum  //给这个枚举类型起一个名字叫at_ack_t
{
    AT_ACK_NONE, //0
    AT_ACK_OK,   //1
    AT_ACK_ERROR,//2
    AT_ACK_BUSY, //3
    AT_ACK_READY,//4
} at_ack_t;

typedef struct //定义一个结构体起名叫at_ack_match_t,里面有两个成员
{
    at_ack_t ack;  //第一个成员是枚举类型的,起名ack
    const char *string;  //第二个成员是字符串指针类型，叫string
} at_ack_match_t;

static const at_ack_match_t at_ack_matches[] =      //定义一个结构体数组，类型为at_ack_match_t 
{													//这个at_ack_matches[]就是一个“应答匹配项”数组，现在数组里有4个成员
    {AT_ACK_OK, "OK\r\n"},							//每个成员都是一个”应答匹配项“，ack对应的字符串是string
    {AT_ACK_ERROR, "ERROR\r\n"},
    {AT_ACK_BUSY, "busy p…\r\n"},
    {AT_ACK_READY, "ready\r\n"},
};

static char rxbuf[1024];

static void esp_at_usart_write(const char *data);
static bool esp_at_wait_boot(uint32_t timeout);

static void esp_at_usart_init(void)  //串口初始化
{
    USART_InitTypeDef USART_InitStructure;
    USART_StructInit(&USART_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_Init(USART2, &USART_InitStructure);
    USART_Cmd(USART2, ENABLE);
}

bool esp_at_init(void) //把esp32c3复位
{
    esp_at_usart_init();
    
    if (!esp_at_wait_boot(3000))
        return false;
    if (!esp_at_write_command("AT+RESTORE", 2000))
        return false;
    if (!esp_at_wait_ready(5000))
        return false;
    
    return true;
}

static void esp_at_usart_write(const char *data)
{
    while (data && *data) //等价于 while ( (data != NULL) && (*data != '\0') )  如果data指针不为NULL且*data取过来的不是'\0'，则一直循环
    {						//也就是说前一个防止传入空指针使得程序崩溃，后一个如果传过来的是'\0',说明字符串已经发完了，可以跳出循环了
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, *data++);
    }
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    USART_SendData(USART2, '\r');
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    USART_SendData(USART2, '\n');
}

//把一行字符串与预定义应答对比，返回对应枚举
static at_ack_t match_internal_ack(const char *str) //这里要传一串字符串指针
{
    for (uint32_t i = 0; i < ARRAY_SIZE(at_ack_matches); i++) //循环检查每一项传进来的字符串str是否跟每个应答匹配项里的string完全一样
    {
        if (strcmp(str, at_ack_matches[i].string) == 0)   //如果完全一样
            return at_ack_matches[i].ack;    //返回每个应答匹配项对应的ack(例如 AT_ACK_OK)
    }
    
    return AT_ACK_NONE;   
}

//函数作用：等待模组回应，返回一个at_ack_t里的应答状态
static at_ack_t esp_at_usart_wait_receive(uint32_t timeout)
{
    uint32_t rxlen = 0; //定义一个变量，记录当前接收到的字节数
    const char *line = rxbuf; //定义一个指针line指向rxbuf接收缓冲区的首地址
    uint64_t start = xTaskGetTickCount(); //记录当前时间
    
    rxbuf[0] = '\0'; //先清空缓冲区准备接收
    while (rxlen < sizeof(rxbuf) - 1) //确保不会溢出，预留一个字节给\0结束符
    {
        while (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == RESET) //串口每接收到字节就跳出循环
        {
            if (xTaskGetTickCount() - start >= timeout) //等字节进来的同时也在检查是否超时,如果当前时间 - 开始时间 ≥ 设定超时值，
                return AT_ACK_NONE;              //就放弃等待，返回 NONE
        }
        rxbuf[rxlen++] = USART_ReceiveData(USART2);  //有数据了，读取一个字节，存入缓冲区rxbuf,每存入一次，rxlen都自增加1
        rxbuf[rxlen] = '\0';     //c语言字符串始终以'\0'结尾
        if (rxbuf[rxlen - 1] == '\n') //读到换行符，说明模组发完了一行数据，例如"OK\r\n"
        {
            at_ack_t ack = match_internal_ack(line); //定义一个应答变量ack(跟上面结构体里的ack没有半毛钱关系),用来保存match_internal_ack这个函数返回的应答类型
													 //在 C 语言里，字符串其实就是一个 char *（指针），所以line这里就代表着一串字符串数据比如”OK\r\n“
            if (ack != AT_ACK_NONE)  //如果匹配到了有效应答
                return ack;          //返回这个对应的ack
            line = rxbuf + rxlen;    //如果没匹配到，移到新行的开始位置，准备接收下一行
        }
    }
    
    return AT_ACK_NONE;  //循环遍历完都没有匹配到
}

bool esp_at_wait_ready(uint32_t timeout)  //等待 ESP 模组返回 "ready\r\n"，并在设定时间内判断是否等到了。
{
    return esp_at_usart_wait_receive(timeout) == AT_ACK_READY; //拿返回值作比较，如果相等表示等到了”ready“,反则就是没等到
}

//函数作用：发送相关的AT命令并且等待OK
bool esp_at_write_command(const char *command, uint32_t timeout) //command要发送的命令字符串比如"AT+GMR"等等
{
#if ESP_AT_DEBUG                           //调试用的
    printf("[DEBUG] Send: %s\n", command);
#endif

    esp_at_usart_write(command);        //串口发送命令到esp32去
    at_ack_t ack = esp_at_usart_wait_receive(timeout); //返回一个响应结果AT_ACK_NONE或者AT_ACK_OK,

#if ESP_AT_DEBUG                                 //调试用的
    printf("[DEBUG] Response:\n%s\n", rxbuf);
#endif

    return ack == AT_ACK_OK;     //匹配是否返回状态是"OK"，成功返回true,否则false
}

const char *esp_at_get_response(void) 
{
    return rxbuf;
}

static bool esp_at_wait_boot(uint32_t timeout)
{
	for(int t = 0; t<timeout; t +=100)
	{
		if(esp_at_write_command("AT",100))
			return true;
	}
	
	return false;
}


//wifi初始化，设置wifi模式为模式1
bool esp_at_wifi_init(void)
{
	return esp_at_write_command("AT+CWMODE=1",2000) ;
	
}

//连接wifi
bool esp_at_connect_wifi(const char *ssid, const char *pwd, const char *mac)
{
	if(ssid == NULL || pwd == NULL)  //防止WiFi名称和WiFi密码为空
		return false;
	
	char cmd[128]; //在栈上开辟一个缓冲区
	int len = snprintf(cmd , sizeof(cmd),"AT+CWJAP=\"%s\",\"%s\"",ssid,pwd); //拼接cmd字符串并且返回长度给len
	
	if(mac)
	{
		snprintf(cmd+len, sizeof(cmd)-len, ",\"%s\"",mac);
	}
	
	return esp_at_write_command(cmd,5000);  //发送构造好的 AT+CWJAP 命令
}

//cwstate解析
static bool parse_cwstate_response(const char* response, esp_wifi_info_t* info)
{
    response = strstr(response, "+CWSTATE:");
    if (response == NULL)
        return false;
    
    int wifi_state;

    if (sscanf(response, "+CWSTATE:%d,\"%63[^\"]\"", &wifi_state, info->ssid) != 2)
        return false;

    info->connected = (wifi_state == 2);

    return true;
}

//cwjap解析
static bool parse_cwjap_response(const char* response, esp_wifi_info_t* info)
{
    response = strstr(response, "+CWJAP:");
    if (response == NULL)
        return false;

    if (sscanf(response, "+CWJAP:\"%63[^\"]\",\"%17[^\"]\",%d,%d",info->ssid,info->bssid, &info->channel, &info->rssi) != 4)
        return false;

    return true;
}

//查询当前 Wi-Fi 连接状态
bool esp_at_get_wifi_info(esp_wifi_info_t *info)
{
	if(!esp_at_write_command("AT+CWSTATE?",2000))  //查询 Wi-Fi模块当前的连接状态
		return false;
	
	if(!parse_cwstate_response(esp_at_get_response(),info))    
		return false;
	
	if(info->connected == true)
	{
		if(!esp_at_write_command("AT+CWJAP?",2000))   //查询 Wi-Fi 模块当前连接的网络的详细信息
			return false;
		
		if(!parse_cwjap_response(esp_at_get_response(),info))    
			return false;
	}
	return true;
}

bool wifi_is_connected(void)
{
	esp_wifi_info_t info;
	if(esp_at_get_wifi_info(&info))
	{
		return info.connected;
	}
	return false;
}

//sntp(简单网络时间协议)初始化
bool esp_at_sntp_init(void)
{
	if(!esp_at_write_command("AT+CIPSNTPCFG=1,8",2000))
		return false;
	
	return true;
}

// 将字符串月份转为数字
uint8_t month_str_to_num(const char* month_str)
{
    static const char* months[] = 
    {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    for (uint8_t i = 0; i < 12; i++)
    { 
        if (strcmp(month_str, months[i]) == 0)
            return i + 1;
    }
    return 0;
}

// 将字符串星期转为数字1-7
uint8_t weekday_str_to_num(const char* weekday_str)
{
    static const char* weekdays[] = 
    {
        "Mon","Tue","Wed","Thu","Fri","Sat","Sun"
    };
    for (uint8_t i = 0; i < 7; i++)
    { 
        if (strcmp(weekday_str, weekdays[i]) == 0)
            return i;
    }
    return 0;
}

static bool parse_cipsntptime_response(const char *response,esp_date_time_t *date)
{
    
    char weekday_str[8];
    char month_str[4];
    response = strstr(response, "+CIPSNTPTIME:");
    if (sscanf(response, "+CIPSNTPTIME:%3s %3s %hhu %hhu:%hhu:%hhu %hu",weekday_str,month_str,&date->day,&date->hour,
		&date->minute,&date->second,&date->year) !=7)
        return false;

    date->weekday = weekday_str_to_num(weekday_str);
    date->month = month_str_to_num(month_str);

    return true;
} 

bool esp_at_sntp_get_time(esp_date_time_t *date)
{
	if(!esp_at_write_command("AT+CIPSNTPTIME?",2000))
		return false;
	
	if(!parse_cipsntptime_response(esp_at_get_response(),date))
		return false;
	
	return true;
}

const char *esp_at_http_get(const char *url)
{
	char *txbuf = rxbuf ;
	snprintf(txbuf,sizeof(rxbuf),"AT+HTTPCLIENT=2,1,\"%s\",,,2",url);
	bool result = esp_at_write_command(txbuf,5000);
	return (result ? esp_at_get_response() : NULL);
}



#include <string.h>
#include "stm32f4xx_conf.h"
#include "rtc.h"
#include "cpu_tick.h"

void rtc_init(void)
{
    RTC_InitTypeDef RTC_InitStruct;
    RTC_StructInit(&RTC_InitStruct);
    RTC_Init(&RTC_InitStruct);
    
    RCC_RTCCLKCmd(ENABLE);   //打开RTC时钟
    RTC_WaitForSynchro();    //等待硬件同步完成
}

static void _rtc_set_time_once(const rtc_date_time_t *date_time)   //设置一次时间 (把自己创的时间结构体->写入RTC硬件寄存器)
{
    RTC_DateTypeDef date;  //创建rtc结构体 日期(年月日周几)
    RTC_TimeTypeDef time;  //创建rtc结构体  时间(时分秒)
    
    RTC_DateStructInit(&date); //初始化默认值
    RTC_TimeStructInit(&time);
    
    date.RTC_Year = date_time->year - 2000;    //把时间翻译为硬件格式
    date.RTC_Month = date_time->month;
    date.RTC_Date = date_time->day;
    date.RTC_WeekDay = date_time->weekday;
    time.RTC_Hours = date_time->hour;
    time.RTC_Minutes = date_time->minute;
    time.RTC_Seconds = date_time->second;
    
    RTC_SetDate(RTC_Format_BIN, &date);   //写入RTC
    RTC_SetTime(RTC_Format_BIN, &time);
    
}

static void _rtc_get_time_once(rtc_date_time_t *date_time) //读取一次时间（从RTC硬件读取)
{
    RTC_DateTypeDef date;
    RTC_TimeTypeDef time;
    
    RTC_DateStructInit(&date);
    RTC_TimeStructInit(&time);
    
    RTC_GetDate(RTC_Format_BIN, &date);    //从硬件中读取
    RTC_GetTime(RTC_Format_BIN, &time);
    
    date_time->year = 2000 + date.RTC_Year;   //存到你自己创建的时间结构体里面
    date_time->month = date.RTC_Month;
    date_time->day = date.RTC_Date;
    date_time->weekday = date.RTC_WeekDay;
    date_time->hour = time.RTC_Hours;
    date_time->minute = time.RTC_Minutes;
    date_time->second = time.RTC_Seconds;
}


//这两段代码主要是为了保证读写不会被秒的跳变给破环   比如2025/1/1   23:59:59  ------>   2025/1/2   00:00:00

void rtc_set_time(const rtc_date_time_t *date_time)   //验证是否写成功
{
    rtc_date_time_t rtime;
    do {
        _rtc_set_time_once(date_time); //写入一次时间
        _rtc_get_time_once(&rtime);    //立刻读出一次时间
    } while (date_time->second != rtime.second);    //比较写入的秒和读出的秒是否一致
}

void rtc_get_time(rtc_date_time_t *date_time)   //验证是否读的稳定
{
    rtc_date_time_t time1, time2;
    do {
        _rtc_get_time_once(&time1);  //读一次时间
        _rtc_get_time_once(&time2);  //再读一次时间
    } while (memcmp(&time1, &time2, sizeof(rtc_date_time_t)) != 0);   //如果两次读到的时间一样，就跳出循环
    
    memcpy(date_time, &time1, sizeof(rtc_date_time_t));   //把time1作为最终结果返回
}

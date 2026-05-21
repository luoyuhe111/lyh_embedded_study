#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "cpu_tick.h"
#include "page.h"
#include "app.h"
#include "st7789.h"
#include "FreeRTOS.h"
#include "task.h"

extern void board_lowlevel_init(void);
extern void board_init(void);

static void main_init(void *param)    //即使里面没有参数，也必须要这么写，freertos默认的要求
{
	board_init();          //开发板设备初始化
	
	welcome_page_display();
	
	wifi_init();
	wifi_page_display();
	wifi_wait_connect();
	
	main_loop_init();
	main_page_display();
	
	while(1)
	{
		main_loop_init();
	}
	
}

int main(void)
{
	board_lowlevel_init(); //底层初始化
	
	xTaskCreate(main_init,"init",1024,NULL,9,NULL); //创建任务
	
	vTaskStartScheduler();  //正式启动内核（总开关）
	
	while(1)
	{
		; //code should not run here;
	}
	
}


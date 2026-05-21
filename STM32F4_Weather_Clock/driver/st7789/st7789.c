#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx_conf.h"
#include "cpu_tick.h"
#include "st7789.h"
#include "font.h"
#include "image.h"

// CLK(SCL) —— PB13    
// MOSI(SDA) —— PC3    
// MISO —— PC2
// CS —— PE2      片选信号，低电平有效
// RESET —— PE3   复位引脚， 低电平复位
// DC —— PE4		数据(1)/命令(0)选择
// BL —— PE5      背光控制，高电平点亮背光 

#define CS_PORT     GPIOE
#define CS_PIN      GPIO_Pin_2
#define RESET_PORT  GPIOE
#define RESET_PIN   GPIO_Pin_3
#define DC_PORT     GPIOE
#define DC_PIN      GPIO_Pin_4
#define BL_PORT     GPIOE
#define BL_PIN      GPIO_Pin_5


static void st7789_init_display(void);

static void st7789_io_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);
    
    GPIO_SetBits(GPIOE, GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5);
    GPIO_ResetBits(BL_PORT, BL_PIN);
    
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_High_Speed;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_Init(GPIOE, &GPIO_InitStruct);
    
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource2, GPIO_AF_SPI2);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_SPI2);
	
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_High_Speed;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_Init(GPIOC, &GPIO_InitStruct);

}

static void st7789_spi_init(void)
{
	SPI_InitTypeDef SPI_InitStruct;
    SPI_StructInit(&SPI_InitStruct);
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB; //高位在前
    SPI_Init(SPI2, &SPI_InitStruct);
	SPI_DMACmd(SPI2,SPI_I2S_DMAReq_Tx,ENABLE);
    SPI_Cmd(SPI2, ENABLE);
}

static void st7789_dma_init(void)
{
	DMA_InitTypeDef DMA_InitStructure;
	DMA_StructInit(&DMA_InitStructure);
	DMA_InitStructure.DMA_Channel = DMA_Channel_0;  //// 通道，根据芯片手册里的映射表
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI2->DR; //外设数据寄存器地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral; //传输方向：内存->外设
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; //不自增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; //一次读两个字节
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;     // 
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; 
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_INC8;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA1_Stream4,&DMA_InitStructure);
	DMA_Cmd(DMA1_Stream4,ENABLE);
}

static void  st7789_interrupt_init(void)
{
;
}

void st7789_init(void)
{
    
	st7789_spi_init();
	st7789_dma_init();
	st7789_interrupt_init();
	st7789_io_init();
	
    st7789_init_display();
}

//通过SPI向st7789写入命令和数据
static void st7789_write_register(uint8_t reg, uint8_t data[], uint16_t length) //reg是要写入的寄存器地址
{
	SPI_DataSizeConfig(SPI2,SPI_DataSize_8b);
	
    GPIO_ResetBits(CS_PORT, CS_PIN); //片选拉低，开始通信
    
    GPIO_ResetBits(DC_PORT, DC_PIN); //拉低DC,代表发送的是命令
    SPI_SendData(SPI2, reg);
    while (SPI_GetFlagStatus(SPI2, SPI_FLAG_TXE) == RESET);
    while (SPI_GetFlagStatus(SPI2, SPI_FLAG_BSY) != RESET);
    
    GPIO_SetBits(DC_PORT, DC_PIN); //DC拉高，代表接下来发送数据
    for (uint16_t i = 0; i < length; i++)
    {
        SPI_SendData(SPI2, data[i]);
        while (!SPI_GetFlagStatus(SPI2, SPI_FLAG_TXE));
    }
    while (SPI_GetFlagStatus(SPI2, SPI_FLAG_BSY) != RESET);
    
    GPIO_SetBits(CS_PORT, CS_PIN); //片选拉高，代表结束通信
}

static void st7789_write_gram(uint8_t data[], uint32_t length, bool single_color)
{
	SPI_DataSizeConfig(SPI2,SPI_DataSize_16b);
	
	GPIO_ResetBits(CS_PORT, CS_PIN);  //cs片选拉低，表示开始通信
    GPIO_SetBits(DC_PORT, DC_PIN); //dc拉高代表传输的是数据，拉低代表传输的是命令	
	
	length >>= 1; //       length/2
	
	do
	{
		uint32_t chunk_size = length < 65535 ? length :65535;
		DMA_InitTypeDef DMA_InitStructure;
		DMA_StructInit(&DMA_InitStructure);
		
		DMA_Cmd(DMA1_Stream4, DISABLE);
        while (DMA1_Stream4->CR & DMA_SxCR_EN);  
		
		//DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)data;  //接收缓冲区数组首地址
		DMA1_Stream4->M0AR = (uint32_t)data;
		//DMA_InitStructure.DMA_BufferSize = chunk_size ;    //缓冲区元素个数
		DMA1_Stream4->NDTR  = chunk_size;
		//DMA_InitStructure.DMA_MemoryInc = single_color ? DMA_MemoryInc_Disable : DMA_MemoryInc_Enable;
		if(single_color) DMA1_Stream4->CR &= ~DMA_SxCR_MINC;
		else 			 DMA1_Stream4->CR |= DMA_SxCR_MINC;
		
		DMA_Cmd(DMA1_Stream4,ENABLE);
		while (DMA_GetFlagStatus(DMA1_Stream4,DMA_FLAG_TCIF4) == RESET); //等DMA把数据搬完
		DMA_ClearFlag(DMA1_Stream4,DMA_FLAG_TCIF4);
		
		if(!single_color)
			data +=chunk_size * 2 ;
		length -=chunk_size;
		
	}while(length > 0);
	
	while (SPI_GetFlagStatus(SPI2,SPI_FLAG_BSY) != RESET); /* SPI 和 DMA 结合时，DMA 把数据送到 SPI DR 后：
	SPI仍然需要几拍时钟把数据完全移出，如果不等 BSY 清零就关闭 SPI 或切换片选（CS），
	会导致：数据丢失、最后一个字节没发完、从机通信异常、所以这是 SPI 收尾保护动作。*/
	
    GPIO_SetBits(CS_PORT, CS_PIN);
}

//st7789初始化， 规定先拉低RESET引脚一段时间后再拉高
static void st7789_reset(void)   
{
    GPIO_ResetBits(RESET_PORT, RESET_PIN);
	vTaskDelay(pdMS_TO_TICKS(2));//这里至少20us
    GPIO_SetBits(RESET_PORT, RESET_PIN);
    vTaskDelay(pdMS_TO_TICKS(120));
}

static void st7789_set_backlight(bool on)
{
    GPIO_WriteBit(BL_PORT, BL_PIN, on ? Bit_SET : Bit_RESET);
}

//初始化LCD控制器
static void st7789_init_display(void)
{
    st7789_reset(); //先重置一下LCD

    st7789_write_register(0x11, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    
//	st7789_write_register(0x36,(uint8_t[]){0x00},1);
//	st7789_write_register(0x3a,(uint8_t[]){0x05},1);
//	st7789_write_register(0xb2,(uint8_t[]){0x0c,0x0c,0x00,0x33,0x33},5);
//	st7789_write_register(0xb7,(uint8_t[]){0x35},1);
//	st7789_write_register(0xbb,(uint8_t[]){0x28},1);
//	st7789_write_register(0xc0,(uint8_t[]){0x2c},1);
//	st7789_write_register(0xc2,(uint8_t[]){0x01},1);
//	st7789_write_register(0xc3,(uint8_t[]){0x0b},1);
//	st7789_write_register(0xc4,(uint8_t[]){0x20},1);
//	st7789_write_register(0xc6,(uint8_t[]){0x0f},1);
//	st7789_write_register(0xd0,(uint8_t[]){0xa4,0xa1},2);
//	st7789_write_register(0xe0,(uint8_t[]){0xd0,0x01,0x08,0x0f,0x11,0x2a,0x36,0x55,0x44,0x3a,0x0b,0x06,0x11,0x20},14);
//	st7789_write_register(0xe1,(uint8_t[]){0xd0,0x02,0x07,0x0a,0x0b,0x18,0x34,0x43,0x4a,0x2b,0x1b,0x1c,0x22,0x1f},14);
//	st7789_write_register(0x29,NULL,0);
	
    st7789_write_register(0x36, (uint8_t[]){0x00}, 1);
    st7789_write_register(0x3A, (uint8_t[]){0x55}, 1);
    st7789_write_register(0xB7, (uint8_t[]){0x46}, 1);
    st7789_write_register(0xBB, (uint8_t[]){0x1B}, 1);
    st7789_write_register(0xC0, (uint8_t[]){0x2C}, 1);
    st7789_write_register(0xC2, (uint8_t[]){0x01}, 1);
    st7789_write_register(0xC4, (uint8_t[]){0x20}, 1);
    st7789_write_register(0xC6, (uint8_t[]){0x0F}, 1);
    st7789_write_register(0xD0, (uint8_t[]){0xA4,0xA1}, 2);
    st7789_write_register(0xD6, (uint8_t[]){0xA1}, 1);
    st7789_write_register(0xE0, (uint8_t[]){0xF0,0x00,0x06,0x04,0x05,0x05,0x31,0x44,0x48,0x36,0x12,0x12,0x2B,0x34}, 14);
    st7789_write_register(0xE0, (uint8_t[]){0xF0,0x0B,0x0F,0x0F,0x0D,0x26,0x31,0x43,0x47,0x38,0x14,0x14,0x2C,0x32}, 14);
    st7789_write_register(0x21, NULL, 0);
    st7789_write_register(0x29, NULL, 0);
    
    st7789_fill_color(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1, 0x0000);
    st7789_set_backlight(true);
}

//判断是否超出屏幕
static bool in_screen_range(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)  
{
    if (x1 >= ST7789_WIDTH || y1 >= ST7789_HEIGHT)
        return false;
    if (x2 >= ST7789_WIDTH || y2 >= ST7789_HEIGHT)
        return false;
    if (x1 > x2 || y1 > y2)
        return false;

    return true;
}

//告诉st7789接下来要在屏幕的哪个区域写像素数据
static void st7789_set_range_and_prepare_gram(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) //这个x1,x2,y1,y2写法是由手册p198定死的，x1高八位，x1低八位,x2高八位，x2低八位
{
    st7789_write_register(0x2A, (uint8_t[]){(x1 >> 8) & 0xff, x1 & 0xff, (x2 >> 8) & 0xff, x2 & 0xff}, 4); //0x2a设置列地址范围 我的思考：为何一定要&0xff?
    st7789_write_register(0x2B, (uint8_t[]){(y1 >> 8) & 0xff, y1 & 0xff, (y2 >> 8) & 0xff, y2 & 0xff}, 4); //0x2b设置行地址范围
    st7789_write_register(0x2C, NULL, 0);  //准备写入像素数据
}

void st7789_fill_color(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    if (!in_screen_range(x1, y1, x2, y2))
        return;
    
    st7789_set_range_and_prepare_gram(x1, y1, x2, y2);
    
    uint32_t pixels = (x2 - x1 + 1) * (y2 - y1 + 1); //计算要填充的像素总数
	st7789_write_gram((uint8_t*)&color,pixels*2,true);
	
}


//函数签名：在屏幕 (x,y) 处，绘制一个宽 width、高 height 的字模（bitmap）。
//model 指向字模数据（按行存储，每行若干字节）。
//color 是前景色（字的颜色），bg_color 是背景色（字以外的像素填充色）
static void st7789_draw_font(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *model, uint16_t color, uint16_t bg_color)
{
    uint16_t bytes_per_row = (width + 7) / 8; //计算该model每行所需字节数
    
	static uint8_t buff[72*72*2];
	uint8_t *pbuf = buff;
	
	for (uint16_t row = 0; row < height; row++) //循环遍历每一行像素
	{
		const uint8_t *row_data = model + row * bytes_per_row; //row_data指向model在当前行的起始位置，例如row =0是，row_data=&model[0]也就是指向model字模第0行的起始字节
		for (uint16_t col = 0; col < width; col++) //循环遍历当前行的每一个像素
		{
			uint8_t pixel = row_data[col / 8] & (1 << (7 - col % 8)); //row_data[col / 8]是一个字节，里面有八位像素！
			uint16_t pixel_color = pixel ? color : bg_color;
			*pbuf++ = pixel_color  & 0xff;
			*pbuf++ = (pixel_color>>8) & 0xff;
		}
	}

    st7789_set_range_and_prepare_gram(x, y, x + width - 1, y + height - 1);
    st7789_write_gram(buff,pbuf - buff,false);
	
}

static const uint8_t *ascii_get_model(const char ch,const font_t *font)
{
	uint16_t bytes_per_row = (font->size / 2+7) / 8;
	uint16_t bytes_per_char = font->size * bytes_per_row;
	if(font->ascii_map)
	{
		const char *map = font->ascii_map;
		do{
			if(*map ==ch)
			{
				return font->ascii_model + (map-font->ascii_map) * bytes_per_char;
			}
		} while(*(++map) != '\0'); //*(++p)指的是让 p 指向下一个元素，然后取这个新位置的内容。
	}
	else
	{
		return font->ascii_model + (ch - ' ') * bytes_per_char;
	}
	
	return NULL;
}

//显示单个 ASCII 字符（英文字母、数字、符号）
static void st7789_write_ascii(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color, const font_t *font)
{
    if (font == NULL)  //判断字库是否存在
        return;
    
    uint16_t fheight = font->size, fwidth = font->size / 2; //获取字符的尺寸，宽度是高度的一半
    if (!in_screen_range(x, y, x + fwidth - 1, y + fheight - 1))  //检查是否超出屏幕
        return;
    
    if (ch < 0x20 || ch > 0x7E) //判断字符是否在ASCII字符范围里
        return;
	
	const uint8_t *model = ascii_get_model(ch,font);
	if(model)
		st7789_draw_font(x, y, fwidth, fheight, model, color, bg_color);
}

//显示单个中文字符
static void st7789_write_chinese(uint16_t x, uint16_t y,const char *ch, uint16_t color, uint16_t bg_color, const font_t *font)
{
    if (ch == NULL || font == NULL)   //判断输入的字符是否为空以及字库是否存在
        return;

    uint16_t fheight = font->size, fwidth = font->size; //获取字的大小
    if (!in_screen_range(x, y, x + fwidth - 1, y + fheight - 1)) //判断是否超出屏幕
        return;
	
    const font_chinese_t *c = font->chinese;  //定义一个c变量这样子容易表示，不然要font->chinese->name比较繁琐
    for (; c->name != NULL; c++) //遍历整个中文字库数组，找到名字（汉字）与输入的 ch 匹配的那个结构体。
    {
        if (strcmp(c->name, ch) == 0)   //如果相等，则跳出循环
            break;
    }
    if (c->name == NULL)  //如果遍历完都没有找到这个汉字，直接退出
        return;
    
    st7789_draw_font(x, y, fwidth, fheight, c->model, color, bg_color);
}

static bool is_gb2312(char ch)  //判断给定的字符ch是否在gb2312标准的范围内
{
    return ((unsigned char)ch >= 0xA1 && (unsigned char)ch <= 0xF7);
}

void st7789_write_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color, const font_t *font)
{
    while (*str != '\0' )  //遍历字符串(只要当前不是 '\0'就会一直继续),指针指向的是每个字节
    {
        // int len = utf8_char_length(*str);
        int len = is_gb2312(*str) ? 2 : 1; //判断是Ascii字符(一个字节)还是gb2312标准的中文字符(2个字节)
        if (len <= 0) //无效字符过滤(保险起见)
        {
            str++;
            continue;
        }
        else if (len == 1) //如果是ascii字符
        {
            st7789_write_ascii(x, y, *str, color, bg_color, font);
            str++;
            x += font->size / 2; //ascii字符宽度是中文的一半
        }
        else //如果是中文字符
        {
            char ch[5];
            strncpy(ch, str, len); //把当前的str复制到临时字符串ch
            st7789_write_chinese(x, y, ch, color, bg_color, font);
            str += len;
            x += font->size;
        }
    }
}


//显示一张图片
void st7789_draw_image(uint16_t x,uint16_t y,const image_t *image)
{
	if(x > ST7789_WIDTH || y >ST7789_HEIGHT ||(x+image->width) >ST7789_WIDTH || (y+image->height) > ST7789_HEIGHT)
		return;
    
    st7789_set_range_and_prepare_gram(x, y, x + image->width - 1, y + image->height - 1);
    
	st7789_write_gram((uint8_t*)image->data,(image->height)*(image->width)*2,false);
	
    
}

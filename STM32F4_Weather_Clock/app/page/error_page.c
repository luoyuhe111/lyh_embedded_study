#include "st7789.h"
#include "font.h"
#include "image.h"
#include <stdint.h>
#include <string.h>
void error_page_display(const char *msg)
{
    const uint16_t color_bg = mkcolor(0, 0, 0); //±³¾°É«ºÚÉ«
    st7789_fill_color(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1, color_bg);
    st7789_draw_image(40,37, &image_error);
	
	uint16_t start_x =0;
	int len = strlen(msg) *font20_maple_bold.size / 2;
	if(len < ST7789_WIDTH)
		start_x = (ST7789_WIDTH - len +1) / 2;
    st7789_write_string(start_x, 245, msg, mkcolor(255, 255, 0), color_bg, &font20_maple_bold);
}

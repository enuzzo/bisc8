#ifndef PORT_DISPLAY_H
#define PORT_DISPLAY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DRIVER_COLOR_WHITE  = 0xff,
    DRIVER_COLOR_BLACK  = 0x00,
    FONT_BACKGROUND = DRIVER_COLOR_WHITE,
}COLOR_IMAGE;


void PortLvgl_Start_Init();
void PortDisplay_Init();
void EPD_Init();    /* 墨水屏初始化 */
void EPD_Clear();   /* 清空屏幕 */
void EPD_Display(); /* 刷buffer到墨水屏 */

/*局部刷新*/
void EPD_DisplayPartBaseImage();
void EPD_Init_Partial();
void EPD_DisplayPart();
void EPD_DrawColorPixel(uint16_t x, uint16_t y,uint8_t color);
size_t EPD_FrameBufferSize();
bool EPD_CopyFrameBuffer(uint8_t *dst, size_t len);




#ifdef __cplusplus
}
#endif




#endif

#ifndef OLED_GRAPHICS_H
#define OLED_GRAPHICS_H

#include <stdint.h>

// Kích thước màn hình OLED
#define OLED_WIDTH       128U
#define OLED_HEIGHT      64U

// Kích thước bộ đệm ảnh (byte)
#define OLED_BUFFER_SIZE ((OLED_WIDTH * OLED_HEIGHT) / 8U)


// Định nghĩa màu sắc của OLED
typedef enum
{
    OLED_BLACK = 0,
    OLED_WHITE = 1
} OLED_Color;


void OLED_Clear(void);
void OLED_Fill(OLED_Color color);
void OLED_DrawPixel(int16_t x, int16_t y, OLED_Color color);
#endif
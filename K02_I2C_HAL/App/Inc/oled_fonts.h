#ifndef OLED_FONTS_H
#define OLED_FONTS_H

#include <stdint.h>

// Kích thước và bảng điểm ảnh của font
typedef struct
{
    uint8_t width;
    uint8_t height;
    const uint8_t *data;
} OLED_Font;

// Hai font hỗ trợ ký tự ASCII từ 32 đến 126
extern const OLED_Font OLED_FONT_5X7;
extern const OLED_Font OLED_FONT_8X16;

#endif
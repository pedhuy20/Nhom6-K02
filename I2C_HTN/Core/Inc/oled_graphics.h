#ifndef OLED_GRAPHICS_H
#define OLED_GRAPHICS_H

#include <stdint.h>
#include "oled_fonts.h"

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

// Tọa độ một điểm
typedef struct
{
    int16_t x;
    int16_t y;
} OLED_Point;


void OLED_Clear(void);
void OLED_Fill(OLED_Color color);
void OLED_DrawPixel(int16_t x, int16_t y, OLED_Color color);
void OLED_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, OLED_Color color);
void OLED_DrawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, OLED_Color color);
void OLED_FillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, OLED_Color color);
void OLED_DrawGraph(int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
                    const int16_t *values, uint16_t count,
                    int16_t value_min, int16_t value_max,
                    OLED_Color color);
void OLED_DrawPolyline(const OLED_Point *points, uint16_t count, OLED_Color color);
// Vẽ một ký tự; trả về 1 nếu vẽ được, 0 nếu không vẽ được
uint8_t OLED_DrawChar(int16_t x, int16_t y, char ch,
                      const OLED_Font *font, OLED_Color color);

// Vẽ chuỗi trên một dòng; trả về số ký tự đã vẽ
uint16_t OLED_DrawText(int16_t x, int16_t y, const char *text,
                       const OLED_Font *font, OLED_Color color);
// Lấy con trỏ chỉ đọc tới bộ đệm ảnh
const uint8_t *OLED_GetFramebuffer(void);
#endif 
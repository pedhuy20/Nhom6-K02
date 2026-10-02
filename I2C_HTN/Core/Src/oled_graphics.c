#include "oled_graphics.h"

//Bộ đệm ảnh
static uint8_t OLED_Framebuffer[OLED_BUFFER_SIZE];

//Xóa toàn bộ màn hình về màu đen
void OLED_Clear(void)
{
    uint16_t i;

    for (i = 0U; i < OLED_BUFFER_SIZE; i++)
    {
        OLED_Framebuffer[i] = 0U;
    }
}

//Tô màu toàn bộ màn hình với màu đã cho
void OLED_Fill(OLED_Color color)
{
    uint16_t i;
    uint8_t value;

    if (color != OLED_BLACK && color != OLED_WHITE)
    {
        return;
    }

    if (color == OLED_WHITE)
    {
        value = 0xFFU;
    }
    else
    {
        value = 0x00U;
    }

    for (i = 0U; i < OLED_BUFFER_SIZE; i++)
    {
        OLED_Framebuffer[i] = value;
    }
}

// Vẽ một điểm ảnh trong bộ đệm
void OLED_DrawPixel(int16_t x, int16_t y, OLED_Color color)
{
    uint16_t index;
    uint8_t mask;

    // Kiểm tra tọa độ
    if (x < 0 || x >= (int16_t)OLED_WIDTH ||
        y < 0 || y >= (int16_t)OLED_HEIGHT)
    {
        return;
    }

    // Kiểm tra màu
    if (color != OLED_BLACK && color != OLED_WHITE)
    {
        return;
    }

    // Xác định byte và bit chứa điểm ảnh
    index = (uint16_t)(x + (y / 8) * OLED_WIDTH);
    mask = (uint8_t)(1U << (y % 8));

    if (color == OLED_WHITE)
    {
        OLED_Framebuffer[index] |= mask;
    }
    else
    {
        OLED_Framebuffer[index] &= ~mask;
    }
}
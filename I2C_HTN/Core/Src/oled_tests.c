#include "oled_tests.h"
#include "oled_graphics.h"

// Kết quả kiểm tra từng hàm
volatile uint8_t g_oled_clear_ok;
volatile uint8_t g_oled_fill_ok;
volatile uint8_t g_oled_pixel_ok;

// Kiểm tra mọi byte trong bộ đệm có bằng value không
static uint8_t BufferIs(uint8_t value)
{
    const uint8_t *buffer = OLED_GetFramebuffer();
    uint16_t i;

    for (i = 0U; i < OLED_BUFFER_SIZE; i++)
    {
        if (buffer[i] != value)
        {
            return 0U;
        }
    }

    return 1U;
}

// Kiểm tra xóa bộ đệm đang trắng
uint8_t OLED_TestClear(void)
{
    OLED_Fill(OLED_WHITE);
    OLED_Clear();

    return BufferIs(0x00U);
}

// Kiểm tra tô trắng và tô đen
uint8_t OLED_TestFill(void)
{
    // Tô trắng bộ đệm đang đen
    OLED_Clear();
    OLED_Fill(OLED_WHITE);

    if (BufferIs(0xFFU) == 0U)
    {
        return 0U;
    }

    // Tô đen bộ đệm đang trắng
    OLED_Fill(OLED_BLACK);

    return BufferIs(0x00U);
}

// Kiểm tra chức năng cơ bản của DrawPixel
uint8_t OLED_TestDrawPixel(void)
{
    const uint8_t *buffer = OLED_GetFramebuffer();

    OLED_Clear();

    // Bật bit 3 của byte 10
    OLED_DrawPixel(10, 3, OLED_WHITE);

    if (buffer[10] != 0x08U)
    {
        return 0U;
    }

    // Bật bit 4 hai lần, giữ nguyên bit 3
    OLED_DrawPixel(10, 4, OLED_WHITE);
    OLED_DrawPixel(10, 4, OLED_WHITE);

    if (buffer[10] != 0x18U)
    {
        return 0U;
    }

    // Tắt bit 3 hai lần, giữ nguyên bit 4
    OLED_DrawPixel(10, 3, OLED_BLACK);
    OLED_DrawPixel(10, 3, OLED_BLACK);

    if (buffer[10] != 0x10U)
    {
        return 0U;
    }

    
    OLED_DrawPixel(10, 4, OLED_BLACK);

    
    OLED_DrawPixel(-1, 0, OLED_WHITE);
    OLED_DrawPixel(0, -1, OLED_WHITE);
    OLED_DrawPixel(128, 0, OLED_WHITE);
    OLED_DrawPixel(0, 64, OLED_WHITE);

    // Bộ đệm phải trở về toàn bộ bằng 0
    return BufferIs(0x00U);
}

// Chạy tất cả bài kiểm tra
void OLED_TestAll(void)
{
    g_oled_clear_ok = OLED_TestClear();
    g_oled_fill_ok = OLED_TestFill();
    g_oled_pixel_ok = OLED_TestDrawPixel();
}
#include "oled_tests.h"
#include "oled_graphics.h"

// Kết quả kiểm tra từng hàm
volatile uint8_t g_oled_clear_ok;
volatile uint8_t g_oled_fill_ok;
volatile uint8_t g_oled_pixel_ok;
volatile uint8_t g_oled_line_ok;
volatile uint8_t g_oled_rect_ok;
volatile uint8_t g_oled_fillrect_ok;
volatile uint8_t g_oled_char_ok = 0U;
volatile uint8_t g_oled_text_ok = 0U;

// Ảnh chữ A của font 5x7: năm byte ở page đầu
static const uint8_t expected_a5[5] = {
    0x7CU, 0x12U, 0x11U, 0x12U, 0x7CU
};

// Ảnh chữ A của font 8x16: tám byte mỗi page
static const uint8_t expected_a8[16] = {
    0xE0U, 0xE0U, 0x18U, 0x06U,
    0x06U, 0x18U, 0xE0U, 0x00U,

    0x7FU, 0x7FU, 0x06U, 0x06U,
    0x06U, 0x06U, 0x7FU, 0x00U
};

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

// So sánh các byte đầu; những byte còn lại phải bằng 0
static uint8_t BufferMatches(const uint8_t *expected, uint16_t size)
{
    const uint8_t *buffer = OLED_GetFramebuffer();
    uint16_t i;
    uint8_t value;

    for (i = 0U; i < OLED_BUFFER_SIZE; i++)
    {
        value = 0U;

        if (i < size)
        {
            value = expected[i];
        }

        if (buffer[i] != value)
        {
            return 0U;
        }
    }

    return 1U;
}

// Kiểm tra các trường hợp vẽ đường thẳng cơ bản
uint8_t OLED_TestDrawLine(void)
{
    const uint8_t horizontal[] = {
        0x08U, 0x08U, 0x08U, 0x08U, 0x08U
    };

    const uint8_t vertical[] = {
        0xFFU
    };

    const uint8_t diagonal[] = {
        0x01U, 0x01U, 0x02U, 0x02U,
        0x02U, 0x04U, 0x04U
    };

    const uint8_t steep[] = {
        0x03U, 0x1CU, 0x60U
    };

    const uint8_t point[] = {
        0x01U
    };

    // Đường ngang
    OLED_Clear();
    OLED_DrawLine(0, 3, 4, 3, OLED_WHITE);
    if (BufferMatches(horizontal, 5U) == 0U)
    {
        return 0U;
    }

    // Đường dọc
    OLED_Clear();
    OLED_DrawLine(0, 0, 0, 7, OLED_WHITE);
    if (BufferMatches(vertical, 1U) == 0U)
    {
        return 0U;
    }

    // Đường xiên có dx > dy
    OLED_Clear();
    OLED_DrawLine(0, 0, 6, 2, OLED_WHITE);
    if (BufferMatches(diagonal, 7U) == 0U)
    {
        return 0U;
    }

    // Đảo điểm đầu và điểm cuối
    OLED_Clear();
    OLED_DrawLine(6, 2, 0, 0, OLED_WHITE);
    if (BufferMatches(diagonal, 7U) == 0U)
    {
        return 0U;
    }

    // Vẽ màu đen để xóa đường vừa vẽ
    OLED_DrawLine(6, 2, 0, 0, OLED_BLACK);
    if (BufferIs(0x00U) == 0U)
    {
        return 0U;
    }

    // Đường xiên có dy > dx
    OLED_Clear();
    OLED_DrawLine(0, 0, 2, 6, OLED_WHITE);
    if (BufferMatches(steep, 3U) == 0U)
    {
        return 0U;
    }

    // Hai điểm trùng nhau
    OLED_Clear();
    OLED_DrawLine(0, 0, 0, 0, OLED_WHITE);
    return BufferMatches(point, 1U);
}

// Kiểm tra vẽ khung chữ nhật
uint8_t OLED_TestDrawRect(void)
{
    const uint8_t expected[] = {
        0x1FU, 0x11U, 0x11U, 0x11U, 0x1FU
    };

    const uint8_t clipped[] = {
        0x04U, 0x04U, 0x07U
    };

    // Vẽ khung từ (0,0) đến (4,4)
    OLED_Clear();
    OLED_DrawRect(0, 0, 4, 4, OLED_WHITE);

    if (BufferMatches(expected, sizeof(expected)) == 0U)
    {
        return 0U;
    }

    // Đảo hai góc, kết quả phải giống nhau
    OLED_Clear();
    OLED_DrawRect(4, 4, 0, 0, OLED_WHITE);

    if (BufferMatches(expected, sizeof(expected)) == 0U)
    {
        return 0U;
    }

    // Xóa khung vừa vẽ
    OLED_DrawRect(0, 0, 4, 4, OLED_BLACK);

    if (BufferIs(0x00U) == 0U)
    {
        return 0U;
    }

    // Chỉ vẽ phần cạnh nằm trong màn hình
    OLED_Clear();
    OLED_DrawRect(-2, -2, 2, 2, OLED_WHITE);

    return BufferMatches(clipped, sizeof(clipped));
}

// Kiểm tra tô kín hình chữ nhật
uint8_t OLED_TestFillRect(void)
{
    const uint8_t expected[] = {
        0x1FU, 0x1FU, 0x1FU, 0x1FU, 0x1FU
    };

    const uint8_t clipped[] = {
        0x07U, 0x07U, 0x07U
    };

    // Tô kín từ (0,0) đến (4,4)
    OLED_Clear();
    OLED_FillRect(0, 0, 4, 4, OLED_WHITE);

    if (BufferMatches(expected, sizeof(expected)) == 0U)
    {
        return 0U;
    }

    // Đảo hai góc, kết quả phải giống nhau
    OLED_Clear();
    OLED_FillRect(4, 4, 0, 0, OLED_WHITE);

    if (BufferMatches(expected, sizeof(expected)) == 0U)
    {
        return 0U;
    }

    // Xóa vùng vừa tô
    OLED_FillRect(0, 0, 4, 4, OLED_BLACK);

    if (BufferIs(0x00U) == 0U)
    {
        return 0U;
    }

    // Chỉ tô phần nằm trong màn hình
    OLED_Clear();
    OLED_FillRect(-2, -2, 2, 2, OLED_WHITE);

    return BufferMatches(clipped, sizeof(clipped));
}

// Kiểm tra vẽ một ký tự bằng hai font
uint8_t OLED_TestDrawChar(void)
{
    uint8_t i;
    uint8_t expected[136] = {0U};

    // Vẽ chữ A bằng font nhỏ
    OLED_Clear();

    if (OLED_DrawChar(0, 0, 'A',
                      &OLED_FONT_5X7, OLED_WHITE) != 1U)
        return 0U;

    if (BufferMatches(expected_a5, sizeof(expected_a5)) == 0U)
        return 0U;

    // Chuẩn bị ảnh chữ A bằng font lớn
    for (i = 0U; i < 8U; i++)
    {
        expected[i] = expected_a8[i];

        // Page thứ hai bắt đầu từ byte 128
        expected[128U + i] = expected_a8[8U + i];
    }

    OLED_Clear();

    if (OLED_DrawChar(0, 0, 'A',
                      &OLED_FONT_8X16, OLED_WHITE) != 1U)
        return 0U;

    if (BufferMatches(expected, sizeof(expected)) == 0U)
        return 0U;

    // Không đủ chỗ hoặc ký tự không hợp lệ
    OLED_Clear();

    if (OLED_DrawChar(124, 0, 'A',
                      &OLED_FONT_5X7, OLED_WHITE) != 0U)
        return 0U;

    if (OLED_DrawChar(0, 0, '\n',
                      &OLED_FONT_5X7, OLED_WHITE) != 0U)
        return 0U;

    return BufferIs(0U);
}

// Kiểm tra vẽ chuỗi và khoảng cách giữa các ký tự
uint8_t OLED_TestDrawText(void)
{
    uint8_t i;
    uint8_t expected_small[17] = {0U};
    uint8_t expected_large[145] = {0U};

    // Chuỗi A A: hai chữ A ở x = 0 và x = 12
    for (i = 0U; i < 5U; i++)
    {
        expected_small[i] = expected_a5[i];
        expected_small[12U + i] = expected_a5[i];
    }

    OLED_Clear();

    if (OLED_DrawText(0, 0, "A A",
                      &OLED_FONT_5X7, OLED_WHITE) != 3U)
        return 0U;

    if (BufferMatches(expected_small, sizeof(expected_small)) == 0U)
        return 0U;

    // Chuỗi AA: hai chữ A ở x = 0 và x = 9
    for (i = 0U; i < 8U; i++)
    {
        expected_large[i] = expected_a8[i];
        expected_large[9U + i] = expected_a8[i];

        expected_large[128U + i] = expected_a8[8U + i];
        expected_large[137U + i] = expected_a8[8U + i];
    }

    OLED_Clear();

    if (OLED_DrawText(0, 0, "AA",
                      &OLED_FONT_8X16, OLED_WHITE) != 2U)
        return 0U;

    if (BufferMatches(expected_large, sizeof(expected_large)) == 0U)
        return 0U;

    // Cuối màn hình chỉ đủ hai trong ba chữ A
    OLED_Clear();

    if (OLED_DrawText(117, 0, "AAA",
                      &OLED_FONT_5X7, OLED_WHITE) != 2U)
        return 0U;

    return 1U;
}


// Chạy toàn bộ bài kiểm tra
void OLED_TestAll(void)
{
    g_oled_clear_ok    = OLED_TestClear();
    g_oled_fill_ok     = OLED_TestFill();
    g_oled_pixel_ok    = OLED_TestDrawPixel();
    g_oled_line_ok     = OLED_TestDrawLine();
    g_oled_rect_ok     = OLED_TestDrawRect();
    g_oled_fillrect_ok = OLED_TestFillRect();
    g_oled_char_ok     = OLED_TestDrawChar();
    g_oled_text_ok     = OLED_TestDrawText();
}
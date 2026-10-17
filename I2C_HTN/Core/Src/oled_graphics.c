#include "oled_graphics.h"
#include <stdlib.h>
//Bộ đệm ảnh
static uint8_t OLED_Framebuffer[OLED_BUFFER_SIZE];

// Lấy con trỏ chỉ đọc tới bộ đệm
const uint8_t *OLED_GetFramebuffer(void)
{
    return OLED_Framebuffer;
}

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

// Vẽ đường thẳng nối hai điểm(Thuật toán Breshenham vẽ đoạn thẳng)
void OLED_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, OLED_Color color)
{
    // Khoảng cách theo hai trục
    int32_t dx = abs(x1 - x0);
    int32_t dy = abs(y1 - y0);

    // Hướng di chuyển tới điểm cuối
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;

    // Sai số dùng để chọn pixel tiếp theo
    int32_t err = dx - dy;
    int32_t e2;

    while (1)
    {
        OLED_DrawPixel(x0, y0, color);

        // Dừng sau khi vẽ điểm cuối
        if (x0 == x1 && y0 == y1)
        {
            break;
        }

        e2 = 2 * err;

        // Di chuyển theo trục x
        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }

        // Di chuyển theo trục y
        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }

}

// Vẽ khung chữ nhật từ hai góc đối diện
void OLED_DrawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, OLED_Color color)
{
    // Hai cạnh ngang
    OLED_DrawLine(x0, y0, x1, y0, color);
    OLED_DrawLine(x0, y1, x1, y1, color);

    // Hai cạnh dọc
    OLED_DrawLine(x0, y0, x0, y1, color);
    OLED_DrawLine(x1, y0, x1, y1, color);
}

// Tô kín hình chữ nhật từ hai góc đối diện
void OLED_FillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, OLED_Color color)
{
    int16_t x_start = (x0 < x1) ? x0 : x1;
    int16_t x_end   = (x0 < x1) ? x1 : x0;
    int16_t y_start = (y0 < y1) ? y0 : y1;
    int16_t y_end   = (y0 < y1) ? y1 : y0;

    int16_t x;
    int16_t y;

    if (x_start < 0) x_start = 0;
    if (y_start < 0) y_start = 0;

    // Tô từng pixel, lần lượt theo từng hàng
    for (y = y_start; y <= y_end && y < (int16_t)OLED_HEIGHT; y++)
    {
        for (x = x_start; x <= x_end && x < (int16_t)OLED_WIDTH; x++)
        {
            OLED_DrawPixel(x, y, color);
        }
    }
}

// Nối các điểm thành đường gấp khúc
void OLED_DrawPolyline(const OLED_Point *points, uint16_t count, OLED_Color color)
{
    uint16_t i;

    // Kiểm tra dữ liệu
    if (points == NULL || count == 0U)
    {
        return;
    }

    // Kiểm tra màu
    if (color != OLED_BLACK && color != OLED_WHITE)
    {
        return;
    }

    // Vẽ một điểm nếu mảng chỉ có một phần tử
    if (count == 1U)
    {
        OLED_DrawPixel(points[0].x, points[0].y, color);
        return;
    }

    // Nối từng cặp điểm liên tiếp
    for (i = 1U; i < count; i++)
    {
        OLED_DrawLine(points[i - 1U].x,
                      points[i - 1U].y,
                      points[i].x,
                      points[i].y,
                      color);
    }
}

// Vẽ đồ thị từ mảng giá trị
void OLED_DrawGraph(int16_t x0, int16_t y0,
                    int16_t x1, int16_t y1,
                    const int16_t *values, uint16_t count,
                    int16_t value_min, int16_t value_max,
                    OLED_Color color)
{
    uint16_t i;
    int32_t value;
    int16_t x;
    int16_t y;
    int16_t prev_x = x0;
    int16_t prev_y = y1;

    // Kiểm tra dữ liệu và thang đo
    if (values == NULL || count == 0U ||
        value_min >= value_max)
    {
        return;
    }

    // Kiểm tra vùng vẽ nằm trong màn hình
    if (x0 < 0 || y0 < 0 ||
        x1 >= (int16_t)OLED_WIDTH ||
        y1 >= (int16_t)OLED_HEIGHT ||
        x0 >= x1 || y0 >= y1)
    {
        return;
    }

    // Kiểm tra màu
    if (color != OLED_BLACK && color != OLED_WHITE)
    {
        return;
    }

    // Vẽ trục ngang và trục dọc
    OLED_DrawLine(x0, y1, x1, y1, color);
    OLED_DrawLine(x0, y1, x0, y0, color);

    // Duyệt từng mẫu dữ liệu
    for (i = 0U; i < count; i++)
    {
        value = values[i];

        // Giới hạn giá trị trong thang đo
        if (value < value_min)
        {
            value = value_min;
        }
        else if (value > value_max)
        {
            value = value_max;
        }

        // Chia đều vị trí ngang của các mẫu
        if (count == 1U)
        {
            x = x0 + (x1 - x0) / 2;
        }
        else
        {
            x = x0 + (int32_t)i * (x1 - x0)
                     / (count - 1U);
        }

        // Đổi giá trị thành tọa độ dọc
        y = y1 - (value - value_min) * (y1 - y0)
                 / ((int32_t)value_max - value_min);

        // Vẽ điểm đầu hoặc nối với điểm trước
        if (i == 0U)
        {
            OLED_DrawPixel(x, y, color);
        }
        else
        {
            OLED_DrawLine(prev_x, prev_y, x, y, color);
        }

        // Lưu tọa độ cho lần nối tiếp theo
        prev_x = x;
        prev_y = y;
    }
}

// Vẽ một ký tự vào bộ đệm
uint8_t OLED_DrawChar(int16_t x, int16_t y, char ch,
                      const OLED_Font *font, OLED_Color color)
{
    uint8_t row;
    uint8_t col;
    uint8_t bits;
    uint16_t index;
    OLED_Color background;

    // Kiểm tra font và ký tự
    if (font == NULL || font->data == NULL ||
        font->width == 0U || font->width > 8U ||
        font->height == 0U || ch < 32 || ch > 126)
    {
        return 0U;
    }

    // Kiểm tra màu
    if (color != OLED_BLACK && color != OLED_WHITE)
    {
        return 0U;
    }

    // Chỉ vẽ khi toàn bộ ký tự nằm trong màn hình
    if (x < 0 || y < 0 ||
        (int32_t)x + font->width > (int32_t)OLED_WIDTH ||
        (int32_t)y + font->height > (int32_t)OLED_HEIGHT)
    {
        return 0U;
    }

    // Màu nền đối lập với màu chữ
    background = (color == OLED_WHITE) ? OLED_BLACK : OLED_WHITE;

    // Vị trí bắt đầu của ký tự trong bảng font
    index = (uint16_t)((ch - 32) * font->height);

    // Đọc từng hàng của ký tự
    for (row = 0U; row < font->height; row++)
    {
        bits = font->data[index + row];

        // Đọc từng bit và vẽ điểm ảnh tương ứng
        for (col = 0U; col < font->width; col++)
        {
            if ((bits & (0x80U >> col)) != 0U)
            {
                OLED_DrawPixel(x + col, y + row, color);
            }
            else
            {
                OLED_DrawPixel(x + col, y + row, background);
            }
        }
    }

    return 1U;
}

// Vẽ chuỗi ký tự trên một dòng
uint16_t OLED_DrawText(int16_t x, int16_t y, const char *text,
                       const OLED_Font *font, OLED_Color color)
{
    uint16_t count = 0U;
    uint8_t row;
    OLED_Color background;

    // Kiểm tra con trỏ
    if (text == NULL || font == NULL)
    {
        return 0U;
    }

    background = (color == OLED_WHITE) ? OLED_BLACK : OLED_WHITE;

    // Vẽ lần lượt đến khi hết chuỗi hoặc không còn chỗ
    while (*text != '\0')
    {
        if (OLED_DrawChar(x, y, *text, font, color) == 0U)
        {
            break;
        }

        // Chuyển sang ký tự tiếp theo
        count++;
        text++;
        x += font->width;

        if (*text == '\0' || x >= (int16_t)OLED_WIDTH)
        {
            break;
        }

        // Chừa một cột nền giữa hai ký tự
        for (row = 0U; row < font->height; row++)
        {
            OLED_DrawPixel(x, y + row, background);
        }

        x++;
    }

    return count;
}
#include "i2c_driver.h"
#include <stdint.h>
#include <string.h>

/* Địa chỉ 7-bit mặc định của màn hình OLED SSD1306 (0x78 >> 1 = 0x3C) */
#define OLED_I2C_ADDR           0x3CU

/* Kích thước màn hình OLED SSD1306 128x64 */
#define OLED_WIDTH              128U
#define OLED_HEIGHT             64U
#define OLED_PAGES              (OLED_HEIGHT / 8U)

/* Các biến toàn cục để theo dõi trực tiếp trong cửa sổ Watch của Keil uVision */
volatile I2C_Status_t g_init_status = I2C_STATUS_OK;
volatile I2C_Status_t g_oled_status = I2C_STATUS_OK;
uint8_t  g_found_addrs[16];
uint8_t  g_found_count = 0U;

/* Handle quản lý I2C1 */
static I2C_Handle_t g_i2c1;

/* Bộ đệm khung hình (Framebuffer 1024 byte) */
static uint8_t g_oled_vram[OLED_WIDTH * OLED_PAGES];

/* Hàm gửi 1 lệnh điều khiển xuống SSD1306 (Control byte = 0x00) */
static I2C_Status_t OLED_WriteCommand(uint8_t cmd)
{
    uint8_t buf[2];
    buf[0] = 0x00U; /* Co = 0, D/C# = 0 -> Byte tiếp theo là Command */
    buf[1] = cmd;
    return I2C_Master_Transmit(&g_i2c1, OLED_I2C_ADDR, buf, 2U, I2C_TIMEOUT_DEFAULT_MS);
}

/* Hàm đẩy toàn bộ Framebuffer (8 page x 128 byte) lên OLED SSD1306 */
static I2C_Status_t OLED_UpdateScreen(void)
{
    I2C_Status_t status;
    uint8_t page;
    uint8_t tx_buf[OLED_WIDTH + 1U];

    tx_buf[0] = 0x40U; /* Co = 0, D/C# = 1 -> Các byte tiếp theo là Data (GDDRAM) */

    for (page = 0U; page < OLED_PAGES; page++)
    {
        status = OLED_WriteCommand(0xB0U + page); /* Chọn Page (0..7) */
        if (status != I2C_STATUS_OK) return status;

        status = OLED_WriteCommand(0x00U);        /* Cột thấp = 0 */
        if (status != I2C_STATUS_OK) return status;

        status = OLED_WriteCommand(0x10U);        /* Cột cao = 0 */
        if (status != I2C_STATUS_OK) return status;

        memcpy(&tx_buf[1], &g_oled_vram[page * OLED_WIDTH], OLED_WIDTH);
        status = I2C_Master_Transmit(&g_i2c1, OLED_I2C_ADDR, tx_buf, OLED_WIDTH + 1U, I2C_TIMEOUT_DEFAULT_MS);
        if (status != I2C_STATUS_OK) return status;
    }

    return I2C_STATUS_OK;
}

/* Khởi tạo màn hình OLED SSD1306 từ tập lệnh cơ bản */
static I2C_Status_t OLED_Init(void)
{
    static const uint8_t init_cmds[] = {
        0xAEU,       /* Display OFF */
        0x20U, 0x02U,/* Memory Addressing Mode: Page Addressing Mode */
        0xB0U,       /* Page Start Address = 0 */
        0xC8U,       /* COM Output Scan Direction: Remapped */
        0x00U,       /* Low Column Address = 0 */
        0x10U,       /* High Column Address = 0 */
        0x40U,       /* Start Line Address = 0 */
        0x81U, 0xFFU,/* Contrast Control = Max */
        0xA1U,       /* Segment Re-map: column 127 mapped to SEG0 */
        0xA6U,       /* Normal Display (0xA7 = Inverse Display) */
        0xA8U, 0x3FU,/* Multiplex Ratio = 64 (1/64 duty) */
        0xA4U,       /* Output follows RAM content */
        0xD3U, 0x00U,/* Display Offset = 0 */
        0xD5U, 0xF0U,/* Display Clock Divide Ratio / Oscillator Frequency */
        0xD9U, 0x22U,/* Pre-charge Period */
        0xDAU, 0x12U,/* COM Pins Hardware Configuration */
        0xDBU, 0x20U,/* VCOMH Deselect Level */
        0x8DU, 0x14U,/* Charge Pump Enable */
        0xAFU        /* Display ON */
    };

    I2C_Status_t status;
    uint32_t i;

    for (i = 0U; i < sizeof(init_cmds); i++)
    {
        status = OLED_WriteCommand(init_cmds[i]);
        if (status != I2C_STATUS_OK)
        {
            return status;
        }
    }

    return I2C_STATUS_OK;
}

/* Vẽ 1 điểm ảnh vào Framebuffer để test đồ họa cơ bản */
static void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;

    if (color != 0U)
        g_oled_vram[x + (y / 8U) * OLED_WIDTH] |= (uint8_t)(1U << (y & 7U));
    else
        g_oled_vram[x + (y / 8U) * OLED_WIDTH] &= (uint8_t)~(1U << (y & 7U));
}

/* Tạo hình test (khung viền + 2 đường chéo + ô bàn cờ ở giữa) */
static void OLED_DrawTestPattern(void)
{
    uint8_t x, y;

    memset(g_oled_vram, 0x00, sizeof(g_oled_vram));

    /* 1. Vẽ khung viền ngoài màn hình 128x64 */
    for (x = 0U; x < OLED_WIDTH; x++)
    {
        OLED_DrawPixel(x, 0U, 1U);
        OLED_DrawPixel(x, OLED_HEIGHT - 1U, 1U);
    }
    for (y = 0U; y < OLED_HEIGHT; y++)
    {
        OLED_DrawPixel(0U, y, 1U);
        OLED_DrawPixel(OLED_WIDTH - 1U, y, 1U);
    }

    /* 2. Vẽ 2 đường chéo cắt nhau */
    for (x = 0U; x < OLED_WIDTH; x++)
    {
        OLED_DrawPixel(x, (uint8_t)((x * OLED_HEIGHT) / OLED_WIDTH), 1U);
        OLED_DrawPixel(x, (uint8_t)((OLED_HEIGHT - 1U) - (x * OLED_HEIGHT) / OLED_WIDTH), 1U);
    }

    /* 3. Vẽ khối bàn cờ ở chính giữa màn hình */
    for (y = 20U; y < 44U; y++)
    {
        for (x = 44U; x < 84U; x++)
        {
            if ((((x / 4U) + (y / 4U)) & 1U) == 0U)
            {
                OLED_DrawPixel(x, y, 1U);
            }
        }
    }
}

/* Tạo độ trễ đơn giản bằng vòng lặp để test đảo màu màn hình */
static void Delay_Loop(volatile uint32_t count)
{
    while (count--)
    {
        __NOP();
    }
}

int main(void)
{
    /* 1. Cấu hình I2C1 (PB6 = SCL, PB7 = SDA) ở tốc độ Fast Mode 400kHz */
    g_i2c1.pI2Cx = I2C1;
    g_i2c1.I2C_Config.I2C_SCLSpeed    = I2C_SCL_SPEED_FM;
    g_i2c1.I2C_Config.I2C_ACKControl  = I2C_ACK_ENABLE;
    g_i2c1.I2C_Config.I2C_FMDutyCycle = I2C_FM_DUTY_2;

    g_init_status = I2C_Init(&g_i2c1);

    /* 2. Quét toàn bộ bus I2C để kiểm tra xem có thấy 0x3C của OLED không */
    if (g_init_status == I2C_STATUS_OK)
    {
        (void)I2C_Scan(&g_i2c1, g_found_addrs, sizeof(g_found_addrs),
                       &g_found_count, I2C_TIMEOUT_DEFAULT_MS);

        /* 3. Kiểm tra riêng địa chỉ OLED 0x3C và khởi tạo màn hình */
        g_oled_status = I2C_IsDeviceReady(&g_i2c1, OLED_I2C_ADDR, I2C_TIMEOUT_DEFAULT_MS);
        if (g_oled_status == I2C_STATUS_OK)
        {
            g_oled_status = OLED_Init();
        }

        /* 4. Nếu khởi tạo thành công, vẽ hình test và đẩy lên OLED */
        if (g_oled_status == I2C_STATUS_OK)
        {
            OLED_DrawTestPattern();
            g_oled_status = OLED_UpdateScreen();
        }
    }

    while (1)
    {
        /* Nếu OLED bị rút dây hoặc lỗi NACK/Timeout thì thử phục hồi bus và khởi tạo lại */
        if (g_oled_status != I2C_STATUS_OK)
        {
            (void)I2C_BusRecover(&g_i2c1);
            if (I2C_IsDeviceReady(&g_i2c1, OLED_I2C_ADDR, I2C_TIMEOUT_DEFAULT_MS) == I2C_STATUS_OK)
            {
                if (OLED_Init() == I2C_STATUS_OK)
                {
                    OLED_DrawTestPattern();
                    g_oled_status = OLED_UpdateScreen();
                }
            }
            Delay_Loop(1000000UL);
        }
        else
        {
            /* Đảo màu hiển thị (Normal <-> Inverse) định kỳ để kiểm tra truyền lệnh liên tục */
            Delay_Loop(4000000UL);
            g_oled_status = OLED_WriteCommand(0xA7U); /* Inverse Display */
            if (g_oled_status != I2C_STATUS_OK) continue;

            Delay_Loop(4000000UL);
            g_oled_status = OLED_WriteCommand(0xA6U); /* Normal Display */
        }
    }
}
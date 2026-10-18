#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include "i2c_driver.h"

// Địa chỉ 7 bit, chưa dịch trái
#define OLED_I2C_ADDRESS  0x3CU
#define OLED_TIMEOUT_MS   100U

// Kết quả điều khiển màn hình
typedef enum
{
    OLED_OK = 0,
    OLED_ERROR,
    OLED_TIMEOUT,
    OLED_NOT_READY,
    OLED_INVALID_ARGUMENT
} OLED_Status;

// Khởi tạo SSD1306 bằng bus I2C đã được cấu hình
OLED_Status OLED_Init(I2C_Handle_t *i2c_handle);

// Gửi bộ đệm ảnh lên SSD1306
OLED_Status OLED_Update(void);

#endif

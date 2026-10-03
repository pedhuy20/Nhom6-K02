#ifndef OLED_TESTS_H
#define OLED_TESTS_H

#include <stdint.h>

// Kết quả kiểm tra: 1 là đạt, 0 là lỗi
extern volatile uint8_t g_oled_clear_ok;
extern volatile uint8_t g_oled_fill_ok;
extern volatile uint8_t g_oled_pixel_ok;

// Kiểm tra xóa bộ đệm
uint8_t OLED_TestClear(void);

// Kiểm tra tô màu bộ đệm
uint8_t OLED_TestFill(void);

// Kiểm tra vẽ điểm ảnh
uint8_t OLED_TestDrawPixel(void);

// Chạy tất cả bài kiểm tra
void OLED_TestAll(void);

#endif
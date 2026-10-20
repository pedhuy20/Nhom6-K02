/*
 * K02 - SV4: TICH HOP VA KIEM THU BAN THANH GHI
 * STM32F103C8T6 / Keil Arm Compiler 6 / C99.
 * HSI 8 MHz, I2C1 100 kHz, PB6=SCL, PB7=SDA.
 * SHT30=0x44, BH1750=0x23, SSD1306 128x64=0x3C (dia chi 7 bit).
 *
 * CAN THEM VAO TARGET:
 *   rcc.c, gpio.c, i2c_driver.c, sht30.c, bh1750.c,
 *   oled_ssd1306.c, oled_graphics.c, oled_fonts.c, oled_tests.c,
 *   startup va system_stm32f10x.c phu hop voi chip.
 * Them thu muc header vao Include Paths. Chi mot file duoc dinh nghia main().
 * Dung i2c_driver.c SV1 da gui, khop i2c_driver.h. Cac vong cho cua driver
 * dung DWT cho timeout; main khong the sua mot driver lap vo han tu ben ngoai.
 *
 * KHONG include rcc.h/gpio.h o day: hai header tu viet dung lai cac ten
 * RCC_TypeDef/GPIO_TypeDef da co trong stm32f10x.h. Chi khai bao ba ham
 * can dung ben duoi; khong doi file cua SV1 va khong tao them ban driver.
 *
 * DEBUG / WATCH (chay board, khong phai so lieu gia lap de bao cao):
 *   g_sv4                 : gia tri, loi, thoi gian, ket qua quet va test.
 *   g_sv4_ms              : thoi gian ms; phai tang lien tuc khi Run.
 *   g_sv4_page = 255       : tu doi trang moi 4 s; 0..3: co dinh trang.
 *   g_sv4_command = 1      : quet lai 0x08..0x77, moi lan mot dia chi.
 *   g_sv4_command = 2      : yeu cau phuc hoi bus qua driver SV1.
 *   g_sv4_command = 4      : do 10 lan OLED_Update voi cung mot anh test.
 *   g_sv4_command = 8      : chay lai test framebuffer va CRC trong RAM.
 * Co the OR cac lenh. Ung dung xoa lenh sau khi nhan.
 *
 * THU NGHIEM SV4:
 * 1. Quet thay ba dia chi; ten tren trang quet la ANH XA CAU HINH,
 *    ACK tai mot dia chi khong tu chung minh danh tinh linh kien.
 * 2. Rut BH1750/SHT30: dong du lieu bao ERROR, cam bien con lai van doc.
 *    Cam lai: tu khoi tao/doc lai. LED PC13 nhay nhanh khi co loi.
 * 3. Bus bi giu thap: phuc hoi toi da mot lan/s, ghi ket qua trong g_sv4.
 *    Chi phuc hoi duoc khi nguyen nhan vat ly cho phep day duoc nha.
 * 4. Logic Analyzer: SCL=PB6, SDA=PB7, GND chung; giai ma I2C 7 bit.
 *    SHT30: write 24 00, STOP, doi >=20 ms, read 6 byte + CRC.
 *    BH1750: write 01, 07, 10 (cac giao dich rieng), read 2 byte.
 *    OLED: cac lenh co control=00, du lieu co control=40, chunk=32 byte
 *    theo driver SV3. Dung lenh 4 de thu nhieu khung cung noi dung.
 * 5. g_sv4.bench_* la thoi gian OLED_Update, KHONG gom ve framebuffer.
 *    Ban nay KHONG dung ngat SysTick/I2C; ghi ro dieu kien do khi so voi HAL.
 *    So Flash/RAM lay tu Program Size/Map cua Keil, main khong tu do duoc.
 *    Flash thuong = Code+RO-data+RW-data; RAM = RW-data+ZI-data.
 *    Anh Logic Analyzer, rut/cam cam bien, bang so lieu va bao cao van
 *    can duoc thuc hien/thu thap tren board; khong tu danh dau da dat.
 *
 * DWT cung cap ca moc ms va phep do us; khong can them SysTick_Handler.
 * Chu y driver SV1 gui kem: OAR1 bit 14 chua duoc dat trong I2C_Init;
 * Bus_Init ben duoi bo sung bit nay. Nhan >=3 byte khoa ngat sau lan doc
 * N-2: can SV1 sua vung bao ve neu them ngat trong tuong lai. Main hien tai
 * tat SysTick va khong bat nguon ngat ngoai vi nao, de tranh chen ngat nen.
 * Khong reset DWT->CYCCNT khi dang chay; Now() phai duoc goi it nhat mot
 * lan trong moi chu ky tran DWT (~537 s o 8 MHz). Cac task hien tai ngan hon.
 */

#include "i2c_driver.h"
#include "sht30.h"
#include "bh1750.h"
#include "oled_ssd1306.h"
#include "oled_graphics.h"
#include "oled_tests.h"
#include <stdio.h>

extern unsigned char RCC_InitHSI8MHz(void);
extern void RCC_EnableI2C1Clock(void);
extern void GPIO_I2C_Init(void);

#ifndef SV4_ENABLE_PC13_LED
#define SV4_ENABLE_PC13_LED        1
#endif
#ifndef SV4_RUN_MEMORY_TESTS
#define SV4_RUN_MEMORY_TESTS       1
#endif

#define SV4_CLOCK_HZ               8000000UL
#define SV4_SENSOR_PERIOD_MS       1000UL
#define SV4_SHT_WAIT_MS            20UL
#define SV4_BH_FIRST_WAIT_MS        200UL
#define SV4_DISPLAY_PERIOD_MS      500UL
#define SV4_RETRY_MS               1000UL
#define SV4_SCAN_PERIOD_MS         15000UL
#define SV4_PROBE_TIMEOUT_MS       3UL
#define SV4_MAX_DEVICES            16U
#define SV4_HISTORY_COUNT          64U
#define SV4_GRAPH_MAX_LUX          1000
#define SV4_BENCH_FRAMES           10U
#define SV4_CMD_SCAN               1UL
#define SV4_CMD_RECOVER            2UL
#define SV4_CMD_BENCH              4UL
#define SV4_CMD_MEMORY_TEST        8UL

typedef struct
{
    uint8_t valid;             /* Co du lieu moi hop le / OLED dang san sang. */
    uint8_t ever_ok;
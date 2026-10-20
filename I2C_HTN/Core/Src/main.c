
#include "i2c_driver.h"
#include "sht30.h"
#include "bh1750.h"
#include "oled_sh1106.h"
#include "oled_graphics.h"
#include <stdio.h>
#include <stdarg.h>

/* rcc.h/gpio.h cua SV1 trung ten kieu CMSIS: chi khai bao API can goi. */
extern unsigned char RCC_InitHSI8MHz(void);
extern void RCC_EnableGPIOBClock(void);
extern unsigned char GPIO_SCL_Read(void);
extern unsigned char GPIO_SDA_Read(void);

#define SV4_CLOCK_HZ          8000000UL
#define SENSOR_PERIOD_MS      1000UL
#define DISPLAY_PERIOD_MS     250UL
#define RETRY_MS              1000UL
#define BUTTON_DEBOUNCE_MS    30UL
#define SCAN_CAPACITY         112U       /* 0x08..0x77 */

typedef struct
{
    uint8_t valid;
    uint32_t last_status;
} SV4_Device;

typedef struct
{
    uint32_t fatal_code, loop_count;
    uint32_t recover_attempts, recover_ok, oled_last_us;
    I2C_Status_t bus_status, scan_status;
    SV4_Device sht30, bh1750, oled;
    float temperature_c, humidity_rh, light_lux;
} SV4_Diagnostics;

volatile SV4_Diagnostics g_sv4;
volatile uint8_t g_sv4_page = 1U;
volatile uint32_t g_sv4_ms;
I2C_Handle_t g_i2c1;

static uint32_t last_cycles, recover_due, sht_due, bh_due, oled_due;
static uint8_t bus_ready, sht_pending, bh_configured, frame_valid;
static uint8_t scan_active = 1U, scan_count, scan_addresses[SCAN_CAPACITY];
static uint8_t button_raw = 1U, button_stable = 1U;
static uint32_t button_changed_at;

/* Giu phan le trong last_cycles; phep tru unsigned xu ly CYCCNT quay vong. */
static uint32_t Now(void)
{
    uint32_t ms = (uint32_t)(DWT->CYCCNT - last_cycles) /
                  (SV4_CLOCK_HZ / 1000UL);
    last_cycles += ms * (SV4_CLOCK_HZ / 1000UL);
    g_sv4_ms += ms;
    return g_sv4_ms;
}

static uint8_t Due(uint32_t now, uint32_t deadline)
{
    return (uint8_t)((uint32_t)(now - deadline) < 0x80000000UL);
}

static void Time_Init(void)
{
    uint32_t before, i;
    SysTick->CTRL = 0UL;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    before = DWT->CYCCNT;
    for (i = 0U; i < 32U; i++) { __NOP(); }
    if (DWT->CYCCNT == before) g_sv4.fatal_code = 3U;
    last_cycles = DWT->CYCCNT;
}

static void Button_Init(void)
{
    RCC_EnableGPIOBClock();
    GPIOB->BSRR = 1UL;
    GPIOB->CRL = (GPIOB->CRL & ~0xFUL) | 0x8UL; /* PB0 input pull-up. */
    button_changed_at = Now();
}

static void Button_Task(uint32_t now)
{
    uint8_t level = (uint8_t)((GPIOB->IDR & 1UL) != 0UL);
    if (level != button_raw)
    {
        button_raw = level;
        button_changed_at = now;
    }
    if (button_raw == button_stable ||
        (uint32_t)(now - button_changed_at) < BUTTON_DEBOUNCE_MS) return;
    button_stable = button_raw;
    if (button_stable == 0U)
    {
        g_sv4_page = (g_sv4_page == 3U) ? 1U : (uint8_t)(g_sv4_page + 1U);
        if (g_sv4_page == 1U) scan_active = 1U;
        oled_due = now;
    }
}

/* Chi kiem tra giua cac giao dich: SV1 doc chan, main doc them co BUSY. */
static uint8_t BusIsStuck(void)
{
    return (uint8_t)(GPIO_SCL_Read() == 0U || GPIO_SDA_Read() == 0U ||
                     (I2C1->SR2 & I2C_SR2_BUSY) != 0U);
}

static uint8_t Bus_Task(uint32_t now)
{
    if (bus_ready != 0U && BusIsStuck() == 0U) return 1U;

    g_sv4.sht30.valid = g_sv4.bh1750.valid = g_sv4.oled.valid = 0U;
    g_sv4.sht30.last_status = SHT30_LOI_I2C;
    g_sv4.bh1750.last_status = BH1750_LOI_I2C;
    g_sv4.oled.last_status = OLED_ERROR;
    sht_pending = bh_configured = 0U;
    if (bus_ready != 0U) g_sv4.bus_status = I2C_STATUS_BUSY;
    bus_ready = 0U;
    if (Due(now, recover_due) == 0U) return 0U;

    g_sv4.recover_attempts++;
    /* SV1 da khoi tao lai I2C ben trong ham nay; khong goi Init lap. */
    g_sv4.bus_status = I2C_BusRecover(&g_i2c1);
    recover_due = Now() + RETRY_MS;
    if (g_sv4.bus_status != I2C_STATUS_OK) return 0U;
    g_sv4.recover_ok++;
    bus_ready = 1U;
    sht_due = bh_due = oled_due = Now();
    return 1U;
}

static void SHT_Task(uint32_t now)
{
    SHT30_TrangThai status;
    SHT30_DuLieu data;
    if (Due(now, sht_due) == 0U) return;
    if (sht_pending == 0U)
    {
        status = SHT30_BatDauDo(&g_i2c1);
        sht_pending = (uint8_t)(status == SHT30_THANH_CONG);
        sht_due = Now() + (sht_pending ? 20UL : SENSOR_PERIOD_MS);
        if (sht_pending != 0U) return;
    }
    else
    {
        status = SHT30_DocDuLieu(&g_i2c1, &data); /* SV2 xu ly CRC va doi don vi. */
        sht_pending = 0U;
        sht_due = Now() + SENSOR_PERIOD_MS;
        if (status == SHT30_THANH_CONG)
        {
            g_sv4.temperature_c = data.nhiet_do_c;
            g_sv4.humidity_rh = data.do_am_rh;
        }
    }
    g_sv4.sht30.last_status = status;
    g_sv4.sht30.valid = (uint8_t)(status == SHT30_THANH_CONG);
}

static void BH_Task(uint32_t now)
{
    BH1750_TrangThai status;
    float lux;
    if (Due(now, bh_due) == 0U) return;
    if (bh_configured == 0U)
    {
        status = BH1750_KhoiTao(&g_i2c1);
        bh_configured = (uint8_t)(status == BH1750_THANH_CONG);
        bh_due = Now() + (bh_configured ? 200UL : RETRY_MS);
        if (bh_configured != 0U) return;
    }
    else
    {
        status = BH1750_DocLux(&g_i2c1, &lux);
        if (status == BH1750_THANH_CONG) g_sv4.light_lux = lux;
        else bh_configured = 0U;
        bh_due = Now() + SENSOR_PERIOD_MS;
    }
    g_sv4.bh1750.last_status = status;
    g_sv4.bh1750.valid = (uint8_t)(status == BH1750_THANH_CONG);
}

/* Gom snprintf va ham ve SV3 de cac dong giao dien ngan, gioi han 21 ky tu. */
static uint16_t Text(uint8_t row, const char *format, ...)
{
    char line[22];
    va_list args;
    va_start(args, format);
    (void)vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    return OLED_DrawText(0, (int16_t)(row * 8U), line, &OLED_FONT_5X7, OLED_WHITE);
}

/* Khong can bat printf float cua Keil; chi goi mot lan trong moi Text(). */
static const char *Number(float value)
{
    static char out[16];
    int32_t n = (int32_t)(value * 10.0f + (value < 0.0f ? -0.5f : 0.5f));
    uint32_t magnitude = (n < 0) ? (uint32_t)(-n) : (uint32_t)n;
    (void)snprintf(out, sizeof(out), "%s%lu.%lu", n < 0 ? "-" : "",
                   (unsigned long)(magnitude / 10U),
                   (unsigned long)(magnitude % 10U));
    return out;
}

static void DrawPage(uint32_t now)
{
    uint8_t i, index, group, groups;
    const char *name;
    int16_t x;
    if (g_sv4_page == 1U)
    {
        Text(0U, "1/3 I2C ADDRESSES");
        Text(1U, "Found:%u (7-bit)", (unsigned int)scan_count);
        groups = (uint8_t)((scan_count + 3U) / 4U);
        group = groups > 1U ? (uint8_t)((now / 2500UL) % groups) : 0U;
        for (i = 0U; i < 4U; i++)
        {
            index = (uint8_t)(group * 4U + i);
            if (index >= scan_count) break;
            name = scan_addresses[index] == SHT30_DIA_CHI ? "SHT30" :
                   scan_addresses[index] == BH1750_DIA_CHI ? "BH1750" :
                   scan_addresses[index] == OLED_I2C_ADDRESS ? "OLED SH1106" : "Unknown";
            Text((uint8_t)(i + 2U), "%s: 0x%02X", name,
                 (unsigned int)scan_addresses[index]);
        }
        if (scan_count == 0U) Text(2U, "No device found");
        if (groups > 1U) Text(7U, "List:%u/%u", (unsigned int)(group + 1U),
                              (unsigned int)groups);
    }
    else if (g_sv4_page == 2U)
    {
        Text(0U, "2/3 SENSOR VALUES");
        Text(1U, "SHT30 0x%02X %s", (unsigned int)SHT30_DIA_CHI,
             g_sv4.sht30.valid ? "OK" : "ERROR");
        if (g_sv4.sht30.valid != 0U)
        {
            x = (int16_t)((Text(2U, "T: %s", Number(g_sv4.temperature_c)) + 1U) * 6U);
            OLED_DrawRect(x, 16, (int16_t)(x + 2), 18, OLED_WHITE); /* Dau do. */
            (void)OLED_DrawChar((int16_t)(x + 4), 16, 'C', &OLED_FONT_5X7, OLED_WHITE);
            Text(3U, "RH: %s %%RH", Number(g_sv4.humidity_rh));
        }
        else { Text(2U, "T: ERROR"); Text(3U, "RH: ERROR"); }
        Text(4U, "BH1750 0x%02X %s", (unsigned int)BH1750_DIA_CHI,
             g_sv4.bh1750.valid ? "OK" : "ERROR");
        if (g_sv4.bh1750.valid != 0U) Text(5U, "Light: %s lux", Number(g_sv4.light_lux));
        else Text(5U, "Light: ERROR");
        Text(6U, "OLED SH1106:0x%02X", (unsigned int)OLED_I2C_ADDRESS);
    }
    else
    {
        Text(0U, "3/3 8MHz I2C100kHz");
        if (frame_valid != 0U)
            Text(2U, "Frame:%lu.%lu ms", (unsigned long)(g_sv4.oled_last_us / 1000UL),
                 (unsigned long)((g_sv4.oled_last_us / 100UL) % 10UL));
        else Text(2U, "Frame:-- ms");
        Text(4U, bus_ready ? "Bus:OK" : "Bus:ERROR");
        Text(5U, "Rec:%lu/%lu", (unsigned long)g_sv4.recover_ok,
             (unsigned long)g_sv4.recover_attempts);
    }
}

static void OLED_Task(uint32_t now)
{
    OLED_Status status;
    uint32_t start, elapsed;
    if (Due(now, oled_due) == 0U) return;
    if (g_sv4.oled.valid == 0U)
    {
        status = OLED_Init(&g_i2c1);
        oled_due = Now() + (status == OLED_OK ? 0UL : RETRY_MS);
    }
    else
    {
        OLED_Clear();
        DrawPage(now);
        start = DWT->CYCCNT;
        status = OLED_Update();
        elapsed = DWT->CYCCNT - start; /* Chi do Update, khong gom ve/thong ke. */
        if (status == OLED_OK)
        {
            g_sv4.oled_last_us = elapsed / (SV4_CLOCK_HZ / 1000000UL);
            frame_valid = 1U;
        }
        oled_due = Now() + (status == OLED_OK ? DISPLAY_PERIOD_MS : RETRY_MS);
    }
    g_sv4.oled.last_status = status;
    g_sv4.oled.valid = (uint8_t)(status == OLED_OK);
}

static void App_Task(void)
{
    uint32_t now = Now();
    g_sv4.loop_count++;
    Button_Task(now);
    if (Bus_Task(now) == 0U) return;
    SHT_Task(Now());
    if (sht_pending != 0U || BusIsStuck() != 0U) return;
    BH_Task(Now());
    if (BusIsStuck() != 0U) return;
    if (scan_active != 0U)
    {
        g_sv4.scan_status = I2C_Scan(&g_i2c1, scan_addresses, SCAN_CAPACITY,
                                     &scan_count, 3UL);
        if (g_sv4.scan_status != I2C_STATUS_OK) { bus_ready = 0U; return; }
        scan_active = 0U;
    }
    OLED_Task(Now());
}

int main(void)
{
    if (RCC_InitHSI8MHz() == 0U) g_sv4.fatal_code = 1U;
    SystemCoreClockUpdate();
    if (SystemCoreClock != SV4_CLOCK_HZ) g_sv4.fatal_code = 1U;
    if (g_sv4.fatal_code == 0U) Time_Init();
    if (g_sv4.fatal_code != 0U)
        while (1) { __NOP(); } /* Chi loi khoi dong clock/DWT. */
    Button_Init();
    g_i2c1.pI2Cx = I2C1;
    g_i2c1.I2C_Config.I2C_SCLSpeed = I2C_SCL_SPEED_SM;
    g_i2c1.I2C_Config.I2C_ACKControl = I2C_ACK_ENABLE;
    g_i2c1.I2C_Config.I2C_FMDutyCycle = I2C_FM_DUTY_2;
    /* I2C_Init SV1 da cap clock va cau hinh chan I2C1 PB6/PB7 mac dinh. */
    g_sv4.bus_status = I2C_Init(&g_i2c1);
    bus_ready = (uint8_t)(g_sv4.bus_status == I2C_STATUS_OK);
    sht_due = bh_due = oled_due = recover_due = Now() + 50UL;
    while (1) App_Task();
}
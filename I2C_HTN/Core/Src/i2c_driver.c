/*

 * Hỗ trợ: I2C1 (PB6/PB7 hoặc ánh xạ lại qua AFIO sang PB8/PB9),
 * I2C2 (PB10/PB11), Master địa chỉ 7 bit với cơ chế thăm dò cờ (polling),
 * chế độ Standard/Fast, truyền/nhận N byte, Repeated START,
 * kiểm tra ACK/quét địa chỉ, xử lý NACK/lỗi/Timeout và khôi phục bus bằng GPIO.
 * Điều kiện sử dụng: SCL/SDA có điện trở kéo lên; không truy cập bus đồng thời;
 */

#include "i2c_driver.h"
#include <stddef.h>
#include <stdint.h>
#define I2C_ERROR_BITS (I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_AF | \
                        I2C_SR1_OVR | I2C_SR1_TIMEOUT)

#define I2C_GPIO_AF_OD_50MHZ  0xFU
#define I2C_GPIO_GP_OD_50MHZ  0x7U
#define I2C_RECOVERY_WAIT_MS  2U
#define I2C_RECOVERY_PULSE_US 10U

static uint32_t s_cpu_cycles_per_ms = 0U;
static uint32_t s_cpu_cycles_per_us = 0U;

typedef struct
{
    GPIO_TypeDef *port;
    uint8_t scl;
    uint8_t sda;
} I2C_Pins_t;


/* Private : Quản lý Timeout bằng bộ đếm chu kỳ (an toàn khi tràn nếu khoảng chờ < 2^31 chu kỳ)
 */

static uint32_t I2C_Now(void)
{
    return DWT->CYCCNT;
}

static uint8_t I2C_TimeoutValid(uint32_t timeout_ms)
{
    if ((timeout_ms == 0U) || (s_cpu_cycles_per_ms == 0U))
        return 0U;

    return ((uint64_t)timeout_ms * s_cpu_cycles_per_ms <= 0x7FFFFFFFULL)
               ? 1U : 0U;
}

static uint8_t I2C_Expired(uint32_t start, uint32_t timeout_ms)
{
    uint32_t limit = timeout_ms * s_cpu_cycles_per_ms;
    return ((uint32_t)(I2C_Now() - start) >= limit) ? 1U : 0U;
}

static void I2C_DelayUs(uint32_t microseconds)
{
    uint32_t start = I2C_Now();
    uint32_t cycles = microseconds * s_cpu_cycles_per_us;
    while ((uint32_t)(I2C_Now() - start) < cycles) { }
}


/* Private : Ánh xạ chân và cấu hình chế độ GPIO */

static uint8_t I2C_GetPins(I2C_TypeDef *i2c, I2C_Pins_t *pins)
{
    if ((pins == NULL) || (i2c == NULL))
        return 0U;

    pins->port = GPIOB;
    if (i2c == I2C1)
    {
        if ((AFIO->MAPR & AFIO_MAPR_I2C1_REMAP) != 0U)
        {
            pins->scl = 8U;
            pins->sda = 9U;
        }
        else
        {
            pins->scl = 6U;
            pins->sda = 7U;
        }
    }
    else if (i2c == I2C2)
    {
        pins->scl = 10U;
        pins->sda = 11U;
    }
    else
    {
        return 0U;
    }
    return 1U;
}

static void I2C_PinMode(GPIO_TypeDef *gpio, uint8_t pin, uint32_t nibble)
{
    volatile uint32_t *cr = (pin < 8U) ? &gpio->CRL : &gpio->CRH;
    uint32_t shift = ((uint32_t)pin & 7U) * 4U;
    *cr = (*cr & ~(0xFUL << shift)) | (nibble << shift);
}

static void I2C_PinsToAFOD(const I2C_Pins_t *p)
{
    uint32_t mask = (1UL << p->scl) | (1UL << p->sda);
    p->port->BSRR = mask; /* Nhả các đường tín hiệu trước; cần điện trở kéo lên bên ngoài */
    I2C_PinMode(p->port, p->scl, I2C_GPIO_AF_OD_50MHZ);
    I2C_PinMode(p->port, p->sda, I2C_GPIO_AF_OD_50MHZ);
}

static uint8_t I2C_PinHigh(const I2C_Pins_t *p, uint8_t pin)
{
    return ((p->port->IDR & (1UL << pin)) != 0U) ? 1U : 0U;
}

static void I2C_PinRelease(const I2C_Pins_t *p, uint8_t pin)
{
    p->port->BSRR = (1UL << pin);
}

static void I2C_PinPullLow(const I2C_Pins_t *p, uint8_t pin)
{
    p->port->BRR = (1UL << pin);
}


/* Private : Kiểm tra lỗi SR1, chờ cờ, tạo STOP và khôi phục ACK */

static I2C_Status_t I2C_ErrorFromSR1(uint32_t sr1)
{
    if ((sr1 & I2C_SR1_ARLO) != 0U) return I2C_STATUS_ARBITRATION_LOST;
    if ((sr1 & I2C_SR1_BERR) != 0U) return I2C_STATUS_BUS_ERROR;
    if ((sr1 & I2C_SR1_AF) != 0U)   return I2C_STATUS_NACK;
    if ((sr1 & I2C_SR1_OVR) != 0U)  return I2C_STATUS_OVERRUN;
    if ((sr1 & I2C_SR1_TIMEOUT) != 0U) return I2C_STATUS_TIMEOUT;
    return I2C_STATUS_OK;
}

static void I2C_ClearErrorFlags(I2C_TypeDef *i2c)
{
    /* Các cờ lỗi trong SR1 của STM32F1 được xóa bằng cách ghi 0 vào các bit tương ứng. */
    i2c->SR1 &= ~I2C_ERROR_BITS;
}

static I2C_Status_t I2C_WaitSR1(I2C_TypeDef *i2c, uint32_t mask,
                                uint32_t started, uint32_t timeout_ms)
{
    for (;;)
    {
        uint32_t sr1 = i2c->SR1;
        I2C_Status_t err = I2C_ErrorFromSR1(sr1);
        if (err != I2C_STATUS_OK) return err;
        if ((sr1 & mask) == mask) return I2C_STATUS_OK;
        if (I2C_Expired(started, timeout_ms)) return I2C_STATUS_TIMEOUT;
    }
}

static I2C_Status_t I2C_WaitIdle(I2C_TypeDef *i2c, uint32_t started,
                                 uint32_t timeout_ms)
{
    while ((i2c->SR2 & I2C_SR2_BUSY) != 0U)
    {
        if (I2C_Expired(started, timeout_ms)) return I2C_STATUS_BUSY;
    }
    return I2C_STATUS_OK;
}

static I2C_Status_t I2C_WaitSTOP(I2C_TypeDef *i2c, uint32_t started,
                                 uint32_t timeout_ms)
{
    while (((i2c->CR1 & I2C_CR1_STOP) != 0U) ||
           ((i2c->SR2 & I2C_SR2_BUSY) != 0U))
    {
        I2C_Status_t err = I2C_ErrorFromSR1(i2c->SR1);
        if (err != I2C_STATUS_OK) return err;
        if (I2C_Expired(started, timeout_ms)) return I2C_STATUS_TIMEOUT;
    }
    return I2C_STATUS_OK;
}

static void I2C_ClearADDR(I2C_TypeDef *i2c)
{
    volatile uint32_t dummy;
    dummy = i2c->SR1;
    dummy = i2c->SR2;
    (void)dummy;
}

static void I2C_RestoreACK(I2C_Handle_t *h)
{
    I2C_TypeDef *i2c = h->pI2Cx;
    i2c->CR1 &= ~I2C_CR1_POS;
    if (h->I2C_Config.I2C_ACKControl == I2C_ACK_ENABLE)
        i2c->CR1 |= I2C_CR1_ACK;
    else
        i2c->CR1 &= ~I2C_CR1_ACK;
}

static void I2C_Abort(I2C_Handle_t *h, uint32_t started, uint32_t timeout_ms)
{
    I2C_TypeDef *i2c = h->pI2Cx;
    /* Khi xảy ra ARLO, ngoại vi mất quyền Master; không ép STOP nếu thiết bị khác đang giữ bus. */
    if ((i2c->SR2 & I2C_SR2_MSL) != 0U)
    {
        i2c->CR1 |= I2C_CR1_STOP;
    }
    I2C_ClearErrorFlags(i2c);
    /* Chỉ cố gắng dừng phiên truyền nhận trong giới hạn cho phép; không chờ vô hạn khi xử lý lỗi. */
    while (((i2c->CR1 & I2C_CR1_STOP) != 0U) &&
           !I2C_Expired(started, timeout_ms)) { }
    I2C_RestoreACK(h);
}

static uint8_t I2C_ValidHandle(I2C_Handle_t *h)
{
    if ((h == NULL) || (h->pI2Cx != I2C1 && h->pI2Cx != I2C2))
        return 0U;
    if ((s_cpu_cycles_per_ms == 0U) ||
        ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0U) ||
        ((h->pI2Cx->CR1 & I2C_CR1_PE) == 0U))
        return 0U;
    return 1U;
}

static uint8_t I2C_ValidAddress(uint8_t addr)
{
    /* Chỉ chấp nhận địa chỉ 7 bit không thuộc dải địa chỉ dành riêng. */
    return ((addr >= 0x08U) && (addr <= 0x77U)) ? 1U : 0U;
}


/* Private : Phát START và địa chỉ; giữ cờ ADDR để pha nhận kịp cấu hình ACK */

static I2C_Status_t I2C_StartAddress(I2C_TypeDef *i2c, uint8_t address,
                                     uint8_t direction, uint32_t started,
                                     uint32_t timeout_ms)
{
    I2C_Status_t s;
    i2c->CR1 |= I2C_CR1_START;
    s = I2C_WaitSR1(i2c, I2C_SR1_SB, started, timeout_ms);
    if (s != I2C_STATUS_OK) return s;

    /* Sau khi đọc SR1 để kiểm tra SB, ghi DR sẽ xóa cờ SB. */
    i2c->DR = (uint8_t)((address << 1U) | (direction & 1U));
    return I2C_WaitSR1(i2c, I2C_SR1_ADDR, started, timeout_ms);
}

/* Không tạo STOP tại đây; hàm WriteRead sẽ sử dụng Repeated START. */
static I2C_Status_t I2C_WriteBytes(I2C_TypeDef *i2c, const uint8_t *data,
                                   uint16_t size, uint32_t started,
                                   uint32_t timeout_ms)
{
    I2C_Status_t s;
    uint16_t i;
    if (size == 0U) return I2C_STATUS_OK;

    /* Sự kiện EV8_1: Cần ghi byte đầu tiên vào DR ngay sau khi xóa cờ ADDR. */
    i2c->DR = data[0];
    for (i = 1U; i < size; i++)
    {
        s = I2C_WaitSR1(i2c, I2C_SR1_TXE, started, timeout_ms);
        if (s != I2C_STATUS_OK) return s;
        i2c->DR = data[i];
    }
    return I2C_WaitSR1(i2c, I2C_SR1_BTF, started, timeout_ms);
}

/* Khi vào hàm, cờ ADDR phải còn được đặt; hàm này sẽ tạo STOP. */
static I2C_Status_t I2C_ReadBytes(I2C_TypeDef *i2c, uint8_t *data,
                                  uint16_t size, uint32_t started,
                                  uint32_t timeout_ms)
{
    I2C_Status_t s;
    uint16_t remaining = size;
    uint32_t primask;

    if (size == 1U)
    {
        /* Theo RM0008, EV6_3: Xóa ACK, xóa ADDR rồi tạo STOP liên tiếp, không để ngắt xen vào. */
        i2c->CR1 &= ~I2C_CR1_ACK;
        primask = __get_PRIMASK();
        __disable_irq();
        I2C_ClearADDR(i2c);
        i2c->CR1 |= I2C_CR1_STOP;
        __set_PRIMASK(primask);

        s = I2C_WaitSR1(i2c, I2C_SR1_RXNE, started, timeout_ms);
        if (s != I2C_STATUS_OK) return s;
        data[0] = (uint8_t)i2c->DR;
        return I2C_STATUS_OK;
    }

    if (size == 2U)
    {
        /* Phải thiết lập POS và ACK trước khi xóa cờ ADDR. */
        primask = __get_PRIMASK();
        __disable_irq();
        I2C_ClearADDR(i2c);
        i2c->CR1 &= ~I2C_CR1_ACK;
        __set_PRIMASK(primask);

        s = I2C_WaitSR1(i2c, I2C_SR1_BTF, started, timeout_ms);
        if (s != I2C_STATUS_OK) return s;

        primask = __get_PRIMASK();
        __disable_irq();
        i2c->CR1 |= I2C_CR1_STOP;
        data[0] = (uint8_t)i2c->DR;
        __set_PRIMASK(primask);
        data[1] = (uint8_t)i2c->DR;
        return I2C_STATUS_OK;
    }

    /* Khi nhận từ 3 byte: Chờ BTF để phần cứng kéo dài xung SCL an toàn nếu CPU xử lý chậm. */
    I2C_ClearADDR(i2c);
    while (remaining > 3U)
    {
        s = I2C_WaitSR1(i2c, I2C_SR1_BTF, started, timeout_ms);
        if (s != I2C_STATUS_OK) return s;
        *data++ = (uint8_t)i2c->DR;
        remaining--;
    }

    /* Lúc này còn đúng 3 byte; byte N-2 nằm trong DR, byte N-1 nằm trong thanh ghi dịch. */
    s = I2C_WaitSR1(i2c, I2C_SR1_BTF, started, timeout_ms);
    if (s != I2C_STATUS_OK) return s;

    i2c->CR1 &= ~I2C_CR1_ACK;
    *data++ = (uint8_t)i2c->DR; /* Đọc byte N-2 để bắt đầu dịch byte N */

    /* Tạo STOP, sau đó đọc byte N-1. */
    primask = __get_PRIMASK();
    __disable_irq();
    i2c->CR1 |= I2C_CR1_STOP;
    *data++ = (uint8_t)i2c->DR; /* Byte thứ N-1 */
    __set_PRIMASK(primask);

    s = I2C_WaitSR1(i2c, I2C_SR1_RXNE, started, timeout_ms);
    if (s != I2C_STATUS_OK) return s;
    *data = (uint8_t)i2c->DR; /* Byte thứ N */
    return I2C_STATUS_OK;
}

static void I2C_PrepareReceive(I2C_TypeDef *i2c, uint16_t size)
{
    i2c->CR1 |= I2C_CR1_ACK;     /* Tạm bật ACK để nhận nhiều byte, không phụ thuộc cấu hình mặc định. */
    i2c->CR1 &= ~I2C_CR1_POS;
    if (size == 2U) i2c->CR1 |= I2C_CR1_POS;
}


/* Public: Cấp xung nhịp và bật/tắt ngoại vi */

void I2C_PeriClockControl(I2C_TypeDef *pI2Cx, uint8_t EnorDi)
{
    uint32_t bit;
    if (pI2Cx == I2C1) bit = RCC_APB1ENR_I2C1EN;
    else if (pI2Cx == I2C2) bit = RCC_APB1ENR_I2C2EN;
    else return;

    if (EnorDi == I2C_ENABLE) RCC->APB1ENR |= bit;
    else RCC->APB1ENR &= ~bit;
}

void I2C_PeripheralControl(I2C_TypeDef *pI2Cx, uint8_t EnorDi)
{
    if (pI2Cx != I2C1 && pI2Cx != I2C2) return;
    if (EnorDi == I2C_ENABLE) pI2Cx->CR1 |= I2C_CR1_PE;
    else pI2Cx->CR1 &= ~I2C_CR1_PE;
}

void I2C_DeInit(I2C_TypeDef *pI2Cx)
{
    if (pI2Cx == I2C1)
    {
        RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
        RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;
    }
    else if (pI2Cx == I2C2)
    {
        RCC->APB1RSTR |= RCC_APB1RSTR_I2C2RST;
        RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C2RST;
    }
}


/* Public: Khởi tạo định thời I2C, GPIO và bộ đếm DWT */

I2C_Status_t I2C_Init(I2C_Handle_t *pI2CHandle)
{
    I2C_TypeDef *i2c;
    I2C_Pins_t pins;
    uint32_t pclk1_hz;
    uint32_t mhz;
    uint32_t ccr;
    uint32_t trise;
    uint32_t divider;
    uint32_t apb1_prescaler;
    uint32_t fast;
    uint32_t duty;

    if (pI2CHandle == NULL) return I2C_STATUS_INVALID_ARG;
    i2c = pI2CHandle->pI2Cx;
    if (i2c != I2C1 && i2c != I2C2) return I2C_STATUS_INVALID_ARG;

    if (pI2CHandle->I2C_Config.I2C_ACKControl > I2C_ACK_ENABLE ||
        pI2CHandle->I2C_Config.I2C_FMDutyCycle > I2C_FM_DUTY_16_9)
        return I2C_STATUS_INVALID_ARG;

    fast = (pI2CHandle->I2C_Config.I2C_SCLSpeed > 100000U) ? 1U : 0U;
    if (pI2CHandle->I2C_Config.I2C_SCLSpeed == 0U ||
        pI2CHandle->I2C_Config.I2C_SCLSpeed > 400000U)
        return I2C_STATUS_INVALID_ARG;

    /* Hàm CMSIS cập nhật tần số HCLK dựa trên cấu hình RCC hiện tại. */
    SystemCoreClockUpdate();
    if ((SystemCoreClock < 2000000U) || (SystemCoreClock / 1000U == 0U))
        return I2C_STATUS_INVALID_ARG;

    apb1_prescaler = (RCC->CFGR >> 8U) & 7U; /* Các bit PPRE1[2:0] quy định hệ số chia APB1 */
    divider = (apb1_prescaler < 4U) ? 1U : (1UL << (apb1_prescaler - 3U));
    pclk1_hz = SystemCoreClock / divider;
    if (pclk1_hz > 36000000U || pclk1_hz < (fast ? 4000000U : 2000000U))
        return I2C_STATUS_INVALID_ARG;
    if ((pclk1_hz % 1000000U) != 0U)
        return I2C_STATUS_INVALID_ARG; /* Trường CR2.FREQ yêu cầu tần số được xác định theo đơn vị MHz nguyên */
    mhz = pclk1_hz / 1000000U;

    duty = pI2CHandle->I2C_Config.I2C_FMDutyCycle;
    if (fast == 0U)
    {
        uint32_t denom = 2U * pI2CHandle->I2C_Config.I2C_SCLSpeed;
        ccr = (pclk1_hz + denom - 1U) / denom;
        if (ccr < 4U) ccr = 4U;
        trise = mhz + 1U;
    }
    else
    {
        uint32_t denom = (duty == I2C_FM_DUTY_2 ? 3U : 25U) *
                         pI2CHandle->I2C_Config.I2C_SCLSpeed;
        ccr = (pclk1_hz + denom - 1U) / denom;
        if (ccr < (duty == I2C_FM_DUTY_2 ? 4U : 1U))
            ccr = (duty == I2C_FM_DUTY_2 ? 4U : 1U);
        trise = ((mhz * 300U) / 1000U) + 1U;
    }
    if (ccr > 0x0FFFU || trise > 63U) return I2C_STATUS_INVALID_ARG;

    /* DWT->CYCCNT tăng theo HCLK, không chiếm tài nguyên SysTick hoặc TIM2. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0U)
    {
        DWT->CYCCNT = 0U;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
    s_cpu_cycles_per_ms = SystemCoreClock / 1000U;
    s_cpu_cycles_per_us = SystemCoreClock / 1000000U;

    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    I2C_PeriClockControl(i2c, I2C_ENABLE);
    if (I2C_GetPins(i2c, &pins) == 0U) return I2C_STATUS_INVALID_ARG;
    I2C_PinsToAFOD(&pins);

    i2c->CR1 &= ~I2C_CR1_PE;
    i2c->CR2 = (i2c->CR2 & ~(I2C_CR2_FREQ | I2C_CR2_ITERREN |
                             I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN |
                             I2C_CR2_DMAEN)) | mhz;

    i2c->CCR = ccr | (fast ? I2C_CCR_FS : 0U) |
               ((fast && duty == I2C_FM_DUTY_16_9) ? I2C_CCR_DUTY : 0U);
    i2c->TRISE = trise;
    i2c->CR1 &= ~(I2C_CR1_POS | I2C_CR1_SMBUS | I2C_CR1_NOSTRETCH);
    I2C_ClearErrorFlags(i2c);
    I2C_RestoreACK(pI2CHandle);
    i2c->CR1 |= I2C_CR1_PE;
    return I2C_STATUS_OK;
}


/* Public: Chỉ truyền dữ liệu (ghi) */

I2C_Status_t I2C_Master_Transmit(I2C_Handle_t *pI2CHandle,
                                 uint8_t DevAddress, const uint8_t *pData,
                                 uint16_t Size, uint32_t TimeoutMs)
{
    I2C_Status_t s;
    I2C_TypeDef *i2c;
    uint32_t started;

    if (!I2C_ValidHandle(pI2CHandle) || !I2C_ValidAddress(DevAddress) ||
        pData == NULL || Size == 0U || !I2C_TimeoutValid(TimeoutMs))
        return I2C_STATUS_INVALID_ARG;

    i2c = pI2CHandle->pI2Cx;
    started = I2C_Now();
    s = I2C_WaitIdle(i2c, started, TimeoutMs);
    if (s != I2C_STATUS_OK) return s;
    I2C_ClearErrorFlags(i2c);

    s = I2C_StartAddress(i2c, DevAddress, I2C_WRITE, started, TimeoutMs);
    if (s != I2C_STATUS_OK) goto error;
    I2C_ClearADDR(i2c);
    s = I2C_WriteBytes(i2c, pData, Size, started, TimeoutMs);
    if (s != I2C_STATUS_OK) goto error;

    i2c->CR1 |= I2C_CR1_STOP;
    s = I2C_WaitSTOP(i2c, started, TimeoutMs);
    if (s == I2C_STATUS_OK) return s;
error:
    I2C_Abort(pI2CHandle, started, TimeoutMs);
    return s;
}

/* ------------------------------------------------------------------------- */
/* Public: Chỉ nhận dữ liệu (đọc) */
/* ------------------------------------------------------------------------- */
I2C_Status_t I2C_Master_Receive(I2C_Handle_t *pI2CHandle,
                                uint8_t DevAddress, uint8_t *pData,
                                uint16_t Size, uint32_t TimeoutMs)
{
    I2C_Status_t s;
    I2C_TypeDef *i2c;
    uint32_t started;

    if (!I2C_ValidHandle(pI2CHandle) || !I2C_ValidAddress(DevAddress) ||
        pData == NULL || Size == 0U || !I2C_TimeoutValid(TimeoutMs))
        return I2C_STATUS_INVALID_ARG;
    i2c = pI2CHandle->pI2Cx;
    started = I2C_Now();
    s = I2C_WaitIdle(i2c, started, TimeoutMs);
    if (s != I2C_STATUS_OK) return s;
    I2C_ClearErrorFlags(i2c);

    I2C_PrepareReceive(i2c, Size);
    s = I2C_StartAddress(i2c, DevAddress, I2C_READ, started, TimeoutMs);
    if (s != I2C_STATUS_OK) goto error;
    s = I2C_ReadBytes(i2c, pData, Size, started, TimeoutMs);
    if (s != I2C_STATUS_OK) goto error;

    s = I2C_WaitSTOP(i2c, started, TimeoutMs);
    if (s != I2C_STATUS_OK) goto error;
    I2C_RestoreACK(pI2CHandle);
    return I2C_STATUS_OK;
error:
    I2C_Abort(pI2CHandle, started, TimeoutMs);
    return s;
}


/* Public: Ghi–đọc kết hợp, dùng Repeated START (không có STOP giữa hai pha) */

I2C_Status_t I2C_Master_WriteRead(I2C_Handle_t *pI2CHandle,
                                  uint8_t DevAddress,
                                  const uint8_t *pTxData, uint16_t TxSize,
                                  uint8_t *pRxData, uint16_t RxSize,
                                  uint32_t TimeoutMs)
{
    I2C_Status_t s;
    I2C_TypeDef *i2c;
    uint32_t started;

    if (!I2C_ValidHandle(pI2CHandle) || !I2C_ValidAddress(DevAddress) ||
        pTxData == NULL || pRxData == NULL || TxSize == 0U || RxSize == 0U ||
        !I2C_TimeoutValid(TimeoutMs))
        return I2C_STATUS_INVALID_ARG;

    i2c = pI2CHandle->pI2Cx;
    started = I2C_Now();
    s = I2C_WaitIdle(i2c, started, TimeoutMs);
    if (s != I2C_STATUS_OK) return s;
    I2C_ClearErrorFlags(i2c);

    s = I2C_StartAddress(i2c, DevAddress, I2C_WRITE, started, TimeoutMs);
    if (s != I2C_STATUS_OK) goto error;
    I2C_ClearADDR(i2c);
    s = I2C_WriteBytes(i2c, pTxData, TxSize, started, TimeoutMs);
    if (s != I2C_STATUS_OK) goto error;

    /* Tiếp tục giữ quyền Master; phát Repeated START thay vì STOP. */
    I2C_PrepareReceive(i2c, RxSize);
    s = I2C_StartAddress(i2c, DevAddress, I2C_READ, started, TimeoutMs);
    if (s != I2C_STATUS_OK) goto error;
    s = I2C_ReadBytes(i2c, pRxData, RxSize, started, TimeoutMs);
    if (s != I2C_STATUS_OK) goto error;

    s = I2C_WaitSTOP(i2c, started, TimeoutMs);
    if (s != I2C_STATUS_OK) goto error;
    I2C_RestoreACK(pI2CHandle);
    return I2C_STATUS_OK;
error:
    I2C_Abort(pI2CHandle, started, TimeoutMs);
    return s;
}


/* Public: Kiểm tra phản hồi ACK/NACK khi gửi địa chỉ */

I2C_Status_t I2C_IsDeviceReady(I2C_Handle_t *pI2CHandle,
                               uint8_t DevAddress, uint32_t TimeoutMs)
{
    I2C_Status_t s;
    I2C_TypeDef *i2c;
    uint32_t started;

    if (!I2C_ValidHandle(pI2CHandle) || !I2C_ValidAddress(DevAddress) ||
        !I2C_TimeoutValid(TimeoutMs))
        return I2C_STATUS_INVALID_ARG;
    i2c = pI2CHandle->pI2Cx;
    started = I2C_Now();
    s = I2C_WaitIdle(i2c, started, TimeoutMs);
    if (s != I2C_STATUS_OK) return s;
    I2C_ClearErrorFlags(i2c);

    s = I2C_StartAddress(i2c, DevAddress, I2C_WRITE, started, TimeoutMs);
    if (s != I2C_STATUS_OK) goto error;
    I2C_ClearADDR(i2c);
    i2c->CR1 |= I2C_CR1_STOP;
    s = I2C_WaitSTOP(i2c, started, TimeoutMs);
    if (s == I2C_STATUS_OK) return s;
error:
    I2C_Abort(pI2CHandle, started, TimeoutMs);
    return s;
}


/* Public: Quét các địa chỉ I2C 7 bit */

I2C_Status_t I2C_Scan(I2C_Handle_t *pI2CHandle, uint8_t *pAddresses,
                      uint8_t MaxDevices, uint8_t *pFoundCount,
                      uint32_t TimeoutMs)
{
    uint16_t addr;
    uint8_t found = 0U;
    I2C_Status_t s;

    if (!I2C_ValidHandle(pI2CHandle) || pAddresses == NULL ||
        pFoundCount == NULL || MaxDevices == 0U ||
        !I2C_TimeoutValid(TimeoutMs))
        return I2C_STATUS_INVALID_ARG;
    *pFoundCount = 0U;

    for (addr = 0x08U; addr <= 0x77U; addr++)
    {
        s = I2C_IsDeviceReady(pI2CHandle, (uint8_t)addr, TimeoutMs);
        if (s == I2C_STATUS_NACK) continue;
        if (s != I2C_STATUS_OK)
        {
            *pFoundCount = found;
            return s;
        }
        if (found == MaxDevices)
        {
            *pFoundCount = found;
            return I2C_STATUS_BUFFER_FULL;
        }
        pAddresses[found++] = (uint8_t)addr;
    }
    *pFoundCount = found;
    return I2C_STATUS_OK;
}


/* Public: Khôi phục bus bằng tối đa 9 xung SCL thủ công và một xung STOP */

I2C_Status_t I2C_BusRecover(I2C_Handle_t *pI2CHandle)
{
    I2C_Pins_t pins;
    I2C_TypeDef *i2c;
    uint32_t started;
    uint32_t n;
    uint8_t ok = 1U;
    I2C_Status_t init_status;

    if (!I2C_ValidHandle(pI2CHandle))
        return I2C_STATUS_RECOVERY_FAILED;

    i2c = pI2CHandle->pI2Cx;
    if (!I2C_GetPins(i2c, &pins)) return I2C_STATUS_RECOVERY_FAILED;

    i2c->CR1 &= ~I2C_CR1_PE;
    I2C_PinRelease(&pins, pins.scl);
    I2C_PinRelease(&pins, pins.sda);
    I2C_PinMode(pins.port, pins.scl, I2C_GPIO_GP_OD_50MHZ);
    I2C_PinMode(pins.port, pins.sda, I2C_GPIO_GP_OD_50MHZ);
    I2C_DelayUs(I2C_RECOVERY_PULSE_US);

    /* Nếu SCL bị thiết bị bên ngoài giữ ở mức thấp, không ép lên mức cao để tránh xung đột. */
    started = I2C_Now();
    while (!I2C_PinHigh(&pins, pins.scl))
    {
        if (I2C_Expired(started, I2C_RECOVERY_WAIT_MS))
        {
            ok = 0U;
            break;
        }
    }

    if (ok)
    {
        for (n = 0U; n < 9U && !I2C_PinHigh(&pins, pins.sda); n++)
        {
            I2C_PinPullLow(&pins, pins.scl);
            I2C_DelayUs(I2C_RECOVERY_PULSE_US);
            I2C_PinRelease(&pins, pins.scl);
            started = I2C_Now();
            while (!I2C_PinHigh(&pins, pins.scl))
            {
                if (I2C_Expired(started, I2C_RECOVERY_WAIT_MS))
                {
                    ok = 0U;
                    break;
                }
            }
            if (!ok) break;
            I2C_DelayUs(I2C_RECOVERY_PULSE_US);
        }
    }

    if (ok && I2C_PinHigh(&pins, pins.scl) && I2C_PinHigh(&pins, pins.sda))
    {
        /* Chủ động tạo các chuyển mức SDA/SCL còn giúp xử lý tình huống liên quan lỗi bộ lọc tương tự. */
        I2C_PinPullLow(&pins, pins.sda);
        I2C_DelayUs(I2C_RECOVERY_PULSE_US);
        I2C_PinPullLow(&pins, pins.scl);
        I2C_DelayUs(I2C_RECOVERY_PULSE_US);
        I2C_PinRelease(&pins, pins.scl);
        I2C_DelayUs(I2C_RECOVERY_PULSE_US);
        if (I2C_PinHigh(&pins, pins.scl))
        {
            I2C_PinRelease(&pins, pins.sda); /* Điều kiện STOP: SDA chuyển từ thấp lên cao khi SCL đang ở mức cao */
            I2C_DelayUs(I2C_RECOVERY_PULSE_US);
        }
        else ok = 0U;
    }
    else ok = 0U;

    I2C_PinsToAFOD(&pins);
    i2c->CR1 |= I2C_CR1_SWRST;
    i2c->CR1 &= ~I2C_CR1_SWRST;
    init_status = I2C_Init(pI2CHandle);

    if (!ok || init_status != I2C_STATUS_OK)
        return I2C_STATUS_RECOVERY_FAILED;
    if ((i2c->SR2 & I2C_SR2_BUSY) != 0U)
        return I2C_STATUS_RECOVERY_FAILED;
    return I2C_STATUS_OK;
}

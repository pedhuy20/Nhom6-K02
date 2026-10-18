#include "i2c_driver.h"
#include <stddef.h>

static uint8_t Valid(I2C_Handle_t *h, uint8_t address, uint32_t timeout_ms)
{
    return (uint8_t)(h != NULL && h->Instance == I2C1 &&
                     address >= 0x08U && address <= 0x77U &&
                     timeout_ms != 0U && timeout_ms != HAL_MAX_DELAY);
}

static I2C_Status_t Result(I2C_Handle_t *h, HAL_StatusTypeDef status)
{
    uint32_t error;
    if (status == HAL_OK) return I2C_STATUS_OK;
    if (status == HAL_BUSY) return I2C_STATUS_BUSY;
    if (status == HAL_TIMEOUT) return I2C_STATUS_TIMEOUT;
    error = HAL_I2C_GetError(h);
    if (error & HAL_I2C_ERROR_TIMEOUT) return I2C_STATUS_TIMEOUT;
    if (error & HAL_I2C_ERROR_BERR) return I2C_STATUS_BUS_ERROR;
    if (error & HAL_I2C_ERROR_ARLO) return I2C_STATUS_ARBITRATION_LOST;
    if (error & HAL_I2C_ERROR_OVR) return I2C_STATUS_OVERRUN;
    if (error & HAL_I2C_ERROR_AF) return I2C_STATUS_NACK;
    return I2C_STATUS_BUS_ERROR;
}

/* HAL co the tra ve ngay sau khi dat STOP; doi bus ranh nhu driver SV1.
 * Tinh trong timeout cua giao dich; NACK cung can hoan tat STOP.
 */
static I2C_Status_t Finish(I2C_Handle_t *h, HAL_StatusTypeDef status,
                         uint32_t start, uint32_t timeout_ms)
{
    I2C_Status_t result = Result(h, status);
    if (result != I2C_STATUS_OK && result != I2C_STATUS_NACK) return result;
    while (__HAL_I2C_GET_FLAG(h, I2C_FLAG_BUSY) != RESET)
        if ((uint32_t)(HAL_GetTick() - start) >= timeout_ms)
            return I2C_STATUS_TIMEOUT;
    return result;
}

I2C_Status_t I2C_Master_Transmit(I2C_Handle_t *h, uint8_t address,
    const uint8_t *data, uint16_t size, uint32_t timeout_ms)
{
    uint32_t start;
    if (!Valid(h, address, timeout_ms) || data == NULL || size == 0U)
        return I2C_STATUS_INVALID_ARG;
    start = HAL_GetTick();
    return Finish(h, HAL_I2C_Master_Transmit(h, (uint16_t)(address << 1),
                  (uint8_t *)data, size, timeout_ms), start, timeout_ms);
}

I2C_Status_t I2C_Master_Receive(I2C_Handle_t *h, uint8_t address,
    uint8_t *data, uint16_t size, uint32_t timeout_ms)
{
    uint32_t start;
    if (!Valid(h, address, timeout_ms) || data == NULL || size == 0U)
        return I2C_STATUS_INVALID_ARG;
    start = HAL_GetTick();
    return Finish(h, HAL_I2C_Master_Receive(h, (uint16_t)(address << 1),
                  data, size, timeout_ms), start, timeout_ms);
}

I2C_Status_t I2C_IsDeviceReady(I2C_Handle_t *h, uint8_t address, uint32_t timeout_ms)
{
    HAL_StatusTypeDef status;
    uint32_t start;
    if (!Valid(h, address, timeout_ms)) return I2C_STATUS_INVALID_ARG;
    start = HAL_GetTick();
    status = HAL_I2C_IsDeviceReady(h, (uint16_t)(address << 1), 1U, timeout_ms);
    /* HAL F1 tra HAL_ERROR/ErrorCode=NONE khi khong nhan ACK. */
    if (status == HAL_ERROR && HAL_I2C_GetError(h) == HAL_I2C_ERROR_NONE)
        return (uint32_t)(HAL_GetTick() - start) > timeout_ms ?
                   I2C_STATUS_TIMEOUT : I2C_STATUS_NACK;
    return Result(h, status);
}

I2C_Status_t I2C_Scan(I2C_Handle_t *h, uint8_t *addresses,
    uint8_t capacity, uint8_t *found, uint32_t timeout_ms)
{
    uint16_t address;
    I2C_Status_t status;
    if (!Valid(h, 0x08U, timeout_ms) || addresses == NULL || found == NULL ||
        capacity == 0U) return I2C_STATUS_INVALID_ARG;
    *found = 0U;
    for (address = 0x08U; address <= 0x77U; address++)
    {
        status = I2C_IsDeviceReady(h, (uint8_t)address, timeout_ms);
        if (status == I2C_STATUS_NACK) continue;
        if (status != I2C_STATUS_OK) return status;
        if (*found == capacity) return I2C_STATUS_BUFFER_FULL;
        addresses[(*found)++] = (uint8_t)address;
    }
    return I2C_STATUS_OK;
}

static void DelayUs(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000UL);
    while ((uint32_t)(DWT->CYCCNT - start) < cycles) { __NOP(); }
}

static uint8_t WaitSCL(void)
{
    uint32_t start = DWT->CYCCNT;
    while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) == GPIO_PIN_RESET)
        if ((uint32_t)(DWT->CYCCNT - start) >= SystemCoreClock / 500UL)
            return 0U; /* Toi da 2 ms, cung gioi han voi SV1. */
    return 1U;
}

I2C_Status_t I2C_BusRecover(I2C_Handle_t *h)
{
    GPIO_InitTypeDef pins = {0};
    uint8_t ok, n;
    HAL_StatusTypeDef status;
    if (h == NULL || h->Instance != I2C1 ||
        (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0U)
        return I2C_STATUS_RECOVERY_FAILED;

    if (HAL_I2C_DeInit(h) != HAL_OK) return I2C_STATUS_RECOVERY_FAILED;
    __HAL_RCC_I2C1_FORCE_RESET();
    __HAL_RCC_I2C1_RELEASE_RESET();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);
    pins.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    pins.Mode = GPIO_MODE_OUTPUT_OD;
    pins.Pull = GPIO_NOPULL;
    pins.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &pins);
    DelayUs(10U);

    ok = WaitSCL();
    for (n = 0U; ok && n < 9U &&
         HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_RESET; n++)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
        DelayUs(10U);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
        ok = WaitSCL();
        DelayUs(10U);
    }
    if (ok && HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_SET)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
        DelayUs(10U);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
        DelayUs(10U);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
        ok = WaitSCL();
        DelayUs(10U);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET); /* STOP neu SCL cao. */
        DelayUs(10U);
    }
    else ok = 0U;

    /* Tha ca hai chan truoc khi HAL MSP tra lai che do AF open-drain. */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);
    status = HAL_I2C_Init(h);
    if (!ok || status != HAL_OK ||
        HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) == GPIO_PIN_RESET ||
        HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_RESET ||
        __HAL_I2C_GET_FLAG(h, I2C_FLAG_BUSY) != RESET)
        return I2C_STATUS_RECOVERY_FAILED;
    return I2C_STATUS_OK;
}

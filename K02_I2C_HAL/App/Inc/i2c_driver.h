#ifndef SV4_HAL_I2C_DRIVER_H
#define SV4_HAL_I2C_DRIVER_H

#include "stm32f1xx_hal.h"

/* Lop tuong thich: SV2/SV3 giu nguyen API, ben duoi goi HAL I2C. */
typedef I2C_HandleTypeDef I2C_Handle_t;

typedef enum
{
    I2C_STATUS_OK = 0,
    I2C_STATUS_INVALID_ARG,
    I2C_STATUS_BUSY,
    I2C_STATUS_TIMEOUT,
    I2C_STATUS_NACK,
    I2C_STATUS_BUS_ERROR,
    I2C_STATUS_ARBITRATION_LOST,
    I2C_STATUS_OVERRUN,
    I2C_STATUS_RECOVERY_FAILED,
    I2C_STATUS_BUFFER_FULL
} I2C_Status_t;

#define I2C_TIMEOUT_DEFAULT_MS 100U

/* DevAddress la dia chi 7 bit CHUA dich trai; adapter tu dich cho HAL. */
I2C_Status_t I2C_Master_Transmit(I2C_Handle_t *h, uint8_t DevAddress,
    const uint8_t *data, uint16_t size, uint32_t timeout_ms);
I2C_Status_t I2C_Master_Receive(I2C_Handle_t *h, uint8_t DevAddress,
    uint8_t *data, uint16_t size, uint32_t timeout_ms);
I2C_Status_t I2C_IsDeviceReady(I2C_Handle_t *h, uint8_t DevAddress,
    uint32_t timeout_ms);
I2C_Status_t I2C_Scan(I2C_Handle_t *h, uint8_t *addresses,
    uint8_t capacity, uint8_t *found, uint32_t timeout_ms);
I2C_Status_t I2C_BusRecover(I2C_Handle_t *h);

#endif

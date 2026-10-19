
#ifndef __I2C_DRIVER_H
#define __I2C_DRIVER_H

#include "stm32f10x.h"
#include <stdint.h>

/* =====================================================
 * 1. I2C CONFIGURATION MACROS
 * ===================================================== */

/* @I2C_SCLSpeed */
#define I2C_SCL_SPEED_SM        100000U
#define I2C_SCL_SPEED_FM        400000U

/* @I2C_ACKControl */
#define I2C_ACK_ENABLE          1U
#define I2C_ACK_DISABLE         0U

/* @I2C_FMDutyCycle */
#define I2C_FM_DUTY_2           0U
#define I2C_FM_DUTY_16_9        1U

/* Peripheral enable/disable */
#define I2C_ENABLE              1U
#define I2C_DISABLE             0U

/* I2C address direction */
#define I2C_WRITE               0U
#define I2C_READ                1U

/* Default timeout in milliseconds */
#define I2C_TIMEOUT_DEFAULT_MS  100U


/* =====================================================
 * 2. I2C STATUS DEFINITIONS
 * ===================================================== */

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


/* =====================================================
 * 3. I2C CONFIGURATION STRUCTURE
 * ===================================================== */

typedef struct
{
    /* I2C clock speed: 100kHz or 400kHz */
    uint32_t I2C_SCLSpeed;

    /* Enable/Disable ACK */
    uint8_t I2C_ACKControl;

    /* Fast mode duty cycle */
    uint8_t I2C_FMDutyCycle;

} I2C_Config_t;


/* =====================================================
 * 4. I2C HANDLE STRUCTURE
 * ===================================================== */

typedef struct
{
    /* I2C1 or I2C2 peripheral */
    I2C_TypeDef *pI2Cx;

    /* I2C configuration settings */
    I2C_Config_t I2C_Config;

} I2C_Handle_t;


/* =====================================================
 * 5. PERIPHERAL CLOCK CONTROL
 * ===================================================== */

void I2C_PeriClockControl(
    I2C_TypeDef *pI2Cx,
    uint8_t EnorDi
);


/* =====================================================
 * 6. INITIALIZATION AND DE-INITIALIZATION
 * ===================================================== */

I2C_Status_t I2C_Init(
    I2C_Handle_t *pI2CHandle
);

void I2C_DeInit(
    I2C_TypeDef *pI2Cx
);


/* =====================================================
 * 7. PERIPHERAL CONTROL
 * ===================================================== */

void I2C_PeripheralControl(
    I2C_TypeDef *pI2Cx,
    uint8_t EnorDi
);


/* =====================================================
 * 8. MASTER TRANSMIT
 *
 * DevAddress: 7-bit address (NOT shifted)
 * pData:      Data buffer
 * Size:       Number of bytes
 * TimeoutMs:  Timeout in milliseconds
 * ===================================================== */

I2C_Status_t I2C_Master_Transmit(
    I2C_Handle_t *pI2CHandle,
    uint8_t DevAddress,
    const uint8_t *pData,
    uint16_t Size,
    uint32_t TimeoutMs
);


/* =====================================================
 * 9. MASTER RECEIVE
 *
 * DevAddress: 7-bit address (NOT shifted)
 * pData:      Receive buffer
 * Size:       Number of bytes
 * TimeoutMs:  Timeout in milliseconds
 * ===================================================== */

I2C_Status_t I2C_Master_Receive(
    I2C_Handle_t *pI2CHandle,
    uint8_t DevAddress,
    uint8_t *pData,
    uint16_t Size,
    uint32_t TimeoutMs
);


/* =====================================================
 * 10. MASTER WRITE-READ
 *
 * Combined transaction using repeated START.
 * No STOP between write and read phases.
 * ===================================================== */

I2C_Status_t I2C_Master_WriteRead(
    I2C_Handle_t *pI2CHandle,
    uint8_t DevAddress,
    const uint8_t *pTxData,
    uint16_t TxSize,
    uint8_t *pRxData,
    uint16_t RxSize,
    uint32_t TimeoutMs
);


/* =====================================================
 * 11. DEVICE READY CHECK
 *
 * Checks whether a device ACKs its address.
 * ===================================================== */

I2C_Status_t I2C_IsDeviceReady(
    I2C_Handle_t *pI2CHandle,
    uint8_t DevAddress,
    uint32_t TimeoutMs
);


/* =====================================================
 * 12. I2C ADDRESS SCANNER
 *
 * Scans valid 7-bit device addresses.
 * Stores detected addresses in pAddresses.
 * ===================================================== */

I2C_Status_t I2C_Scan(
    I2C_Handle_t *pI2CHandle,
    uint8_t *pAddresses,
    uint8_t MaxDevices,
    uint8_t *pFoundCount,
    uint32_t TimeoutMs
);


/* =====================================================
 * 13. I2C BUS RECOVERY
 *
 * Attempts to release a stuck I2C bus.
 * May generate up to 9 SCL pulses.
 * ===================================================== */

I2C_Status_t I2C_BusRecover(
    I2C_Handle_t *pI2CHandle
);

#endif /* __I2C_DRIVER_H */

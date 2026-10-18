#ifndef SV4_APP_H
#define SV4_APP_H

#include "stm32f1xx_hal.h"

void SV4_Init(I2C_HandleTypeDef *i2c);
void SV4_Task(void);

#endif

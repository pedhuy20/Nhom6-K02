
#include "gpio.h"
#include "rcc.h"

void GPIO_I2C_Init(void)
{
    RCC_EnableGPIOBClock();
    GPIOB->BSRR = (1UL << 6) | (1UL << 7);
    GPIOB->CRL.MODE_6 = 3;
    GPIOB->CRL.CNF_6  = 3;
    GPIOB->CRL.MODE_7 = 3;
    GPIOB->CRL.CNF_7  = 3;
}

void GPIO_I2C_ToGPIO(void)
{
    GPIOB->BSRR = (1UL << 6) | (1UL << 7);
    GPIOB->CRL.CNF_6  = 1;
    GPIOB->CRL.MODE_6 = 3;
    GPIOB->CRL.CNF_7  = 1;
    GPIOB->CRL.MODE_7 = 3;
}
void GPIO_SCL_Write(unsigned char level)
{
    if (level == 0)
    {
        GPIOB->BRR = (1UL << 6);  
    }
    else
    {
        GPIOB->BSRR = (1UL << 6); 
    }
}

void GPIO_SDA_Write(unsigned char level)
{
    if (level == 0)
    {
        GPIOB->BRR = (1UL << 7);  
    }
    else
    {
        GPIOB->BSRR = (1UL << 7); 
    }
}

unsigned char GPIO_SCL_Read(void)
{
    return (unsigned char)GPIOB->IDR.b6;
}

unsigned char GPIO_SDA_Read(void)
{
    return (unsigned char)GPIOB->IDR.b7;
}

#include "rcc.h"

#define RCC_WAIT_LIMIT 1000000UL

unsigned char RCC_InitHSI8MHz(void)
{
    unsigned long timeout;
    RCC->CR.HSION = 1;
    timeout = RCC_WAIT_LIMIT;
    while (RCC->CR.HSIRDY == 0)
    {
        if (timeout == 0UL)
        {
            return 0;
        }
        timeout--;
    }
    RCC->CFGR.SW = 0;
    timeout = RCC_WAIT_LIMIT;
    while (RCC->CFGR.SWS != 0)
    {
        if (timeout == 0UL)
        {
            return 0;
        }
        timeout--;
    }

    RCC->CFGR.HPRE  = 0;
    RCC->CFGR.PPRE1 = 0;
    RCC->CFGR.PPRE2 = 0;
    return 1;
}

void RCC_EnableGPIOBClock(void)
{
    RCC->APB2ENR.IOPBEN = 1;
    (void)RCC->APB2ENR.IOPBEN;
}

void RCC_EnableI2C1Clock(void)
{
    RCC->APB1ENR.I2C1EN = 1;
    (void)RCC->APB1ENR.I2C1EN;
}
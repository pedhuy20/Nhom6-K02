#include "stm32f10x.h"

void test(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
}
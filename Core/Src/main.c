#include "rcc.h"
#include "gpio.h"




int main()
{
    RCC_Enable_PortA();
    RCC_Enable_PortB();

    RCC_Enable_AFIO();
    RCC_Enalble_TIM2();
    while (1)
    {

    }
}
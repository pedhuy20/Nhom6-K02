#ifndef RCC_H
#define RCC_H
typedef struct
{
 struct
 {
  unsigned long HSION     : 1; 
  unsigned long HSIRDY    : 1; 
  unsigned long reserved0 : 1; 
  unsigned long HSITRIM   : 5; 
  unsigned long HSICAL    : 8; 
  unsigned long HSEON     : 1; 
  unsigned long HSERDY    : 1; 
  unsigned long HSEBYP    : 1; 
  unsigned long CSSON     : 1; 
  unsigned long reserved1 : 4; 
  unsigned long PLLON     : 1; 
  unsigned long PLLRDY    : 1; 
  unsigned long reserved2 : 6;
 }CR;
 struct
 {
  unsigned long SW        : 2;  
  unsigned long SWS       : 2;  
  unsigned long HPRE      : 4;  
  unsigned long PPRE1     : 3;  
  unsigned long PPRE2     : 3;  
  unsigned long ADCPRE    : 2;  
  unsigned long PLLSRC    : 1;  
  unsigned long PLLXTPRE  : 1;  
  unsigned long PLLMUL    : 4;  
  unsigned long USBPRE    : 1;  
  unsigned long reserved0 : 1;  
  unsigned long MCO       : 3;  
  unsigned long reserved1 : 5;  
 }CFGR;
unsigned long CIR;
unsigned long APB2RSTR;
unsigned long APB1RSTR;
unsigned long AHBENR;
struct
  {
   unsigned long AFIOEN    : 1;  
   unsigned long reserved0 : 1;  
   unsigned long IOPAEN    : 1;  
   unsigned long IOPBEN    : 1;  
   unsigned long IOPCEN    : 1;  
   unsigned long IOPDEN    : 1;  
   unsigned long IOPEEN    : 1;  
   unsigned long IOPFEN    : 1;  
   unsigned long IOPGEN    : 1;  
   unsigned long reserved1 : 23; 
  }APB2ENR;
struct
  {
   unsigned long reserved0 : 21; 
   unsigned long I2C1EN    : 1;  
   unsigned long I2C2EN    : 1;  
   unsigned long reserved1 : 9;  
  }APB1ENR;  
unsigned long BDCR;
unsigned long CSR;
}RCC_TypeDef;

#define RCC ((volatile RCC_TypeDef *)0x40021000UL)

#define RCC_HCLK_HZ   8000000UL
#define RCC_PCLK1_HZ  8000000UL
#define RCC_PCLK2_HZ  8000000UL

unsigned char RCC_InitHSI8MHz(void);
void RCC_EnableGPIOBClock(void);
void RCC_EnableI2C1Clock(void);

#endif

#ifndef GPIO_H
#define GPIO_H

typedef struct
{
 struct
 {
  unsigned long MODE_0 : 2;
  unsigned long CNF_0  : 2;
  unsigned long MODE_1 : 2;
  unsigned long CNF_1  : 2;
  unsigned long MODE_2 : 2;
  unsigned long CNF_2  : 2;
  unsigned long MODE_3 : 2;
  unsigned long CNF_3  : 2;
  unsigned long MODE_4 : 2;
  unsigned long CNF_4  : 2;
  unsigned long MODE_5 : 2;
  unsigned long CNF_5  : 2;
  unsigned long MODE_6 : 2;
  unsigned long CNF_6  : 2;
  unsigned long MODE_7 : 2;
  unsigned long CNF_7  : 2;
 }CRL;
 unsigned long CRH;
 struct
 {
  unsigned long b0  : 1;
  unsigned long b1  : 1;
  unsigned long b2  : 1;
  unsigned long b3  : 1;
  unsigned long b4  : 1;
  unsigned long b5  : 1;
  unsigned long b6  : 1;
  unsigned long b7  : 1;
  unsigned long b8  : 1;
  unsigned long b9  : 1;
  unsigned long b10 : 1;
  unsigned long b11 : 1;
  unsigned long b12 : 1;
  unsigned long b13 : 1;
  unsigned long b14 : 1;
  unsigned long b15 : 1;
  unsigned long reserved : 16;
 }IDR;
unsigned long ODR;          
unsigned long BSRR; 
unsigned long BRR;
unsigned long LCKR;
}GPIO_TypeDef;
#define GPIOB ((volatile GPIO_TypeDef *)0x40010C00UL)
	
void GPIO_I2C_Init(void);
void GPIO_I2C_ToGPIO(void);
void GPIO_SCL_Write(unsigned char level);
void GPIO_SDA_Write(unsigned char level);
unsigned char GPIO_SCL_Read(void);
unsigned char GPIO_SDA_Read(void);

#endif

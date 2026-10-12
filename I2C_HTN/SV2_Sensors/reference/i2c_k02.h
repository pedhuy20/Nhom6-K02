#ifndef __I2C_K02_H__
#define __I2C_K02_H__

#include <stdint.h>
#include "stm32f10x.h"


typedef enum
{
    I2C_THANH_CONG = 0,
    I2C_LOI_TIMEOUT,
    I2C_LOI_NACK,
    I2C_LOI_BUS,
    I2C_LOI_TRANH_CHAP,
    I2C_LOI_THAM_SO

} I2C_TrangThai;


/* Khoi tao I2C1
 * PB6 = SCL
 * PB7 = SDA
 * 100 kHz
 */
void I2C_KhoiTao(void);


/* Tao START */
I2C_TrangThai I2C_BatDau(void);


/* Tao STOP */
void I2C_Dung(void);


/* Gui dia chi slave 7-bit
 * doc_ghi = 0: ghi
 * doc_ghi = 1: doc
 */
I2C_TrangThai I2C_GuiDiaChi(uint8_t dia_chi,
                            uint8_t doc_ghi);


/* Gui 1 byte */
I2C_TrangThai I2C_GuiByte(uint8_t du_lieu);


/* Gui nhieu byte */
I2C_TrangThai I2C_GhiMang(uint8_t dia_chi,
                          const uint8_t *du_lieu,
                          uint16_t so_byte);


/* Doc nhieu byte */
I2C_TrangThai I2C_DocMang(uint8_t dia_chi,
                          uint8_t *du_lieu,
                          uint16_t so_byte);


/* Kiem tra slave co ACK hay khong */
I2C_TrangThai I2C_KiemTraThietBi(uint8_t dia_chi);


#endif
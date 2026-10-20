#ifndef __SHT30_H__
#define __SHT30_H__

#include <stdint.h>
#include "i2c_driver.h"


#define SHT30_DIA_CHI          0x44U

#define SHT30_LENH_DO_BYTE_1   0x24U
#define SHT30_LENH_DO_BYTE_2   0x00U


typedef enum
{
    SHT30_THANH_CONG = 0,
    SHT30_LOI_I2C,
    SHT30_LOI_CRC,
    SHT30_LOI_THAM_SO

} SHT30_TrangThai;


typedef struct
{
    float nhiet_do_c;
    float do_am_rh;

    uint16_t nhiet_do_tho;
    uint16_t do_am_tho;

} SHT30_DuLieu;


/* Kiem tra cam bien co phan hoi hay khong */
SHT30_TrangThai SHT30_KiemTraKetNoi(
    I2C_Handle_t *bo_dieu_khien_i2c
);


/* Gui lenh bat dau do */
SHT30_TrangThai SHT30_BatDauDo(
    I2C_Handle_t *bo_dieu_khien_i2c
);


/* Doc va xu ly du lieu nhiet do, do am */
SHT30_TrangThai SHT30_DocDuLieu(
    I2C_Handle_t *bo_dieu_khien_i2c,
    SHT30_DuLieu *du_lieu
);


/* Tinh CRC-8 */
uint8_t SHT30_TinhCRC(
    const uint8_t *du_lieu,
    uint8_t so_byte
);


#endif
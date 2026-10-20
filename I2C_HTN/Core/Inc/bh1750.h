#ifndef __BH1750_H__
#define __BH1750_H__

#include <stdint.h>
#include "i2c_driver.h"


#define BH1750_DIA_CHI              0x23U

#define BH1750_LENH_TAT             0x00U
#define BH1750_LENH_BAT             0x01U
#define BH1750_LENH_RESET           0x07U

#define BH1750_DO_LIEN_TUC_CAO      0x10U


typedef enum
{
    BH1750_THANH_CONG = 0,
    BH1750_LOI_I2C,
    BH1750_LOI_THAM_SO

} BH1750_TrangThai;


/* Kiem tra cam bien co phan hoi hay khong */
BH1750_TrangThai BH1750_KiemTraKetNoi(
    I2C_Handle_t *bo_dieu_khien_i2c
);


/* Khoi tao cam bien */
BH1750_TrangThai BH1750_KhoiTao(
    I2C_Handle_t *bo_dieu_khien_i2c
);


/* Gui mot lenh den cam bien */
BH1750_TrangThai BH1750_GuiLenh(
    I2C_Handle_t *bo_dieu_khien_i2c,
    uint8_t lenh
);


/* Doc gia tri tho 16-bit */
BH1750_TrangThai BH1750_DocGiaTriTho(
    I2C_Handle_t *bo_dieu_khien_i2c,
    uint16_t *gia_tri_tho
);


/* Doc cuong do anh sang theo lux */
BH1750_TrangThai BH1750_DocLux(
    I2C_Handle_t *bo_dieu_khien_i2c,
    float *do_sang_lux
);


#endif
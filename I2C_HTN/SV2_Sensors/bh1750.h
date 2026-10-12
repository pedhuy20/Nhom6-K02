#ifndef __BH1750_H__
#define __BH1750_H__

#include <stdint.h>
#include "i2c_k02.h"


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


/* Khoi tao sensor */
BH1750_TrangThai BH1750_KhoiTao(void);


/* Gui mot lenh den sensor */
BH1750_TrangThai BH1750_GuiLenh(uint8_t lenh);


/* Doc gia tri raw 16-bit */
BH1750_TrangThai BH1750_DocRaw(uint16_t *gia_tri_raw);


/* Doc anh sang theo lux */
BH1750_TrangThai BH1750_DocLux(float *do_sang_lux);


#endif
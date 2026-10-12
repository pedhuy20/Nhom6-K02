#include "bh1750.h"


BH1750_TrangThai BH1750_GuiLenh(uint8_t lenh)
{
    I2C_TrangThai trang_thai_i2c;

    trang_thai_i2c =
        I2C_GhiMang(BH1750_DIA_CHI,
                    &lenh,
                    1U);

    if (trang_thai_i2c != I2C_THANH_CONG)
    {
        return BH1750_LOI_I2C;
    }

    return BH1750_THANH_CONG;
}


BH1750_TrangThai BH1750_KhoiTao(void)
{
    BH1750_TrangThai trang_thai;

    /* Bat sensor */
    trang_thai =
        BH1750_GuiLenh(BH1750_LENH_BAT);

    if (trang_thai != BH1750_THANH_CONG)
    {
        return trang_thai;
    }


    /*
     * Xoa du lieu cu trong thanh ghi.
     * RESET chi hop le khi sensor da Power On.
     */
    trang_thai =
        BH1750_GuiLenh(BH1750_LENH_RESET);

    if (trang_thai != BH1750_THANH_CONG)
    {
        return trang_thai;
    }


    /*
     * Continuous High Resolution Mode
     * command = 0x10
     */
    trang_thai =
        BH1750_GuiLenh(BH1750_DO_LIEN_TUC_CAO);

    if (trang_thai != BH1750_THANH_CONG)
    {
        return trang_thai;
    }

    return BH1750_THANH_CONG;
}


BH1750_TrangThai BH1750_DocRaw(uint16_t *gia_tri_raw)
{
    uint8_t du_lieu[2];
    I2C_TrangThai trang_thai_i2c;

    if (gia_tri_raw == 0)
    {
        return BH1750_LOI_THAM_SO;
    }

    trang_thai_i2c =
        I2C_DocMang(BH1750_DIA_CHI,
                    du_lieu,
                    2U);

    if (trang_thai_i2c != I2C_THANH_CONG)
    {
        return BH1750_LOI_I2C;
    }

    *gia_tri_raw =
        ((uint16_t)du_lieu[0] << 8) |
        (uint16_t)du_lieu[1];

    return BH1750_THANH_CONG;
}


BH1750_TrangThai BH1750_DocLux(float *do_sang_lux)
{
    uint16_t gia_tri_raw;
    BH1750_TrangThai trang_thai;

    if (do_sang_lux == 0)
    {
        return BH1750_LOI_THAM_SO;
    }

    trang_thai =
        BH1750_DocRaw(&gia_tri_raw);

    if (trang_thai != BH1750_THANH_CONG)
    {
        return trang_thai;
    }

    /*
     * Theo he so quy doi mac dinh:
     *
     * Lux = RAW / 1.2
     */
    *do_sang_lux =
        (float)gia_tri_raw / 1.2f;

    return BH1750_THANH_CONG;
}
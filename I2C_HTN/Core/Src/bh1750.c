#include "bh1750.h"


BH1750_TrangThai BH1750_KiemTraKetNoi(
    I2C_Handle_t *bo_dieu_khien_i2c)
{
    I2C_Status_t trang_thai_i2c;

    if (bo_dieu_khien_i2c == 0)
    {
        return BH1750_LOI_THAM_SO;
    }

    trang_thai_i2c =
        I2C_IsDeviceReady(
            bo_dieu_khien_i2c,
            BH1750_DIA_CHI,
            I2C_TIMEOUT_DEFAULT_MS
        );

    if (trang_thai_i2c != I2C_STATUS_OK)
    {
        return BH1750_LOI_I2C;
    }

    return BH1750_THANH_CONG;
}


BH1750_TrangThai BH1750_GuiLenh(
    I2C_Handle_t *bo_dieu_khien_i2c,
    uint8_t lenh)
{
    I2C_Status_t trang_thai_i2c;

    if (bo_dieu_khien_i2c == 0)
    {
        return BH1750_LOI_THAM_SO;
    }

    trang_thai_i2c =
        I2C_Master_Transmit(
            bo_dieu_khien_i2c,
            BH1750_DIA_CHI,
            &lenh,
            1U,
            I2C_TIMEOUT_DEFAULT_MS
        );

    if (trang_thai_i2c != I2C_STATUS_OK)
    {
        return BH1750_LOI_I2C;
    }

    return BH1750_THANH_CONG;
}


BH1750_TrangThai BH1750_KhoiTao(
    I2C_Handle_t *bo_dieu_khien_i2c)
{
    BH1750_TrangThai trang_thai;

    if (bo_dieu_khien_i2c == 0)
    {
        return BH1750_LOI_THAM_SO;
    }

    trang_thai =
        BH1750_KiemTraKetNoi(
            bo_dieu_khien_i2c
        );

    if (trang_thai != BH1750_THANH_CONG)
    {
        return trang_thai;
    }


    /* Bat nguon cho cam bien */
    trang_thai =
        BH1750_GuiLenh(
            bo_dieu_khien_i2c,
            BH1750_LENH_BAT
        );

    if (trang_thai != BH1750_THANH_CONG)
    {
        return trang_thai;
    }


    /* Xoa du lieu do cu */
    trang_thai =
        BH1750_GuiLenh(
            bo_dieu_khien_i2c,
            BH1750_LENH_RESET
        );

    if (trang_thai != BH1750_THANH_CONG)
    {
        return trang_thai;
    }


    /* Chon che do do lien tuc do phan giai cao */
    trang_thai =
        BH1750_GuiLenh(
            bo_dieu_khien_i2c,
            BH1750_DO_LIEN_TUC_CAO
        );

    if (trang_thai != BH1750_THANH_CONG)
    {
        return trang_thai;
    }

    return BH1750_THANH_CONG;
}


BH1750_TrangThai BH1750_DocGiaTriTho(
    I2C_Handle_t *bo_dieu_khien_i2c,
    uint16_t *gia_tri_tho)
{
    uint8_t du_lieu[2];
    I2C_Status_t trang_thai_i2c;

    if ((bo_dieu_khien_i2c == 0) ||
        (gia_tri_tho == 0))
    {
        return BH1750_LOI_THAM_SO;
    }

    trang_thai_i2c =
        I2C_Master_Receive(
            bo_dieu_khien_i2c,
            BH1750_DIA_CHI,
            du_lieu,
            2U,
            I2C_TIMEOUT_DEFAULT_MS
        );

    if (trang_thai_i2c != I2C_STATUS_OK)
    {
        return BH1750_LOI_I2C;
    }

    *gia_tri_tho =
        ((uint16_t)du_lieu[0] << 8) |
        (uint16_t)du_lieu[1];

    return BH1750_THANH_CONG;
}


BH1750_TrangThai BH1750_DocLux(
    I2C_Handle_t *bo_dieu_khien_i2c,
    float *do_sang_lux)
{
    uint16_t gia_tri_tho;
    BH1750_TrangThai trang_thai;

    if ((bo_dieu_khien_i2c == 0) ||
        (do_sang_lux == 0))
    {
        return BH1750_LOI_THAM_SO;
    }

    trang_thai =
        BH1750_DocGiaTriTho(
            bo_dieu_khien_i2c,
            &gia_tri_tho
        );

    if (trang_thai != BH1750_THANH_CONG)
    {
        return trang_thai;
    }


    /*
     * Cong thuc quy doi:
     *
     * Lux = gia_tri_tho / 1.2
     */
    *do_sang_lux =
        (float)gia_tri_tho / 1.2f;

    return BH1750_THANH_CONG;
}
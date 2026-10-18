#include "sht30.h"


/* =========================================================
 * Tinh CRC-8
 *
 * Da thuc = 0x31
 * Gia tri khoi tao = 0xFF
 * ========================================================= */
uint8_t SHT30_TinhCRC(
    const uint8_t *du_lieu,
    uint8_t so_byte)
{
    uint8_t crc = 0xFFU;
    uint8_t vi_tri;
    uint8_t bit;

    if (du_lieu == 0)
    {
        return 0U;
    }

    for (vi_tri = 0U;
         vi_tri < so_byte;
         vi_tri++)
    {
        crc ^= du_lieu[vi_tri];

        for (bit = 0U;
             bit < 8U;
             bit++)
        {
            if (crc & 0x80U)
            {
                crc =
                    (uint8_t)(
                        (crc << 1) ^
                        0x31U
                    );
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}


/* =========================================================
 * Kiem tra ket noi cam bien
 * ========================================================= */
SHT30_TrangThai SHT30_KiemTraKetNoi(
    I2C_Handle_t *bo_dieu_khien_i2c)
{
    I2C_Status_t trang_thai_i2c;

    if (bo_dieu_khien_i2c == 0)
    {
        return SHT30_LOI_THAM_SO;
    }

    trang_thai_i2c =
        I2C_IsDeviceReady(
            bo_dieu_khien_i2c,
            SHT30_DIA_CHI,
            I2C_TIMEOUT_DEFAULT_MS
        );

    if (trang_thai_i2c != I2C_STATUS_OK)
    {
        return SHT30_LOI_I2C;
    }

    return SHT30_THANH_CONG;
}


/* =========================================================
 * Gui lenh do mot lan, do chinh xac cao
 *
 * Lenh = 0x2400
 * ========================================================= */
SHT30_TrangThai SHT30_BatDauDo(
    I2C_Handle_t *bo_dieu_khien_i2c)
{
    uint8_t lenh[2];
    I2C_Status_t trang_thai_i2c;

    if (bo_dieu_khien_i2c == 0)
    {
        return SHT30_LOI_THAM_SO;
    }

    lenh[0] =
        SHT30_LENH_DO_BYTE_1;

    lenh[1] =
        SHT30_LENH_DO_BYTE_2;


    trang_thai_i2c =
        I2C_Master_Transmit(
            bo_dieu_khien_i2c,
            SHT30_DIA_CHI,
            lenh,
            2U,
            I2C_TIMEOUT_DEFAULT_MS
        );

    if (trang_thai_i2c != I2C_STATUS_OK)
    {
        return SHT30_LOI_I2C;
    }

    return SHT30_THANH_CONG;
}


/* =========================================================
 * Doc nhiet do va do am
 *
 * Cam bien tra ve 6 byte:
 *
 * Byte 0 : Nhiet do byte cao
 * Byte 1 : Nhiet do byte thap
 * Byte 2 : CRC nhiet do
 *
 * Byte 3 : Do am byte cao
 * Byte 4 : Do am byte thap
 * Byte 5 : CRC do am
 * ========================================================= */
SHT30_TrangThai SHT30_DocDuLieu(
    I2C_Handle_t *bo_dieu_khien_i2c,
    SHT30_DuLieu *du_lieu)
{
    uint8_t bo_dem[6];

    uint16_t nhiet_do_tho;
    uint16_t do_am_tho;

    uint8_t crc_nhiet_do;
    uint8_t crc_do_am;

    I2C_Status_t trang_thai_i2c;


    if ((bo_dieu_khien_i2c == 0) ||
        (du_lieu == 0))
    {
        return SHT30_LOI_THAM_SO;
    }


    trang_thai_i2c =
        I2C_Master_Receive(
            bo_dieu_khien_i2c,
            SHT30_DIA_CHI,
            bo_dem,
            6U,
            I2C_TIMEOUT_DEFAULT_MS
        );

    if (trang_thai_i2c != I2C_STATUS_OK)
    {
        return SHT30_LOI_I2C;
    }


    /* Kiem tra CRC cua nhiet do */
    crc_nhiet_do =
        SHT30_TinhCRC(
            &bo_dem[0],
            2U
        );

    if (crc_nhiet_do != bo_dem[2])
    {
        return SHT30_LOI_CRC;
    }


    /* Kiem tra CRC cua do am */
    crc_do_am =
        SHT30_TinhCRC(
            &bo_dem[3],
            2U
        );

    if (crc_do_am != bo_dem[5])
    {
        return SHT30_LOI_CRC;
    }


    /* Ghep 2 byte thanh gia tri nhiet do tho */
    nhiet_do_tho =
        ((uint16_t)bo_dem[0] << 8) |
        (uint16_t)bo_dem[1];


    /* Ghep 2 byte thanh gia tri do am tho */
    do_am_tho =
        ((uint16_t)bo_dem[3] << 8) |
        (uint16_t)bo_dem[4];


    du_lieu->nhiet_do_tho =
        nhiet_do_tho;

    du_lieu->do_am_tho =
        do_am_tho;


    /*
     * Cong thuc nhiet do:
     *
     * T = -45 + 175 * gia_tri_tho / 65535
     */
    du_lieu->nhiet_do_c =
        -45.0f +
        175.0f *
        ((float)nhiet_do_tho /
         65535.0f);


    /*
     * Cong thuc do am:
     *
     * RH = 100 * gia_tri_tho / 65535
     */
    du_lieu->do_am_rh =
        100.0f *
        ((float)do_am_tho /
         65535.0f);


    return SHT30_THANH_CONG;
}
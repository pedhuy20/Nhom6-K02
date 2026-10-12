#include "sht30.h"


volatile uint8_t sht30_byte_0 = 0;
volatile uint8_t sht30_byte_1 = 0;
volatile uint8_t sht30_byte_2 = 0;
volatile uint8_t sht30_byte_3 = 0;
volatile uint8_t sht30_byte_4 = 0;
volatile uint8_t sht30_byte_5 = 0;

volatile uint8_t sht30_crc_nhiet_do = 0;
volatile uint8_t sht30_crc_do_am = 0;

/* =========================================================
 * CRC-8
 *
 * Polynomial = 0x31
 * Khoi tao   = 0xFF
 * ========================================================= */
uint8_t SHT30_TinhCRC(const uint8_t *du_lieu,
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
                    (uint8_t)((crc << 1) ^ 0x31U);
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
 * Gui lenh single-shot high repeatability
 *
 * Command = 0x2400
 * ========================================================= */
SHT30_TrangThai SHT30_BatDauDo(void)
{
    uint8_t lenh[2];
    I2C_TrangThai trang_thai_i2c;

    lenh[0] = SHT30_LENH_DO_BYTE_1;
    lenh[1] = SHT30_LENH_DO_BYTE_2;

    trang_thai_i2c =
        I2C_GhiMang(SHT30_DIA_CHI,
                    lenh,
                    2U);

    if (trang_thai_i2c != I2C_THANH_CONG)
    {
        return SHT30_LOI_I2C;
    }

    return SHT30_THANH_CONG;
}


/* =========================================================
 * Doc nhiet do + do am
 *
 * Sensor tra ve:
 *
 * Byte 0 : Temperature MSB
 * Byte 1 : Temperature LSB
 * Byte 2 : Temperature CRC
 *
 * Byte 3 : Humidity MSB
 * Byte 4 : Humidity LSB
 * Byte 5 : Humidity CRC
 * ========================================================= */
SHT30_TrangThai SHT30_DocDuLieu(SHT30_DuLieu *du_lieu)
{
    uint8_t bo_dem[6];

    uint16_t nhiet_do_raw;
    uint16_t do_am_raw;

    uint8_t crc_nhiet_do;
    uint8_t crc_do_am;

    I2C_TrangThai trang_thai_i2c;


    if (du_lieu == 0)
    {
        return SHT30_LOI_THAM_SO;
    }


    trang_thai_i2c =
        I2C_DocMang(SHT30_DIA_CHI,
                    bo_dem,
                    6U);

    if (trang_thai_i2c != I2C_THANH_CONG)
    {
        return SHT30_LOI_I2C;
    }
sht30_byte_0 = bo_dem[0];
sht30_byte_1 = bo_dem[1];
sht30_byte_2 = bo_dem[2];
sht30_byte_3 = bo_dem[3];
sht30_byte_4 = bo_dem[4];
sht30_byte_5 = bo_dem[5];

sht30_crc_nhiet_do =
    SHT30_TinhCRC(&bo_dem[0], 2U);

sht30_crc_do_am =
    SHT30_TinhCRC(&bo_dem[3], 2U);

    /* Kiem tra CRC cua nhiet do */
    crc_nhiet_do =
        SHT30_TinhCRC(&bo_dem[0], 2U);

    if (crc_nhiet_do != bo_dem[2])
    {
        return SHT30_LOI_CRC;
    }


    /* Kiem tra CRC cua do am */
    crc_do_am =
        SHT30_TinhCRC(&bo_dem[3], 2U);

    if (crc_do_am != bo_dem[5])
    {
        return SHT30_LOI_CRC;
    }


    /* Ghep 2 byte nhiet do */
    nhiet_do_raw =
        ((uint16_t)bo_dem[0] << 8) |
        (uint16_t)bo_dem[1];


    /* Ghep 2 byte do am */
    do_am_raw =
        ((uint16_t)bo_dem[3] << 8) |
        (uint16_t)bo_dem[4];


    du_lieu->nhiet_do_raw =
        nhiet_do_raw;

    du_lieu->do_am_raw =
        do_am_raw;


    /*
     * Nhiet do:
     *
     * T = -45 + 175 x RAW / 65535
     */
    du_lieu->nhiet_do_c =
        -45.0f +
        175.0f *
        ((float)nhiet_do_raw / 65535.0f);


    /*
     * Do am:
     *
     * RH = 100 x RAW / 65535
     */
    du_lieu->do_am_rh =
        100.0f *
        ((float)do_am_raw / 65535.0f);


    return SHT30_THANH_CONG;
}
#include "stm32f10x.h"
#include "i2c_driver.h"
#include "bh1750.h"
#include "sht30.h"
#include <stdint.h>

/* K02 - Test rieng SV2: BH1750 + SHT30, I2C do SV1 viet */
#define BH1750_CHO_DO_MS        200U
#define SHT30_CHO_DO_MS          20U
#define CHU_KY_DOC_MS         1000U

static I2C_Handle_t bo_dieu_khien_i2c;

/* Bien theo doi trong Keil Watch */
volatile I2C_Status_t trang_thai_i2c = I2C_STATUS_INVALID_ARG;
volatile I2C_Status_t kiem_tra_bh1750 = I2C_STATUS_INVALID_ARG;
volatile I2C_Status_t kiem_tra_sht30 = I2C_STATUS_INVALID_ARG;

volatile BH1750_TrangThai trang_thai_bh1750_khoi_tao = BH1750_LOI_I2C;
volatile BH1750_TrangThai trang_thai_bh1750_doc = BH1750_LOI_I2C;
volatile SHT30_TrangThai trang_thai_sht30_bat_dau = SHT30_LOI_I2C;
volatile SHT30_TrangThai trang_thai_sht30_doc = SHT30_LOI_I2C;

volatile float anh_sang_lux = 0.0f;
volatile float nhiet_do_c = 0.0f;
volatile float do_am_rh = 0.0f;
volatile uint16_t nhiet_do_raw = 0U;
volatile uint16_t do_am_raw = 0U;

/* 1: cam bien dang giao tiep duoc; 0: chua giao tiep duoc */
volatile uint8_t bh1750_ket_noi = 0U;
volatile uint8_t sht30_ket_noi = 0U;
/* 1: co du lieu moi hop le o vong do hien tai */
volatile uint8_t bh1750_du_lieu_moi = 0U;
volatile uint8_t sht30_du_lieu_moi = 0U;
volatile uint32_t so_lan_do = 0U;

/* DWT duoc SV1 bat trong I2C_Init. Moi lan goi < 1 giay. */
static void Tre_Ms(uint32_t thoi_gian_ms)
{
    uint32_t bat_dau = DWT->CYCCNT;
    uint32_t so_chu_ky = (SystemCoreClock / 1000U) * thoi_gian_ms;

    while ((uint32_t)(DWT->CYCCNT - bat_dau) < so_chu_ky)
    {
    }
}

/* Khong khoi tao lai ca bus I2C neu chi mot cam bien bi rut. */
static void BH1750_ThuKetNoiLai(void)
{
    kiem_tra_bh1750 = I2C_IsDeviceReady(
        &bo_dieu_khien_i2c, BH1750_DIA_CHI, I2C_TIMEOUT_DEFAULT_MS);

    if (kiem_tra_bh1750 != I2C_STATUS_OK)
    {
        bh1750_ket_noi = 0U;
        return;
    }

    trang_thai_bh1750_khoi_tao = BH1750_KhoiTao(&bo_dieu_khien_i2c);
    if (trang_thai_bh1750_khoi_tao == BH1750_THANH_CONG)
    {
        Tre_Ms(BH1750_CHO_DO_MS);
        bh1750_ket_noi = 1U;
    }
    else
    {
        bh1750_ket_noi = 0U;
    }
}

static void SHT30_ThuKetNoiLai(void)
{
    kiem_tra_sht30 = I2C_IsDeviceReady(
        &bo_dieu_khien_i2c, SHT30_DIA_CHI, I2C_TIMEOUT_DEFAULT_MS);

    sht30_ket_noi = (kiem_tra_sht30 == I2C_STATUS_OK) ? 1U : 0U;
}

int main(void)
{
    float lux_tam = 0.0f;
    SHT30_DuLieu du_lieu_tam;

    /* I2C1 mac dinh: PB6=SCL, PB7=SDA; 100 kHz */
    bo_dieu_khien_i2c.pI2Cx = I2C1;
    bo_dieu_khien_i2c.I2C_Config.I2C_SCLSpeed = I2C_SCL_SPEED_SM;
    bo_dieu_khien_i2c.I2C_Config.I2C_ACKControl = I2C_ACK_ENABLE;
    bo_dieu_khien_i2c.I2C_Config.I2C_FMDutyCycle = I2C_FM_DUTY_2;

    trang_thai_i2c = I2C_Init(&bo_dieu_khien_i2c);
    if (trang_thai_i2c != I2C_STATUS_OK)
    {
        /* Xem trang_thai_i2c trong Keil Watch neu I2C_Init loi. */
        while (1)
        {
        }
    }

    BH1750_ThuKetNoiLai();
    SHT30_ThuKetNoiLai();

    while (1)
    {
        bh1750_du_lieu_moi = 0U;
        sht30_du_lieu_moi = 0U;

        /* 1. BH1750: mat ket noi -> thu lai moi vong. */
        if (bh1750_ket_noi == 0U)
        {
            BH1750_ThuKetNoiLai();
        }

        if (bh1750_ket_noi == 1U)
        {
            trang_thai_bh1750_doc = BH1750_DocLux(
                &bo_dieu_khien_i2c, &lux_tam);

            if (trang_thai_bh1750_doc == BH1750_THANH_CONG)
            {
                anh_sang_lux = lux_tam;
                bh1750_du_lieu_moi = 1U;
            }
            else
            {
                bh1750_ket_noi = 0U;
            }
        }

        /* 2. SHT30: gui lenh 0x2400, cho 20 ms, doc 6 byte + CRC. */
        if (sht30_ket_noi == 0U)
        {
            SHT30_ThuKetNoiLai();
        }

        if (sht30_ket_noi == 1U)
        {
            trang_thai_sht30_bat_dau = SHT30_BatDauDo(
                &bo_dieu_khien_i2c);

            if (trang_thai_sht30_bat_dau == SHT30_THANH_CONG)
            {
                Tre_Ms(SHT30_CHO_DO_MS);
                trang_thai_sht30_doc = SHT30_DocDuLieu(
                    &bo_dieu_khien_i2c, &du_lieu_tam);

                if (trang_thai_sht30_doc == SHT30_THANH_CONG)
                {
                    nhiet_do_c = du_lieu_tam.nhiet_do_c;
                    do_am_rh = du_lieu_tam.do_am_rh;
                    nhiet_do_raw = du_lieu_tam.nhiet_do_tho;
                    do_am_raw = du_lieu_tam.do_am_tho;
                    sht30_du_lieu_moi = 1U;
                }
                else
                {
                    sht30_ket_noi = 0U;
                }
            }
            else
            {
                sht30_ket_noi = 0U;
            }
        }

        so_lan_do++;
        Tre_Ms(CHU_KY_DOC_MS);
    }
}

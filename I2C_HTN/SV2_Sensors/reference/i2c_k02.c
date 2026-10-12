#include "i2c_k02.h"


#define I2C_TAN_SO         100000U
#define I2C_GIOI_HAN_CHO   100000U


/* =========================================================
 * Lay tan so clock APB1
 * ========================================================= */
static uint32_t I2C_LayTanSoPCLK1(void)
{
    uint32_t tan_so_pclk1;
    uint32_t bo_chia_apb1;

    tan_so_pclk1 = SystemCoreClock;

    /*
     * PPRE1 = RCC->CFGR bit [10:8]
     *
     * 000 -> HCLK / 1
     * 100 -> HCLK / 2
     * 101 -> HCLK / 4
     * 110 -> HCLK / 8
     * 111 -> HCLK / 16
     */
    bo_chia_apb1 =
        (RCC->CFGR >> 8) & 0x07U;

    if (bo_chia_apb1 >= 4U)
    {
        tan_so_pclk1 >>=
            (bo_chia_apb1 - 3U);
    }

    return tan_so_pclk1;
}


/* =========================================================
 * Xoa cac co loi I2C
 * ========================================================= */
static void I2C_XoaCoLoi(void)
{
    I2C1->SR1 &= ~(I2C_SR1_AF   |
                   I2C_SR1_BERR |
                   I2C_SR1_ARLO |
                   I2C_SR1_OVR);
}


/* =========================================================
 * Cho STOP duoc hardware xoa
 * ========================================================= */
static I2C_TrangThai I2C_ChoStopHoanTat(void)
{
    uint32_t bo_dem_cho;

    bo_dem_cho = I2C_GIOI_HAN_CHO;

    while (I2C1->CR1 & I2C_CR1_STOP)
    {
        if (--bo_dem_cho == 0U)
        {
            return I2C_LOI_TIMEOUT;
        }
    }

    return I2C_THANH_CONG;
}


/* =========================================================
 * Khoi phuc trang thai sau khi doc bi loi
 * ========================================================= */
static void I2C_KhoiPhucSauDoc(void)
{
    /* Tao STOP neu can */
    I2C1->CR1 |= I2C_CR1_STOP;

    /* POS ve 0 */
    I2C1->CR1 &= ~I2C_CR1_POS;

    /* Bat lai ACK */
    I2C1->CR1 |= I2C_CR1_ACK;
}


/* =========================================================
 * Khoi tao I2C1
 *
 * STM32F103C8T6
 *
 * PB6 = SCL
 * PB7 = SDA
 * 100 kHz
 * ========================================================= */
void I2C_KhoiTao(void)
{
    uint32_t tan_so_pclk1;
    uint32_t tan_so_mhz;
    uint32_t gia_tri_ccr;


    /* Cap nhat SystemCoreClock */
    SystemCoreClockUpdate();


    /* Bat clock AFIO */
    RCC->APB2ENR |=
        RCC_APB2ENR_AFIOEN;


    /* Bat clock GPIOB */
    RCC->APB2ENR |=
        RCC_APB2ENR_IOPBEN;


    /*
     * I2C1 mac dinh:
     *
     * PB6 = SCL
     * PB7 = SDA
     */
    AFIO->MAPR &=
        ~AFIO_MAPR_I2C1_REMAP;


    /*
     * PB6 va PB7:
     *
     * MODE = 11
     * CNF  = 11
     *
     * Alternate Function
     * Open Drain
     * 50 MHz
     */
    GPIOB->CRL &=
        ~((0xFU << 24) |
          (0xFU << 28));

    GPIOB->CRL |=
        ((0xFU << 24) |
         (0xFU << 28));


    /* Bat clock I2C1 */
    RCC->APB1ENR |=
        RCC_APB1ENR_I2C1EN;


    /* Reset I2C1 */
    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;


    /* Tat peripheral khi cau hinh */
    I2C1->CR1 &= ~I2C_CR1_PE;


    /* Lay clock APB1 */
    tan_so_pclk1 =
        I2C_LayTanSoPCLK1();

    tan_so_mhz =
        tan_so_pclk1 / 1000000U;


    /*
     * CR2.FREQ = APB1 MHz
     */
    I2C1->CR2 &=
        ~I2C_CR2_FREQ;

    I2C1->CR2 |=
        (tan_so_mhz & 0x3FU);


    /*
     * Standard Mode:
     *
     * CCR =
     * PCLK1 / (2 x F_SCL)
     */
    gia_tri_ccr =
        tan_so_pclk1 /
        (2U * I2C_TAN_SO);

    if (gia_tri_ccr < 4U)
    {
        gia_tri_ccr = 4U;
    }

    I2C1->CCR =
        gia_tri_ccr;


    /*
     * Standard Mode:
     *
     * TRISE = FREQ + 1
     */
    I2C1->TRISE =
        tan_so_mhz + 1U;


    /*
     * Bit 14 OAR1 phai bang 1
     * tren STM32F1
     */
    I2C1->OAR1 =
        (1U << 14);


    /* Xoa co loi cu */
    I2C_XoaCoLoi();


    /* POS mac dinh = 0 */
    I2C1->CR1 &=
        ~I2C_CR1_POS;


    /* Bat ACK mac dinh */
    I2C1->CR1 |=
        I2C_CR1_ACK;


    /* Bat peripheral */
    I2C1->CR1 |=
        I2C_CR1_PE;
}


/* =========================================================
 * Tao START
 * ========================================================= */
I2C_TrangThai I2C_BatDau(void)
{
    uint32_t bo_dem_cho;


    /* Cho bus ranh */
    bo_dem_cho =
        I2C_GIOI_HAN_CHO;

    while (I2C1->SR2 & I2C_SR2_BUSY)
    {
        if (--bo_dem_cho == 0U)
        {
            return I2C_LOI_TIMEOUT;
        }
    }


    I2C_XoaCoLoi();


    /* Tao START */
    I2C1->CR1 |=
        I2C_CR1_START;


    /* Cho SB */
    bo_dem_cho =
        I2C_GIOI_HAN_CHO;

    while (!(I2C1->SR1 & I2C_SR1_SB))
    {
        if (I2C1->SR1 & I2C_SR1_BERR)
        {
            I2C1->SR1 &=
                ~I2C_SR1_BERR;

            return I2C_LOI_BUS;
        }


        if (I2C1->SR1 & I2C_SR1_ARLO)
        {
            I2C1->SR1 &=
                ~I2C_SR1_ARLO;

            return I2C_LOI_TRANH_CHAP;
        }


        if (--bo_dem_cho == 0U)
        {
            return I2C_LOI_TIMEOUT;
        }
    }


    return I2C_THANH_CONG;
}


/* =========================================================
 * Tao STOP
 * ========================================================= */
void I2C_Dung(void)
{
    I2C1->CR1 |=
        I2C_CR1_STOP;
}


/* =========================================================
 * Gui dia chi slave 7-bit
 *
 * doc_ghi = 0 : ghi
 * doc_ghi = 1 : doc
 * ========================================================= */
I2C_TrangThai I2C_GuiDiaChi(uint8_t dia_chi,
                            uint8_t doc_ghi)
{
    uint32_t bo_dem_cho;
    volatile uint32_t bien_tam;


    I2C1->DR =
        ((uint32_t)dia_chi << 1) |
        (doc_ghi & 0x01U);


    bo_dem_cho =
        I2C_GIOI_HAN_CHO;


    while (!(I2C1->SR1 & I2C_SR1_ADDR))
    {
        /* Slave NACK */
        if (I2C1->SR1 & I2C_SR1_AF)
        {
            I2C1->SR1 &=
                ~I2C_SR1_AF;

            I2C_Dung();

            return I2C_LOI_NACK;
        }


        /* Bus error */
        if (I2C1->SR1 & I2C_SR1_BERR)
        {
            I2C1->SR1 &=
                ~I2C_SR1_BERR;

            I2C_Dung();

            return I2C_LOI_BUS;
        }


        /* Arbitration lost */
        if (I2C1->SR1 & I2C_SR1_ARLO)
        {
            I2C1->SR1 &=
                ~I2C_SR1_ARLO;

            return I2C_LOI_TRANH_CHAP;
        }


        if (--bo_dem_cho == 0U)
        {
            I2C_Dung();

            return I2C_LOI_TIMEOUT;
        }
    }


    /*
     * Clear ADDR:
     *
     * doc SR1
     * sau do doc SR2
     */
    bien_tam = I2C1->SR1;
    bien_tam = I2C1->SR2;

    (void)bien_tam;


    return I2C_THANH_CONG;
}


/* =========================================================
 * Gui 1 byte
 * ========================================================= */
I2C_TrangThai I2C_GuiByte(uint8_t du_lieu)
{
    uint32_t bo_dem_cho;


    /* Cho TXE */
    bo_dem_cho =
        I2C_GIOI_HAN_CHO;

    while (!(I2C1->SR1 & I2C_SR1_TXE))
    {
        if (I2C1->SR1 & I2C_SR1_AF)
        {
            I2C1->SR1 &=
                ~I2C_SR1_AF;

            return I2C_LOI_NACK;
        }


        if (I2C1->SR1 & I2C_SR1_BERR)
        {
            I2C1->SR1 &=
                ~I2C_SR1_BERR;

            return I2C_LOI_BUS;
        }


        if (I2C1->SR1 & I2C_SR1_ARLO)
        {
            I2C1->SR1 &=
                ~I2C_SR1_ARLO;

            return I2C_LOI_TRANH_CHAP;
        }


        if (--bo_dem_cho == 0U)
        {
            return I2C_LOI_TIMEOUT;
        }
    }


    /* Ghi byte */
    I2C1->DR =
        du_lieu;


    /*
     * Cho BTF de dam bao
     * byte da gui hoan tat.
     */
    bo_dem_cho =
        I2C_GIOI_HAN_CHO;

    while (!(I2C1->SR1 & I2C_SR1_BTF))
    {
        if (I2C1->SR1 & I2C_SR1_AF)
        {
            I2C1->SR1 &=
                ~I2C_SR1_AF;

            return I2C_LOI_NACK;
        }


        if (I2C1->SR1 & I2C_SR1_BERR)
        {
            I2C1->SR1 &=
                ~I2C_SR1_BERR;

            return I2C_LOI_BUS;
        }


        if (I2C1->SR1 & I2C_SR1_ARLO)
        {
            I2C1->SR1 &=
                ~I2C_SR1_ARLO;

            return I2C_LOI_TRANH_CHAP;
        }


        if (--bo_dem_cho == 0U)
        {
            return I2C_LOI_TIMEOUT;
        }
    }


    return I2C_THANH_CONG;
}


/* =========================================================
 * Ghi mot mang du lieu
 * ========================================================= */
I2C_TrangThai I2C_GhiMang(uint8_t dia_chi,
                          const uint8_t *du_lieu,
                          uint16_t so_byte)
{
    I2C_TrangThai trang_thai;
    uint16_t vi_tri;


    if ((du_lieu == 0) ||
        (so_byte == 0U))
    {
        return I2C_LOI_THAM_SO;
    }


    /* START */
    trang_thai =
        I2C_BatDau();

    if (trang_thai != I2C_THANH_CONG)
    {
        return trang_thai;
    }


    /* Dia chi + WRITE */
    trang_thai =
        I2C_GuiDiaChi(dia_chi, 0U);

    if (trang_thai != I2C_THANH_CONG)
    {
        return trang_thai;
    }


    /* Gui tung byte */
    for (vi_tri = 0U;
         vi_tri < so_byte;
         vi_tri++)
    {
        trang_thai =
            I2C_GuiByte(
                du_lieu[vi_tri]);

        if (trang_thai != I2C_THANH_CONG)
        {
            I2C_Dung();

            return trang_thai;
        }
    }


    /* STOP */
    I2C_Dung();


    return I2C_THANH_CONG;
}


/* =========================================================
 * Doc mot mang du lieu
 *
 * Xu ly rieng:
 *
 * 1 byte
 * 2 byte
 * >= 3 byte
 * ========================================================= */
I2C_TrangThai I2C_DocMang(uint8_t dia_chi,
                          uint8_t *du_lieu,
                          uint16_t so_byte)
{
    I2C_TrangThai trang_thai;
    uint32_t bo_dem_cho;
    uint32_t trang_thai_ngat;

    volatile uint32_t bien_tam;

    uint16_t vi_tri;
    uint16_t con_lai;


    if ((du_lieu == 0) ||
        (so_byte == 0U))
    {
        return I2C_LOI_THAM_SO;
    }


    vi_tri = 0U;


    /*
     * Trang thai mac dinh
     * truoc khi bat dau doc.
     */
    I2C1->CR1 &=
        ~I2C_CR1_POS;

    I2C1->CR1 |=
        I2C_CR1_ACK;


    /* START */
    trang_thai =
        I2C_BatDau();

    if (trang_thai != I2C_THANH_CONG)
    {
        return trang_thai;
    }


    /*
     * Gui dia chi + READ.
     *
     * Khong dung I2C_GuiDiaChi()
     * vi ham do se clear ADDR ngay.
     *
     * Khi receive, ACK/POS phai duoc
     * xu ly truoc khi clear ADDR.
     */
    I2C1->DR =
        ((uint32_t)dia_chi << 1) |
        1U;


    /* Cho ADDR */
    bo_dem_cho =
        I2C_GIOI_HAN_CHO;

    while (!(I2C1->SR1 & I2C_SR1_ADDR))
    {
        if (I2C1->SR1 & I2C_SR1_AF)
        {
            I2C1->SR1 &=
                ~I2C_SR1_AF;

            I2C_Dung();

            return I2C_LOI_NACK;
        }


        if (I2C1->SR1 & I2C_SR1_BERR)
        {
            I2C1->SR1 &=
                ~I2C_SR1_BERR;

            I2C_Dung();

            return I2C_LOI_BUS;
        }


        if (I2C1->SR1 & I2C_SR1_ARLO)
        {
            I2C1->SR1 &=
                ~I2C_SR1_ARLO;

            return I2C_LOI_TRANH_CHAP;
        }


        if (--bo_dem_cho == 0U)
        {
            I2C_Dung();

            return I2C_LOI_TIMEOUT;
        }
    }


    /* =====================================================
     * TRUONG HOP 1 BYTE
     * ===================================================== */
    if (so_byte == 1U)
    {
        /*
         * Byte duy nhat phai nhan NACK.
         */
        I2C1->CR1 &=
            ~I2C_CR1_ACK;


        /*
         * Clear ADDR va tao STOP
         * trong critical section.
         */
        trang_thai_ngat =
            __get_PRIMASK();

        __disable_irq();


        bien_tam = I2C1->SR1;
        bien_tam = I2C1->SR2;
        (void)bien_tam;


        I2C1->CR1 |=
            I2C_CR1_STOP;


        if (trang_thai_ngat == 0U)
        {
            __enable_irq();
        }


        /* Cho byte vao DR */
        bo_dem_cho =
            I2C_GIOI_HAN_CHO;

        while (!(I2C1->SR1 &
                 I2C_SR1_RXNE))
        {
            if (--bo_dem_cho == 0U)
            {
                I2C_KhoiPhucSauDoc();

                return I2C_LOI_TIMEOUT;
            }
        }


        /* Doc byte */
        du_lieu[0] =
            (uint8_t)I2C1->DR;


        /* Cho STOP hoan tat */
        trang_thai =
            I2C_ChoStopHoanTat();

        if (trang_thai != I2C_THANH_CONG)
        {
            I2C_KhoiPhucSauDoc();

            return trang_thai;
        }


        /* Khoi phuc ACK */
        I2C1->CR1 |=
            I2C_CR1_ACK;


        return I2C_THANH_CONG;
    }


    /* =====================================================
     * TRUONG HOP 2 BYTE
     * ===================================================== */
    if (so_byte == 2U)
    {
        /*
         * POS = 1:
         * NACK se ap dung cho byte tiep theo.
         */
        I2C1->CR1 |=
            I2C_CR1_POS;


        /*
         * Clear ADDR va ACK = 0
         * trong critical section.
         */
        trang_thai_ngat =
            __get_PRIMASK();

        __disable_irq();


        bien_tam = I2C1->SR1;
        bien_tam = I2C1->SR2;
        (void)bien_tam;


        I2C1->CR1 &=
            ~I2C_CR1_ACK;


        if (trang_thai_ngat == 0U)
        {
            __enable_irq();
        }


        /* Cho ca 2 byte san sang */
        bo_dem_cho =
            I2C_GIOI_HAN_CHO;

        while (!(I2C1->SR1 &
                 I2C_SR1_BTF))
        {
            if (--bo_dem_cho == 0U)
            {
                I2C_KhoiPhucSauDoc();

                return I2C_LOI_TIMEOUT;
            }
        }


        /*
         * STOP + doc byte 1
         * trong critical section.
         */
        trang_thai_ngat =
            __get_PRIMASK();

        __disable_irq();


        I2C1->CR1 |=
            I2C_CR1_STOP;


        du_lieu[0] =
            (uint8_t)I2C1->DR;


        if (trang_thai_ngat == 0U)
        {
            __enable_irq();
        }


        /* Doc byte 2 */
        du_lieu[1] =
            (uint8_t)I2C1->DR;


        /* Cho STOP hoan tat */
        trang_thai =
            I2C_ChoStopHoanTat();

        if (trang_thai != I2C_THANH_CONG)
        {
            I2C_KhoiPhucSauDoc();

            return trang_thai;
        }


        /* Khoi phuc */
        I2C1->CR1 &=
            ~I2C_CR1_POS;

        I2C1->CR1 |=
            I2C_CR1_ACK;


        return I2C_THANH_CONG;
    }


    /* =====================================================
     * TRUONG HOP >= 3 BYTE
     * ===================================================== */

    /*
     * POS = 0
     * ACK = 1
     */
    I2C1->CR1 &=
        ~I2C_CR1_POS;

    I2C1->CR1 |=
        I2C_CR1_ACK;


    /* Clear ADDR */
    bien_tam = I2C1->SR1;
    bien_tam = I2C1->SR2;
    (void)bien_tam;


    con_lai =
        so_byte;


    /*
     * Doc cac byte dau.
     *
     * Dung lai khi chi con
     * dung 3 byte.
     */
    while (con_lai > 3U)
    {
        bo_dem_cho =
            I2C_GIOI_HAN_CHO;


        while (!(I2C1->SR1 &
                 I2C_SR1_RXNE))
        {
            if (I2C1->SR1 & I2C_SR1_BERR)
            {
                I2C1->SR1 &=
                    ~I2C_SR1_BERR;

                I2C_KhoiPhucSauDoc();

                return I2C_LOI_BUS;
            }


            if (I2C1->SR1 & I2C_SR1_ARLO)
            {
                I2C1->SR1 &=
                    ~I2C_SR1_ARLO;

                I2C_KhoiPhucSauDoc();

                return I2C_LOI_TRANH_CHAP;
            }


            if (--bo_dem_cho == 0U)
            {
                I2C_KhoiPhucSauDoc();

                return I2C_LOI_TIMEOUT;
            }
        }


        du_lieu[vi_tri++] =
            (uint8_t)I2C1->DR;


        con_lai--;
    }


    /* =====================================================
     * CON LAI DUNG 3 BYTE
     *
     * Data N-2
     * Data N-1
     * Data N
     * ===================================================== */


    /*
     * Cho:
     *
     * DR            = Data N-2
     * Shift register = Data N-1
     *
     * Khi do BTF = 1.
     */
    bo_dem_cho =
        I2C_GIOI_HAN_CHO;

    while (!(I2C1->SR1 &
             I2C_SR1_BTF))
    {
        if (--bo_dem_cho == 0U)
        {
            I2C_KhoiPhucSauDoc();

            return I2C_LOI_TIMEOUT;
        }
    }


    /*
     * Tat ACK truoc khi doc N-2.
     *
     * Byte N se duoc nhan voi NACK.
     */
    I2C1->CR1 &=
        ~I2C_CR1_ACK;


    /*
     * Doan quan trong:
     *
     * Doc N-2
     * STOP
     * Doc N-1
     *
     * phai dien ra lien tiep.
     */
    trang_thai_ngat =
        __get_PRIMASK();

    __disable_irq();


    /* Doc Data N-2 */
    du_lieu[vi_tri++] =
        (uint8_t)I2C1->DR;


    /*
     * Sau khi doc N-2,
     * Data N bat dau duoc dich vao.
     */
    I2C1->CR1 |=
        I2C_CR1_STOP;


    /* Doc Data N-1 */
    du_lieu[vi_tri++] =
        (uint8_t)I2C1->DR;


    if (trang_thai_ngat == 0U)
    {
        __enable_irq();
    }


    /*
     * Cho Data N vao DR.
     */
    bo_dem_cho =
        I2C_GIOI_HAN_CHO;

    while (!(I2C1->SR1 &
             I2C_SR1_RXNE))
    {
        if (--bo_dem_cho == 0U)
        {
            I2C_KhoiPhucSauDoc();

            return I2C_LOI_TIMEOUT;
        }
    }


    /* Doc Data N */
    du_lieu[vi_tri++] =
        (uint8_t)I2C1->DR;


    /* Cho STOP hoan tat */
    trang_thai =
        I2C_ChoStopHoanTat();

    if (trang_thai != I2C_THANH_CONG)
    {
        I2C_KhoiPhucSauDoc();

        return trang_thai;
    }


    /* Khoi phuc trang thai */
    I2C1->CR1 &=
        ~I2C_CR1_POS;

    I2C1->CR1 |=
        I2C_CR1_ACK;


    return I2C_THANH_CONG;
}


/* =========================================================
 * Kiem tra thiet bi tren bus
 * ========================================================= */
I2C_TrangThai I2C_KiemTraThietBi(uint8_t dia_chi)
{
    I2C_TrangThai trang_thai;


    /* START */
    trang_thai =
        I2C_BatDau();

    if (trang_thai != I2C_THANH_CONG)
    {
        return trang_thai;
    }


    /* Gui dia chi + WRITE */
    trang_thai =
        I2C_GuiDiaChi(
            dia_chi,
            0U);

    if (trang_thai != I2C_THANH_CONG)
    {
        return trang_thai;
    }


    /* STOP */
    I2C_Dung();


    return I2C_THANH_CONG;
}
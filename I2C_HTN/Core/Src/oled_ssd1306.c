#include "oled_ssd1306.h"
#include "oled_graphics.h"
#include <stddef.h>

// Byte điều khiển và kích thước mỗi phần dữ liệu ảnh
#define OLED_CONTROL_COMMAND  0x00U
#define OLED_CONTROL_DATA     0x40U
#define OLED_DATA_CHUNK_SIZE    32U

// Bus được truyền vào khi khởi tạo và trạng thái màn hình
static I2C_Handle_t *OLED_I2CHandle = NULL;
static uint8_t OLED_Initialized = 0U;

// Đổi kết quả I2C sang kết quả OLED
static OLED_Status OLED_FromI2CStatus(I2C_Status_t status)
{
    if (status == I2C_STATUS_OK)
    {
        return OLED_OK;
    }
    if (status == I2C_STATUS_TIMEOUT)
    {
        return OLED_TIMEOUT;
    }
    if (status == I2C_STATUS_INVALID_ARG)
    {
        return OLED_INVALID_ARGUMENT;
    }

    return OLED_ERROR;
}

// Gửi một byte lệnh hoặc tham số lệnh
static OLED_Status OLED_WriteCommand(uint8_t command)
{
    uint8_t packet[2];
    I2C_Status_t status;

    packet[0] = OLED_CONTROL_COMMAND;
    packet[1] = command;

    status = I2C_Master_Transmit(OLED_I2CHandle,
                                 OLED_I2C_ADDRESS,
                                 packet,
                                 sizeof(packet),
                                 OLED_TIMEOUT_MS);

    return OLED_FromI2CStatus(status);
}

// Gửi lần lượt các byte lệnh, dừng khi có lỗi
static OLED_Status OLED_WriteCommands(const uint8_t *commands,
                                      uint16_t count)
{
    uint16_t i;
    OLED_Status status;

    if (commands == NULL || count == 0U)
    {
        return OLED_INVALID_ARGUMENT;
    }

    for (i = 0U; i < count; i++)
    {
        status = OLED_WriteCommand(commands[i]);
        if (status != OLED_OK)
        {
            return status;
        }
    }

    return OLED_OK;
}

// Gửi dữ liệu ảnh thành các phần, mỗi phần tối đa 32 byte
static OLED_Status OLED_WriteData(const uint8_t *data, uint16_t length)
{
    uint8_t packet[OLED_DATA_CHUNK_SIZE + 1U];
    uint16_t offset = 0U;
    uint16_t count;
    uint16_t i;
    I2C_Status_t i2c_status;
    OLED_Status status;

    if (data == NULL || length == 0U)
    {
        return OLED_INVALID_ARGUMENT;
    }

    packet[0] = OLED_CONTROL_DATA;

    while (offset < length)
    {
        count = length - offset;
        if (count > OLED_DATA_CHUNK_SIZE)
        {
            count = OLED_DATA_CHUNK_SIZE;
        }

        for (i = 0U; i < count; i++)
        {
            packet[i + 1U] = data[offset + i];
        }

        i2c_status = I2C_Master_Transmit(OLED_I2CHandle,
                                        OLED_I2C_ADDRESS,
                                        packet,
                                        count + 1U,
                                        OLED_TIMEOUT_MS);
        status = OLED_FromI2CStatus(i2c_status);
        if (status != OLED_OK)
        {
            return status;
        }

        offset += count;
    }

    return OLED_OK;
}

// Chọn toàn bộ vùng ảnh rồi gửi framebuffer
static OLED_Status OLED_SendFramebuffer(void)
{
    static const uint8_t window[] =
    {
        0x21U, 0x00U, 0x7FU,  // Cột 0 đến 127
        0x22U, 0x00U, 0x07U   // Page 0 đến 7
    };
    OLED_Status status;

    status = OLED_WriteCommands(window, sizeof(window));
    if (status != OLED_OK)
    {
        return status;
    }

    return OLED_WriteData(OLED_GetFramebuffer(), OLED_BUFFER_SIZE);
}

// Cấu hình SSD1306 128x64 dùng charge pump nội
OLED_Status OLED_Init(I2C_Handle_t *i2c_handle)
{
    static const uint8_t commands[] =
    {
        0xAEU,               // Tắt hiển thị khi cấu hình
        0xD5U, 0x80U,        // Clock hiển thị
        0xA8U, 0x3FU,        // 64 hàng
        0xD3U, 0x00U,        // Không dịch ảnh theo chiều dọc
        0x40U,               // Dòng bắt đầu bằng 0
        0x8DU, 0x14U,        // Bật charge pump nội
        0x20U, 0x00U,        // Chế độ ghi địa chỉ ngang
        0xA1U,               // Ánh xạ cột
        0xC8U,               // Hướng quét COM
        0xDAU, 0x12U,        // Cấu hình COM cho 128x64
        0x81U, 0x7FU,        // Độ tương phản
        0xD9U, 0xF1U,        // Thời gian pre-charge
        0xDBU, 0x20U,        // Mức VCOMH
        0xA4U,               // Hiển thị nội dung GDDRAM
        0xA6U,               // Hiển thị bình thường
        0x2EU                // Tắt cuộn ảnh
    };
    OLED_Status status;

    OLED_Initialized = 0U;
    OLED_I2CHandle = NULL;

    if (i2c_handle == NULL)
    {
        return OLED_INVALID_ARGUMENT;
    }

    OLED_I2CHandle = i2c_handle;

    status = OLED_WriteCommands(commands, sizeof(commands));
    if (status != OLED_OK)
    {
        return status;
    }

    OLED_Clear();
    status = OLED_SendFramebuffer();
    if (status != OLED_OK)
    {
        return status;
    }

    status = OLED_WriteCommand(0xAFU);
    if (status != OLED_OK)
    {
        return status;
    }

    OLED_Initialized = 1U;
    return OLED_OK;
}

// Gửi ảnh hiện tại, không thay đổi framebuffer
OLED_Status OLED_Update(void)
{
    if (OLED_Initialized == 0U)
    {
        return OLED_NOT_READY;
    }

    return OLED_SendFramebuffer();
}

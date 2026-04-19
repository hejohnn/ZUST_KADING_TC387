/*********************************************************************************************************************
* PD-1503-SPI-16 角度绝对式编码器驱动
*
* 文件名称          zf_device_pd1503
* 适用平台          TC387QP
*
* SPI 协议说明:
*   1. CS 拉低
*   2. MOSI 发送 0x05 (STREAM 命令)
*   3. 连续读取 4 字节: [Angle_H][Angle_L][Status][CRC8]
*   4. CRC8 校验 (SAE J1850), 不通过则重试 (最多 6 次)
*   5. CS 拉高
*   6. 角度换算: angle(°) = 360 * Count / 65536
*
* SPI 配置: Mode 0 (CPOL=0, CPHA=0), MSB First, 8bit, 最大 10MHz
*
* 修改记录
* 日期              作者                备注
* 2024-01-01       user               first version
********************************************************************************************************************/
/*********************************************************************************************************************
* 接线定义：
*                   ------------------------------------
*                   模块管脚            单片机管脚
*                   SCK                 P22.3
*                   MOSI                P22.0
*                   MISO                P22.1
*                   CS                  P22.2
*                   VCC                 3.3V电源
*                   GND                 电源地
*                   ------------------------------------
********************************************************************************************************************/

#include "zf_common_debug.h"
#include "zf_driver_delay.h"
#include "zf_driver_gpio.h"
#include "zf_driver_soft_spi.h"
#include "zf_driver_spi.h"

#include "zf_device_pd1503.h"

//=================================================内部变量定义=====================================================
static uint16 pd1503_last_count  = 0;                                           // 上次角度计数值
static uint16 pd1503_now_count   = 0;                                           // 当前角度计数值

#if PD1503_USE_SOFT_SPI
static soft_spi_info_struct pd1503_spi;
#define pd1503_spi_read()               (soft_spi_read_8bit(&pd1503_spi))
#define pd1503_spi_write(data)          (soft_spi_write_8bit(&pd1503_spi, (data)))
#else
#define pd1503_spi_read()               (spi_read_8bit(PD1503_SPI))
#define pd1503_spi_write(data)          (spi_write_8bit(PD1503_SPI, (data)))
#endif

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     SPI 全双工传输多字节 (在一个 CS 周期内完成)
//-------------------------------------------------------------------------------------------------------------------
static void pd1503_spi_transfer_bytes (const uint8 *tx_buf, uint8 *rx_buf, uint32 len)
{
#if PD1503_USE_SOFT_SPI
    soft_spi_transfer_8bit(&pd1503_spi, tx_buf, rx_buf, len);
#else
    spi_transfer_8bit(PD1503_SPI, tx_buf, rx_buf, len);
#endif
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     CRC8 SAE J1850 校验计算
//-------------------------------------------------------------------------------------------------------------------
uint8 pd1503_crc8_sae_j1850 (const uint8 *data, uint8 length)
{
    uint8 crc = PD1503_CRC8_INIT;
    uint8 i, j;

    for (i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x80)
            {
                crc = (uint8)((crc << 1) ^ PD1503_CRC8_POLY);
            }
            else
            {
                crc = (uint8)(crc << 1);
            }
        }
    }
    crc ^= PD1503_CRC8_XOR_OUT;
    return crc;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     奇偶校验
//-------------------------------------------------------------------------------------------------------------------
uint8 pd1503_verify_parity (uint16 angle_raw, uint8 status)
{
    uint8 ones = 0;
    uint16 temp16;
    uint8  temp8;

    temp16 = angle_raw;
    while (temp16)
    {
        ones += (temp16 & 1);
        temp16 >>= 1;
    }

    temp8 = (uint8)(status & 0xFE);
    while (temp8)
    {
        ones += (temp8 & 1);
        temp8 >>= 1;
    }

    if ((ones & 0x01) == (status & PD1503_STATUS_PARITY_BIT))
    {
        return 1;
    }
    return 0;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     读取编码器角度 (完整数据, 含 CRC 重试机制)
//-------------------------------------------------------------------------------------------------------------------
uint8 pd1503_read_angle (pd1503_data_struct *pd1503_data)
{
    uint8 tx_buf[5] = {PD1503_CMD_STREAM, 0x00, 0x00, 0x00, 0x00};
    uint8 rx_buf[5] = {0};
    uint8 calc_crc;
    uint8 retry;
    uint8 crc_pass = 0;

    for (retry = 0; retry < PD1503_MAX_RETRY; retry++)
    {
        PD1503_CSN(0);
        system_delay_us(1);

        pd1503_spi_transfer_bytes(tx_buf, rx_buf, 5);

        system_delay_us(1);
        PD1503_CSN(1);
        system_delay_us(1);

        calc_crc = pd1503_crc8_sae_j1850(&rx_buf[1], 3);

        if (calc_crc == rx_buf[4])
        {
            crc_pass = 1;
            break;
        }
    }

    if (!crc_pass)
    {
        pd1503_data->angle_count = 0;
        pd1503_data->angle_deg   = 0.0f;
        pd1503_data->status      = 0;
        pd1503_data->crc         = 0;
        pd1503_data->crc_ok      = 0;
        pd1503_data->mag_error   = 0;
        pd1503_data->parity_ok   = 0;
        return 1;
    }

    pd1503_data->angle_count = ((uint16)rx_buf[1] << 8) | (uint16)rx_buf[2];
    pd1503_data->angle_deg   = 360.0f * (float)pd1503_data->angle_count / PD1503_RESOLUTION;
    pd1503_data->status      = rx_buf[3];
    pd1503_data->crc         = rx_buf[4];
    pd1503_data->crc_ok      = 1;

    pd1503_data->mag_error = (rx_buf[3] & PD1503_STATUS_MAG_ERR_BIT) ? 1 : 0;
    pd1503_data->parity_ok = pd1503_verify_parity(pd1503_data->angle_count, rx_buf[3]);

    pd1503_last_count = pd1503_now_count;
    pd1503_now_count  = pd1503_data->angle_count;

    if (!pd1503_data->parity_ok)
    {
        return 3;
    }
    return 0;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     快速获取原始角度计数值 (16bit)
//-------------------------------------------------------------------------------------------------------------------
uint16 pd1503_get_count (void)
{
    pd1503_data_struct data;
    pd1503_read_angle(&data);
    return data.angle_count;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     快速获取角度值 (浮点, 单位°)
//-------------------------------------------------------------------------------------------------------------------
float pd1503_get_angle_deg (void)
{
    pd1503_data_struct data;
    pd1503_read_angle(&data);
    return data.angle_deg;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     PD-1503 编码器初始化
//-------------------------------------------------------------------------------------------------------------------
uint8 pd1503_init (void)
{
    uint8 return_state = 0;

#if PD1503_USE_SOFT_SPI
    soft_spi_init(
        &pd1503_spi,
        0,
        PD1503_SOFT_SPI_DELAY,
        PD1503_SCLK_PIN,
        PD1503_MOSI_PIN,
        PD1503_MISO_PIN,
        SOFT_SPI_PIN_NULL
    );
#else
    spi_init(
        PD1503_SPI,
        SPI_MODE0,
        PD1503_SPI_SPEED,
        PD1503_SCLK_PIN,
        PD1503_MOSI_PIN,
        PD1503_MISO_PIN,
        SPI_CS_NULL
    );
#endif

    gpio_init(PD1503_CS_PIN, GPO, GPIO_HIGH, GPO_PUSH_PULL);

    pd1503_data_struct test_data;
    uint8 test_result = pd1503_read_angle(&test_data);

    if (test_result == 1)
    {
        return_state = 1;
        zf_log(0, "pd1503 encoder init error: SPI communication failed.");
    }
    else if (test_result == 2)
    {
        zf_log(0, "pd1503 encoder warning: magnetic field error.");
    }

    return return_state;
}
/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include <drivers/spi.h>
#include <device.h>
#include <soc.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#include "st25r3911b_spi.h"
#include "st25r3911b_reg.h"

LOG_MODULE_REGISTER(st25r3911b, CONFIG_ST25R3911B_LIB_LOG_LEVEL);

static struct device *spi_dev;

#define ST25R3911B_READ_REG(_reg) (0x40 | (_reg))
#define ST25R3911B_WRITE_REG(_reg) (~0xC0 & (_reg))
#define ST25R3911B_DIRECT_CMD(_cmd) (0xC0 | (_cmd))
#define ST25R3911B_FIFO_READ 0xBF
#define ST25R3911B_FIFO_WRITE 0x80

#define REG_CNT 0x3F

#define _DO_SPI_PORT_NUM(_num) \
	 DT_SPI_ ## _num ## _NAME

#define SPI_PORT_NUM(_num) \
	_DO_SPI_PORT_NUM(_num)

#define _DO_CS_PORT_NUM(_spi_port) \
	DT_ALIAS_SPI_ ## _spi_port ## _CS_GPIOS_CONTROLLER

#define CS_PORT_NUM(_spi_port) \
	_DO_CS_PORT_NUM(_spi_port)

#define _DO_CS_PIN_NUM(_spi_port) \
	DT_ALIAS_SPI_ ## _spi_port ## _CS_GPIOS_PIN

#define CS_PIN_NUM(_spi_port) \
	_DO_CS_PIN_NUM(_spi_port)

/* Timing defined by spec. */
#define T_NCS_SCLK 1

/* SPI CS pin configuration */
static struct spi_cs_control spi_cs = {
	.gpio_pin = CS_PIN_NUM(CONFIG_ST25R3911B_SPI_PORT),
	.delay = T_NCS_SCLK
};

/* SPI hardware configuration. */
static const struct spi_config spi_cfg =  {
	.frequency = CONFIG_ST25R3911B_SPI_FREQ,
	.operation = (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |
		      SPI_TRANSFER_MSB | SPI_LINES_SINGLE |
		      SPI_MODE_CPHA),
	.slave = 0,
	.cs = &spi_cs
};

static bool reg_is_valid(u8_t reg)
{
	return (reg < ST25R3911B_REG_IC_IDENTITY);
}

static bool cmd_is_valid(u8_t cmd)
{
	return ((!((cmd >= ST25R3911B_CMD_SET_DEFAULT) && (cmd <= ST25R3911B_CMD_ANALOG_PRESET))) ||
		(!((cmd >= ST25R3911B_CMD_MASK_RECEIVE_DATA) && (cmd <= ST25R3911B_CMD_TEST_ACCESS))));
}

static int cs_ctrl_gpio_config(void)
{
	spi_cs.gpio_dev = device_get_binding(CS_PORT_NUM(CONFIG_ST25R3911B_SPI_PORT));
	if (!spi_cs.gpio_dev) {
		LOG_ERR("Cannot find %s!",
			CS_PORT_NUM(CONFIG_ST25R3911B_SPI_PORT));

		return -ENXIO;
	}

	return 0;
}

int st25r3911b_spi_init(void)
{
	int err = cs_ctrl_gpio_config();

	if (err) {
		return err;
	}
	spi_dev = device_get_binding(SPI_PORT_NUM(CONFIG_ST25R3911B_SPI_PORT));
	if (!spi_dev) {
		LOG_ERR("SPI binding error.");
		return -ENXIO;
	}

	return 0;
}

int st25r3911b_multiple_reg_write(u8_t start_reg, u8_t *val,
				  size_t len)
{
	int err;
	u8_t cmd;

	if ((!val) || (!reg_is_valid(start_reg)) ||
	    (!reg_is_valid(start_reg + (len - 1)))) {
		return -EINVAL;
	}

	/* Write register address. */
	cmd = ST25R3911B_WRITE_REG(start_reg);

	const struct spi_buf tx_bufs[] = {
		{.buf = &cmd, .len = 1},
		{.buf = val, .len = len}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};

	err = spi_transceive(spi_dev, &spi_cfg, &tx, NULL);
	if (err) {
		LOG_ERR("SPI reg write failed, err: %d.", err);
		return err;
	}

	return 0;
}

int st25r3911b_multiple_reg_read(u8_t start_reg, u8_t *val, size_t len)
{
	int err;

	if ((!val) || (!reg_is_valid(start_reg)) ||
	    (!reg_is_valid(start_reg + (len - 1)))) {
		return -EINVAL;
	}

	/* Write register address. */
	start_reg = ST25R3911B_READ_REG(start_reg);

	const struct spi_buf tx_buf = {
		.buf = &start_reg,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	/* Read register value. */
	const struct spi_buf rx_bufs[] = {
		{.buf = NULL, .len = 1},
		{.buf = val, .len = len}
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs)
	};

	err = spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
	if (err) {
		LOG_ERR("SPI reg read failed, err: %d.", err);
		return err;
	}

	return 0;
}

int st25r3911b_cmd_execute(u8_t cmd)
{
	int err;

	if (!cmd_is_valid(cmd)) {
		return -EINVAL;
	}

	const struct spi_buf tx_buf = {
		.buf = &cmd,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	err = spi_transceive(spi_dev, &spi_cfg, &tx, NULL);
	if (err) {
		LOG_ERR("SPI direct command failed, err: %d.", err);
		return err;
	}

	return 0;
}

int st25r3911b_fifo_write(u8_t *data, size_t length)
{
	int err;

	if ((!data) || (length > ST25R3911B_MAX_FIFO_LEN)) {
		return -EINVAL;
	}

	u8_t cmd = ST25R3911B_FIFO_WRITE;
	const struct spi_buf tx_bufs[] = {
		{.buf = &cmd, .len = 1},
		{.buf = data, .len = length}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};

	err = spi_transceive(spi_dev, &spi_cfg, &tx, NULL);
	if (err) {
		LOG_ERR("SPI FIFO write failed, err: %d.", err);
		return err;
	}

	return 0;
}

int st25r3911b_fifo_read(u8_t *data, size_t length)
{
	int err;

	if ((!data) || (length > ST25R3911B_MAX_FIFO_LEN)) {
		return -EINVAL;
	}

	u8_t cmd = ST25R3911B_FIFO_READ;
	const struct spi_buf tx_buf = {
		.buf = &cmd,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	/* Read FIFO. */
	const struct spi_buf rx_bufs[] = {
		{.buf = NULL, .len = 1},
		{.buf = data, .len = length}
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs)
	};

	err = spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
	if (err) {
		LOG_ERR("SPI FIFO read failed, err: %d.", err);
		return err;
	}

	return 0;
}

int st25r3911b_reg_modify(u8_t reg, u8_t clr_mask, u8_t set_mask)
{
	int err;
	u8_t tmp;

	if (!reg_is_valid(reg)) {
		return -EINVAL;
	}

	err = st25r3911b_reg_read(reg, &tmp);
	if (err) {
		goto error;
	}

	tmp &= ~clr_mask;
	tmp |= set_mask;

	err = st25r3911b_reg_write(reg, tmp);
	if (err) {
		goto error;
	}

	return 0;

error:
	LOG_ERR("SPI set register bit failed, err: %d.", err);

	return err;
}

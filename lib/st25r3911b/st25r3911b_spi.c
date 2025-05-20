/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "st25r3911b_spi.h"
#include "st25r3911b_reg.h"
#include "st25r3911b_dt.h"

LOG_MODULE_REGISTER(st25r3911b, CONFIG_ST25R3911B_LIB_LOG_LEVEL);

static const struct device *spi_dev = DEVICE_DT_GET(DT_BUS(ST25R3911B_NODE));

#define ST25R3911B_READ_REG(_reg) (0x40 | (_reg))
#define ST25R3911B_WRITE_REG(_reg) (~0xC0 & (_reg))
#define ST25R3911B_DIRECT_CMD(_cmd) (0xC0 | (_cmd))
#define ST25R3911B_FIFO_READ 0xBF
#define ST25R3911B_FIFO_WRITE 0x80

#define REG_CNT 0x3F

/* Timing defined by spec. */
#define T_NCS_SCLK 1

/* SPI hardware configuration. */
static const struct spi_config spi_cfg =  {
	.frequency = DT_PROP(ST25R3911B_NODE, spi_max_frequency),
	.operation = (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |
		      SPI_TRANSFER_MSB | SPI_LINES_SINGLE |
		      SPI_MODE_CPHA),
	.slave = DT_REG_ADDR(ST25R3911B_NODE),
	.cs = {
		.gpio = SPI_CS_GPIOS_DT_SPEC_GET(ST25R3911B_NODE),
		.delay = T_NCS_SCLK
	}
};

static bool reg_is_valid(uint8_t reg)
{
	return (reg < ST25R3911B_REG_IC_IDENTITY);
}

static bool cmd_is_valid(uint8_t cmd)
{
	return ((!((cmd >= ST25R3911B_CMD_SET_DEFAULT) && (cmd <= ST25R3911B_CMD_ANALOG_PRESET))) ||
		(!((cmd >= ST25R3911B_CMD_MASK_RECEIVE_DATA) && (cmd <= ST25R3911B_CMD_TEST_ACCESS))));
}

int st25r3911b_spi_init(void)
{
	LOG_DBG("Initializing. SPI device: %s, CS GPIO: %s pin %d",
		spi_dev->name, spi_cfg.cs.gpio.port->name, spi_cfg.cs.gpio.pin);

	if (!device_is_ready(spi_cfg.cs.gpio.port)) {
		LOG_ERR("GPIO device %s is not ready!", spi_cfg.cs.gpio.port->name);

		return -ENXIO;
	}

	if (!device_is_ready(spi_dev)) {
		LOG_ERR("SPI device %s is not ready!", spi_dev->name);
		return -ENXIO;
	}

	return 0;
}

int st25r3911b_multiple_reg_write(uint8_t start_reg, uint8_t *val,
				  size_t len)
{
	int err;
	uint8_t cmd;

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

int st25r3911b_multiple_reg_read(uint8_t start_reg, uint8_t *val, size_t len)
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

int st25r3911b_cmd_execute(uint8_t cmd)
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

int st25r3911b_fifo_write(uint8_t *data, size_t length)
{
	int err;

	if ((!data) || (length > ST25R3911B_MAX_FIFO_LEN)) {
		return -EINVAL;
	}

	uint8_t cmd = ST25R3911B_FIFO_WRITE;
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

int st25r3911b_fifo_read(uint8_t *data, size_t length)
{
	int err;

	if ((!data) || (length > ST25R3911B_MAX_FIFO_LEN)) {
		return -EINVAL;
	}

	uint8_t cmd = ST25R3911B_FIFO_READ;
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

int st25r3911b_reg_modify(uint8_t reg, uint8_t clr_mask, uint8_t set_mask)
{
	int err;
	uint8_t tmp;

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

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * I2C emulator modeling enough of the TAC5112 register file (page-select
 * register 0, page 0/1/3 reset defaults, and the self-clearing latched
 * fault registers) for the driver's ztest suite in
 * tests/drivers/audio/tac5112 to exercise real I2C traffic through the
 * real driver code without hardware. It stands in for the physical
 * TAC5112 on the far end of an nRF5 TWI/I2C peripheral.
 *
 * This is test-only scaffolding: it is built solely under
 * CONFIG_AUDIO_TAC5112_EMUL, never as part of a production image.
 */

#define DT_DRV_COMPAT ti_tac5112

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>

#include "tac5112.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(tac5112_emul, CONFIG_I2C_LOG_LEVEL);

#define TAC5112_EMUL_NUM_PAGES (TAC5112_PAGE_MAX + 1U)
#define TAC5112_EMUL_PAGE_SIZE 256U

struct tac5112_emul_data {
	struct i2c_emul emul;
	const struct device *i2c;
	uint8_t regs[TAC5112_EMUL_NUM_PAGES][TAC5112_EMUL_PAGE_SIZE];
	uint8_t cur_page;
	uint8_t cur_addr;
};

struct tac5112_emul_cfg {
	uint16_t addr;
};

static void tac5112_emul_reset_defaults(struct tac5112_emul_data *data)
{
	memset(data->regs, 0, sizeof(data->regs));

	/* Page 0 reset defaults (Section 8.1.1, Table 8-2). */
	data->regs[0][TAC5112_REG_PASI_CFG0.addr] = 0x30U;
	data->regs[0][TAC5112_REG_CLK_CFG2.addr] = 0x40U;
	data->regs[0][TAC5112_REG_ADC_CH1_CFG2.addr] = 0xA1U;
	data->regs[0][TAC5112_REG_ADC_CH1_CFG3.addr] = 0x80U;
	data->regs[0][TAC5112_REG_ADC_CH2_CFG2.addr] = 0xA1U;
	data->regs[0][TAC5112_REG_ADC_CH2_CFG3.addr] = 0x80U;
	data->regs[0][TAC5112_REG_DAC_CH1A_CFG0.addr] = 0xC9U;
	data->regs[0][TAC5112_REG_DAC_CH1B_CFG0.addr] = 0xC9U;
	data->regs[0][TAC5112_REG_DAC_CH2A_CFG0.addr] = 0xC9U;
	data->regs[0][TAC5112_REG_DAC_CH2B_CFG0.addr] = 0xC9U;
	data->regs[0][TAC5112_REG_CH_EN.addr] = 0xCCU;
	data->regs[0][TAC5112_REG_DEV_STS1.addr] = 0x80U;

	/* Page 1 reset defaults. */
	data->regs[1][TAC5112_REG_INT_MASK0.addr] = 0xFFU;

	/* Page 3 reset defaults (PLL/clock dividers). */
	data->regs[3][TAC5112_REG_CLK_CFG15.addr] = 0x01U;
	data->regs[3][TAC5112_REG_CLK_CFG18.addr] = 0x08U;
	data->regs[3][TAC5112_REG_CLK_CFG19.addr] = 0x20U;
	data->regs[3][TAC5112_REG_CLK_CFG20.addr] = 0x04U;

	data->cur_page = 0U;
	data->cur_addr = 0U;
}

static uint8_t tac5112_emul_reg_get(struct tac5112_emul_data *data, uint8_t page, uint8_t addr)
{
	uint8_t val;

	if (page >= TAC5112_EMUL_NUM_PAGES) {
		LOG_WRN("Read from defined page %u treated as 0", page);
		return 0U;
	}

	val = data->regs[page][addr];

	if (page == 1U) {
		if ((addr == TAC5112_REG_INT_LTCH0.addr) ||
		    (addr == TAC5112_REG_OUT_CH1_LTCH.addr) ||
		    (addr == TAC5112_REG_OUT_CH2_LTCH.addr)) {
			data->regs[page][addr] = 0U;
		}
	}

	return val;
}

static void tac5112_emul_reg_set(struct tac5112_emul_data *data, uint8_t val)
{
	if (data->cur_addr == TAC5112_REG_PAGE_CFG.addr) {
		data->cur_page = val;
		return;
	}

	if (data->cur_page >= TAC5112_EMUL_NUM_PAGES) {
		LOG_WRN("Write to unmodeled page %u reg 0x%02x dropped", data->cur_page,
			data->cur_addr);
		return;
	}

	data->regs[data->cur_page][data->cur_addr] = val;

	if ((data->cur_page == TAC5112_REG_SW_RESET.page) &&
	    (data->cur_addr == TAC5112_REG_SW_RESET.addr) && ((val & TAC5112_SW_RESET_BIT) != 0U)) {
		/* Self-clearing software reset: restore POR defaults. */
		tac5112_emul_reset_defaults(data);
	}
}

static int tac5112_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				 int addr)
{
	struct tac5112_emul_data *data;
	const struct tac5112_emul_cfg *cfg;
	uint32_t i;

	if ((target == NULL) || (msgs == NULL)) {
		return -EINVAL;
	}

	data = target->data;
	cfg = target->cfg;

	if (cfg->addr != (uint16_t)addr) {
		LOG_ERR("Address mismatch, expected 0x%02x, got 0x%02x", cfg->addr, addr);
		return -EIO;
	}

	switch (num_msgs) {
	case 1:
		if (msgs->flags & I2C_MSG_READ) {
			for (i = 0; i < msgs->len; i++) {
				msgs->buf[i] =
					tac5112_emul_reg_get(data, data->cur_page, data->cur_addr);
				data->cur_addr++;
			}

			return 0;
		}

		if (msgs->len == 0) {
			return 0; /* zero-length write used for probing */
		}

		data->cur_addr = msgs->buf[0];
		for (i = 1; i < msgs->len; i++) {
			tac5112_emul_reg_set(data, msgs->buf[i]);
			data->cur_addr++;
		}

		return 0;
	case 2:
		if ((msgs->flags & I2C_MSG_READ) || (msgs->len != 1)) {
			LOG_ERR("Unexpected first message in 2-message transfer");
			return -EIO;
		}
		data->cur_addr = msgs->buf[0];

		msgs++;
		if (!(msgs->flags & I2C_MSG_READ)) {
			LOG_ERR("Unexpected second message in 2-message transfer");
			return -EIO;
		}

		for (i = 0; i < msgs->len; i++) {
			msgs->buf[i] = tac5112_emul_reg_get(data, data->cur_page, data->cur_addr);
			data->cur_addr++;
		}

		return 0;
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}
}

static struct i2c_emul_api tac5112_emul_bus_api = {
	.transfer = tac5112_emul_transfer,
};

static int tac5112_emul_init(const struct emul *target, const struct device *parent)
{
	struct tac5112_emul_data *data = target->data;
	const struct tac5112_emul_cfg *cfg = target->cfg;

	data->emul.api = &tac5112_emul_bus_api;
	data->emul.addr = cfg->addr;
	data->emul.target = target;
	data->i2c = parent;

	tac5112_emul_reset_defaults(data);

	return 0;
}

#define TAC5112_EMUL_DEFINE(n)                                                                     \
	static struct tac5112_emul_data tac5112_emul_data_##n;                                     \
	static const struct tac5112_emul_cfg tac5112_emul_cfg_##n = {                              \
		.addr = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, tac5112_emul_init, &tac5112_emul_data_##n, &tac5112_emul_cfg_##n,   \
			    &tac5112_emul_bus_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(TAC5112_EMUL_DEFINE)

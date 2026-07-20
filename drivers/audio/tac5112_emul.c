/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * I2C emulator modeling enough of the TAC5112 register file (page-select
 * register 0, page 0/1/3 reset defaults, and the self-clearing latched
 * fault registers) for the driver's ztest suite in
 * tests/drivers/audio/tac5112 to exercise real I2C traffic through the
 * real driver code without hardware. It stands in for the physical
 * TAC5112 on the far end of an nRF5 TWI/I2C peripheral.
 *
 * This is test-only scaffolding: it is built solely under
 * CONFIG_AUDIO_CODEC_TAC5112_EMUL, never as part of a production image.
 */

#define DT_DRV_COMPAT ti_tac5112

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#ifdef CONFIG_GPIO_EMUL
#include <zephyr/drivers/gpio/gpio_emul.h>
#endif
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <drivers/tac5112.h>

#include "tac5112_private.h"
#include "tac5112_emul.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tac5112_emul, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define TAC5112_EMUL_NUM_PAGES (TAC5112_PAGE_MAX + 1U)
#define TAC5112_EMUL_PAGE_SIZE 256U

#define TAC5112_EMUL_NUM_FAIL_SLOTS 2U

struct tac5112_emul_data {
	struct i2c_emul emul;
	const struct device *i2c;
	uint8_t regs[TAC5112_EMUL_NUM_PAGES][TAC5112_EMUL_PAGE_SIZE];
	uint8_t cur_page;
	uint8_t cur_addr;
	const struct gpio_dt_spec *irq;
	bool irq_armed;
	struct {
		bool en;
		struct tac5112_reg reg;
	} fail_write[TAC5112_EMUL_NUM_FAIL_SLOTS];

	uint8_t fail_read_count;
	struct tac5112_reg fail_read_reg;
};

struct tac5112_emul_cfg {
	uint16_t addr;
	struct gpio_dt_spec irq;
};

/**
 * @brief True if @p reg is one of the latched fault registers.
 *
 * The latched registers (INT_LTCH0, OUT_CH1_LTCH and OUT_CH2_LTCH, all on page 1)
 * behave differently from ordinary registers in the emulator:
 * - they are read-only, so I2C writes to them are ignored (a latch is only set
 *   by tac5112_emul_inject_latch()),
 * - reading one returns its value and clears it, as on the codec, and
 * - changing one updates the modelled IRQ line.
 *
 * @param[in] reg  Register (page and address) to test.
 *
 * @retval true if @p reg is a latched fault register.
 * @retval false for any other register.
 */
static bool tac5112_emul_is_latch(struct tac5112_reg reg)
{
	return (reg.page == 1U) && ((reg.addr == TAC5112_REG_INT_LTCH0.addr) ||
				    (reg.addr == TAC5112_REG_OUT_CH1_LTCH.addr) ||
				    (reg.addr == TAC5112_REG_OUT_CH2_LTCH.addr));
}

/*
 * Drive the IRQ line from the modelled state: asserted while an unmasked
 * latched event is pending. The output-channel
 * short-circuit and virtual-ground latches are gated by INT_MASK4 (a set
 * mask bit suppresses the interrupt but not the latch itself).
 */
static void tac5112_emul_update_irq(struct tac5112_emul_data *data)
{
#ifdef CONFIG_GPIO_EMUL
	uint8_t mask4 = data->regs[1][TAC5112_REG_INT_MASK4.addr];
	uint8_t pending = 0U;
	int val;

	/* The GPIO emulator may initialise after this emulator, so check lazily,
	 * and leave the pin alone until the driver has made it an input.
	 */
	if (!data->irq_armed || (data->irq->port == NULL) || !device_is_ready(data->irq->port)) {
		return;
	}

	for (uint8_t i = 0U; i <= 1U; i++) {
		uint8_t latch = data->regs[1][(i == 0U) ? TAC5112_REG_OUT_CH1_LTCH.addr
							: TAC5112_REG_OUT_CH2_LTCH.addr];

		if ((mask4 & TAC5112_INT_MASK4_OUT_SC_BIT) == 0U) {
			pending |=
				latch & (TAC5112_OUT_CH_SC_OUTP_BIT | TAC5112_OUT_CH_SC_OUTM_BIT);
		}

		if ((mask4 & TAC5112_INT_MASK4_DRVR_VG_BIT) == 0U) {
			pending |= latch &
				   (TAC5112_OUT_CH_VG_FAULT_P_BIT | TAC5112_OUT_CH_VG_FAULT_M_BIT);
		}
	}

	if (pending != 0U) {
		if (data->irq->dt_flags & GPIO_ACTIVE_LOW) {
			val = 0;
		} else {
			val = 1;
		}
	} else {
		if (data->irq->dt_flags & GPIO_ACTIVE_LOW) {
			val = 1;
		} else {
			val = 0;
		}
	}

	/* gpio_emul_input_set() takes the raw level; honour the line polarity. */
	(void)gpio_emul_input_set(data->irq->port, data->irq->pin, val);
#else
	ARG_UNUSED(data);
#endif
}

static void tac5112_emul_reset_defaults(struct tac5112_emul_data *data)
{
	memset(data->regs, 0, sizeof(data->regs));

	/* Page 0 reset defaults. */
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
	/* PASI slot assignment: channel 1 left slot 0, channel 2 left slot 1. */
	data->regs[0][TAC5112_REG_PASI_TX_CH1_CFG.addr] = 0x20U;
	data->regs[0][TAC5112_REG_PASI_TX_CH2_CFG.addr] = 0x21U;
	data->regs[0][TAC5112_REG_PASI_RX_CH1_CFG.addr] = 0x20U;
	data->regs[0][TAC5112_REG_PASI_RX_CH2_CFG.addr] = 0x21U;
	data->regs[0][TAC5112_REG_DEV_STS1.addr] = 0x80U;

	/* Page 1 reset defaults, */
	data->regs[1][TAC5112_REG_INT_MASK0.addr] = 0xFFU;

	/* Page 3 reset defaults. */
	data->regs[3][TAC5112_REG_CLK_CFG15.addr] = 0x01U;
	data->regs[3][TAC5112_REG_CLK_CFG18.addr] = 0x08U;
	data->regs[3][TAC5112_REG_CLK_CFG19.addr] = 0x20U;
	data->regs[3][TAC5112_REG_CLK_CFG20.addr] = 0x04U;

	data->cur_page = 0U;
	data->cur_addr = 0U;

	tac5112_emul_update_irq(data);
}

static int tac5112_emul_reg_get(struct tac5112_emul_data *data, uint8_t page, uint8_t addr,
				uint8_t *val)
{
	if (page >= TAC5112_EMUL_NUM_PAGES) {
		LOG_ERR("Read from unmodeled page %u reg 0x%02x", page, addr);
		return -EIO;
	}

	*val = data->regs[page][addr];

	if (tac5112_emul_is_latch(TAC5112_REG(page, addr))) {
		/* Latched fault registers clear on read. */
		data->regs[page][addr] = 0U;
		tac5112_emul_update_irq(data);
	}

	return 0;
}

static int tac5112_emul_reg_set(struct tac5112_emul_data *data, uint8_t val)
{
	if (data->cur_addr == TAC5112_REG_PAGE_CFG.addr) {
		data->cur_page = val;
		return 0;
	}

	for (int i = 0; i < TAC5112_EMUL_NUM_FAIL_SLOTS; i++) {
		if (!data->fail_write[i].en || (data->fail_write[i].reg.page != data->cur_page) ||
		    (data->fail_write[i].reg.addr != data->cur_addr)) {
			continue;
		}

		return -EIO;
	}

	if (data->cur_page >= TAC5112_EMUL_NUM_PAGES) {
		LOG_ERR("Write to unmodeled page %u reg 0x%02x", data->cur_page, data->cur_addr);
		return -EIO;
	}

	if ((data->cur_page == TAC5112_REG_SW_RESET.page) &&
	    (data->cur_addr == TAC5112_REG_SW_RESET.addr) && ((val & TAC5112_SW_RESET_BIT) != 0U)) {
		/* Self-clearing software reset: restore POR defaults. Nothing else
		 * about this write matters once the device is reset.
		 */
		tac5112_emul_reset_defaults(data);
		return 0;
	}

	if (tac5112_emul_is_latch(TAC5112_REG(data->cur_page, data->cur_addr))) {
		/* Latched status is read-only; writes are ignored. */
		return 0;
	}

	data->regs[data->cur_page][data->cur_addr] = val;

	if ((data->cur_page == TAC5112_REG_INT_MASK4.page) &&
	    (data->cur_addr == TAC5112_REG_INT_MASK4.addr)) {
		tac5112_emul_update_irq(data);
	}

	return 0;
}

static bool tac5112_emul_read_fails(struct tac5112_emul_data *data)
{
	if ((data->fail_read_count != 0U) && (data->fail_read_reg.page == data->cur_page) &&
	    (data->fail_read_reg.addr == data->cur_addr)) {
		data->fail_read_count--;
		return true;
	}

	return false;
}

/**
 * @brief Reject a transfer that would run past the last register of a page.
 *
 * The codec increments the register address after every byte, and the address is
 * a byte, so a burst that runs off the end of the page would wrap to register 0
 * (PAGE_CFG) and be taken for a page select. The driver limits its bursts to the
 * page, so a longer one is a driver bug.
 *
 * @param[in] start  Address of the first register in the transfer.
 * @param[in] count  Number of registers in the transfer.
 *
 * @retval 0 if the transfer stays within the page.
 * @retval -EINVAL if it runs past the end of the page.
 */
static int tac5112_emul_check_burst(uint8_t start, size_t count)
{
	if (((size_t)start + count) > TAC5112_REG_ADDR_COUNT) {
		LOG_ERR("Transfer of %u registers from 0x%02x runs past the end of the page",
			(unsigned int)count, start);
		return -EINVAL;
	}

	return 0;
}

static int tac5112_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				 int addr)
{
	int ret;
	struct tac5112_emul_data *data;
	const struct tac5112_emul_cfg *cfg;

	if ((target == NULL) || (msgs == NULL)) {
		return -EINVAL;
	}

	data = target->data;
	cfg = target->cfg;

	if (cfg->addr != (uint16_t)addr) {
		LOG_ERR("Address mismatch, expected 0x%02x, got 0x%02x", cfg->addr, addr);
		return -EIO;
	}

	if (!data->irq_armed) {
		/* First access from the driver: its IRQ pin is configured by now,
		 * so drive the idle level.
		 */
		data->irq_armed = true;
		tac5112_emul_update_irq(data);
	}

	switch (num_msgs) {
	case 1:
		if (msgs->len == 0) {
			LOG_ERR("Zero-length transfer");
			return -EINVAL;
		}

		if (msgs->flags & I2C_MSG_READ) {
			ret = tac5112_emul_check_burst(data->cur_addr, msgs->len);
			if (ret != 0) {
				return ret;
			}

			for (int i = 0; i < msgs->len; i++) {
				if (tac5112_emul_read_fails(data)) {
					return -EIO;
				}

				ret = tac5112_emul_reg_get(data, data->cur_page, data->cur_addr,
							   &msgs->buf[i]);
				if (ret != 0) {
					return ret;
				}

				data->cur_addr++;
			}

			return 0;
		}

		ret = tac5112_emul_check_burst(msgs->buf[0], msgs->len - 1U);
		if (ret != 0) {
			return ret;
		}

		data->cur_addr = msgs->buf[0];
		for (int i = 1; i < msgs->len; i++) {
			ret = tac5112_emul_reg_set(data, msgs->buf[i]);
			if (ret != 0) {
				return ret;
			}

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
		if (msgs->len == 0) {
			LOG_ERR("Zero-length data message in 2-message transfer");
			return -EINVAL;
		}

		ret = tac5112_emul_check_burst(data->cur_addr, msgs->len);
		if (ret != 0) {
			return ret;
		}

		if (!(msgs->flags & I2C_MSG_READ)) {
			/* Burst write: the address message is followed by the data. */
			for (int i = 0; i < msgs->len; i++) {
				ret = tac5112_emul_reg_set(data, msgs->buf[i]);
				if (ret != 0) {
					return ret;
				}

				data->cur_addr++;
			}

			return 0;
		}

		for (int i = 0; i < msgs->len; i++) {
			if (tac5112_emul_read_fails(data)) {
				return -EIO;
			}

			ret = tac5112_emul_reg_get(data, data->cur_page, data->cur_addr,
						   &msgs->buf[i]);
			if (ret != 0) {
				return ret;
			}

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

static int tac5112_emul_init(const struct emul *target, const struct device *i2c_parent)
{
	struct tac5112_emul_data *data = target->data;
	const struct tac5112_emul_cfg *cfg = target->cfg;

	data->emul.api = &tac5112_emul_bus_api;
	data->emul.addr = cfg->addr;
	data->emul.target = target;
	data->i2c = i2c_parent;
	data->irq = &cfg->irq;

	tac5112_emul_reset_defaults(data);

	return 0;
}

int tac5112_emul_inject_latch(const struct emul *target, struct tac5112_reg reg, uint8_t bits)
{
	struct tac5112_emul_data *data = target->data;

	if (!tac5112_emul_is_latch(reg)) {
		return -EINVAL;
	}

	data->regs[reg.page][reg.addr] |= bits;
	tac5112_emul_update_irq(data);

	return 0;
}

static void tac5112_emul_set_fail_slot(struct tac5112_emul_data *data, struct tac5112_reg reg,
				       bool en)
{
	size_t free_slot = TAC5112_EMUL_NUM_FAIL_SLOTS;

	for (int i = 0; i < TAC5112_EMUL_NUM_FAIL_SLOTS; i++) {
		if (data->fail_write[i].en && (data->fail_write[i].reg.page == reg.page) &&
		    (data->fail_write[i].reg.addr == reg.addr)) {
			data->fail_write[i].en = en;
			return;
		}

		if (!data->fail_write[i].en && (free_slot == TAC5112_EMUL_NUM_FAIL_SLOTS)) {
			free_slot = i;
		}
	}

	if (!en) {
		return;
	}

	if (free_slot == TAC5112_EMUL_NUM_FAIL_SLOTS) {
		LOG_ERR("No free write-failure slot for pg:%u reg:0x%02x (max %u)", reg.page,
			reg.addr, TAC5112_EMUL_NUM_FAIL_SLOTS);
		return;
	}

	data->fail_write[free_slot].en = true;
	data->fail_write[free_slot].reg = reg;
}

void tac5112_emul_fail_writes(const struct emul *target, struct tac5112_reg reg, bool fail)
{
	tac5112_emul_set_fail_slot(target->data, reg, fail);
}

void tac5112_emul_clear_failures(const struct emul *target)
{
	struct tac5112_emul_data *data = target->data;

	memset(data->fail_write, 0, sizeof(data->fail_write));
	data->fail_read_count = 0U;
}

void tac5112_emul_fail_reads(const struct emul *target, struct tac5112_reg reg, uint8_t count)
{
	struct tac5112_emul_data *data = target->data;

	data->fail_read_reg = reg;
	data->fail_read_count = count;
}

uint8_t tac5112_emul_latch_peek(const struct emul *target, struct tac5112_reg reg)
{
	struct tac5112_emul_data *data = target->data;

	if (tac5112_emul_is_latch(reg)) {
		return data->regs[reg.page][reg.addr];
	}

	return 0U;
}

#define TAC5112_EMUL_DEFINE(n)                                                                     \
	static struct tac5112_emul_data tac5112_emul_data_##n;                                     \
	static const struct tac5112_emul_cfg tac5112_emul_cfg_##n = {                              \
		.addr = DT_INST_REG_ADDR(n),                                                       \
		.irq = GPIO_DT_SPEC_INST_GET_OR(n, interrupt_gpios, {0}),                          \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, tac5112_emul_init, &tac5112_emul_data_##n, &tac5112_emul_cfg_##n,   \
			    &tac5112_emul_bus_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(TAC5112_EMUL_DEFINE)

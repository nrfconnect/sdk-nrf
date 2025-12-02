/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/drivers/i2c.h>
#include <nrfx_twis.h>

#define I2C_S_INST_IDX 0

#define DT_DRV_COMPAT nordic_nrf_twis
#if !DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(I2C_S_INST_IDX))
#error "TWIS instance not enabled or not supported"
#endif

#define NODE_SENSOR DT_NODELABEL(sensor)
#define NODE_TWIS   DT_ALIAS(i2c_slave)

/* Test configuration: */
#define DATA_FIELD_LEN	(16)
#define	DELTA			(1)

#define TWIS_MEMORY_SECTION                                                            \
	COND_CODE_1(DT_NODE_HAS_PROP(NODE_TWIS, memory_regions),                           \
		    (__attribute__((__section__(                                               \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(NODE_TWIS, memory_regions)))))), \
		    ())

static bool test_pass;
static int counter;

#if CONFIG_ROLE_HOST == 1
static const struct device *twim_dev = DEVICE_DT_GET(DT_BUS(NODE_SENSOR));
static uint8_t i2c_twim_buffer[DATA_FIELD_LEN];
static uint8_t previous_data[DATA_FIELD_LEN];

#else /* ROLE */
static nrfx_twis_t twis = {
	.p_reg = (NRF_TWIS_Type *)DT_INST_REG_ADDR(I2C_S_INST_IDX)
};
static uint8_t i2c_twis_buffer[DATA_FIELD_LEN] TWIS_MEMORY_SECTION;
static uint8_t previous;

static const nrfx_twis_config_t config = {
	.addr = {DT_REG_ADDR(NODE_SENSOR), 0},
	.skip_gpio_cfg = true,
	.skip_psel_cfg = true,
};

void twis_verify_data(void)
{
	test_pass = true;
	/* Verify that received bytes differ by DELTA */
	/* First byte shall be (last byte from previous transaction + DELTA) */
	uint8_t byte_0 = i2c_twis_buffer[0];
	uint8_t expected = previous + DELTA;

	if (byte_0 != expected) {
		LOG_ERR("FAIL: rx[0] = %d, expected %d", byte_0, expected);
		test_pass = false;
	}
	/* Next byte shall be (previous byte + DELTA) */
	for (int i = 1; i < DATA_FIELD_LEN; i++) {
		uint8_t byte_i = i2c_twis_buffer[i];
		uint8_t byte_i_less_1 = i2c_twis_buffer[i - 1];

		if (byte_i != byte_i_less_1 + DELTA) {
			LOG_ERR("FAIL: rx[%d] = %d, expected %d",
				i, byte_i, (byte_i_less_1 + DELTA));
			test_pass = false;
		}
	}

	/* Backup last byte for next transaction */
	previous = i2c_twis_buffer[DATA_FIELD_LEN - 1];

	if (test_pass) {
		LOG_INF("Run %d - PASS", counter);
	} else {
		LOG_INF("Run %d - FAILED", counter);
	}
	counter++;
}

void i2s_slave_handler(nrfx_twis_event_t const *p_event)
{
	switch (p_event->type) {
	case NRFX_TWIS_EVT_READ_REQ:
		nrfx_twis_tx_prepare(&twis, i2c_twis_buffer, DATA_FIELD_LEN);
		LOG_DBG("TWIS event: read request");
		break;
	case NRFX_TWIS_EVT_READ_DONE:
		memset(i2c_twis_buffer, 0, DATA_FIELD_LEN);
		LOG_DBG("TWIS event: read done");
		break;
	case NRFX_TWIS_EVT_WRITE_REQ:
		nrfx_twis_rx_prepare(&twis, i2c_twis_buffer, DATA_FIELD_LEN);
		LOG_DBG("TWIS event: write request");
		break;
	case NRFX_TWIS_EVT_WRITE_DONE:
		twis_verify_data();
		LOG_DBG("TWIS event: write done");
		break;
	default:
		LOG_DBG("TWIS event: %d", p_event->type);
		break;
	}
}

void twis_setup(void)
{
	int ret;

	ret = nrfx_twis_init(&twis, &config, i2s_slave_handler);
	if (ret != 0) {
		LOG_ERR("nrfx_twis_init returned %d", ret);
	}

	PINCTRL_DT_DEFINE(NODE_TWIS);
	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(NODE_TWIS), PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("pinctrl_apply_state() returned %d", ret);
	}

	IRQ_CONNECT(DT_INST_IRQN(I2C_S_INST_IDX), DT_INST_IRQ(I2C_S_INST_IDX, priority),
		    nrfx_twis_irq_handler, &twis, 0);

	nrfx_twis_enable(&twis);
}
#endif /* ROLE */

int main(void)
{

#if CONFIG_ROLE_HOST == 1
	int ret;
	/* Variable used to generate increasing values */
	uint8_t acc = 0;

	LOG_INF("%s runs as a TWI HOST", CONFIG_BOARD_TARGET);

	/* Run test forever */
	while (1) {
		test_pass = true;

		/* Generate increasing values for the current test */
		for (int i = 0; i < DATA_FIELD_LEN; i++) {
			i2c_twim_buffer[i] = acc;
			acc += DELTA;
		}

		/* Send data */
		ret = i2c_write(twim_dev, i2c_twim_buffer, DATA_FIELD_LEN,
			DT_REG_ADDR(NODE_SENSOR));
		if (ret != 0) {
			LOG_ERR("i2c_write() returned %d", ret);
		}

		/* Backup data that was sent and clear twim buffer */
		for (int i = 0; i < DATA_FIELD_LEN; i++) {
			previous_data[i] = i2c_twim_buffer[i];
			i2c_twim_buffer[i] = 0;
		}

		/* Receive data */
		ret = i2c_read(twim_dev, i2c_twim_buffer, DATA_FIELD_LEN,
			DT_REG_ADDR(NODE_SENSOR));
		if (ret != 0) {
			LOG_ERR("i2c_read() returned %d", ret);
		}

		/* Check that received data is equal to data that was sent */
		for (int i = 0; i < DATA_FIELD_LEN; i++) {
			if (i2c_twim_buffer[i] != previous_data[i]) {
				LOG_ERR("FAIL: rx[%d] = %d, expected %d",
					i, i2c_twim_buffer[i], previous_data[i]);
				test_pass = false;
			}
		}

		if (test_pass) {
			LOG_INF("Run %d - PASS", counter);
		} else {
			LOG_INF("Run %d - FAILED", counter);
		}

		counter++;
		k_msleep(500);
	}

#else
	LOG_INF("%s runs as a TWI DEVICE", CONFIG_BOARD_TARGET);

	twis_setup();

	/* Run test forever */
	while (1) {
		k_msleep(500);
	}

#endif
}

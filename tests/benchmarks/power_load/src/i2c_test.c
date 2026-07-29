/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "i2c_test.h"

LOG_MODULE_REGISTER(i2c_test, LOG_LEVEL_INF);

extern atomic_t started_threads;

static nrfx_twis_t twis = {.p_reg = (NRF_TWIS_Type *)DT_REG_ADDR(NODE_TWIS)};

static uint8_t i2c_slave_buffer[MAX_TEST_DATA_SIZE] TWIS_MEMORY_SECTION;
static uint8_t i2c_master_buffer[MAX_TEST_DATA_SIZE];

struct i2c_api_twis_fixture {
	const struct device *dev;
	uint8_t addr;
	uint8_t *const master_buffer;
	uint8_t *const slave_buffer;
};

static struct i2c_api_twis_fixture fixture = {
	.dev = DEVICE_DT_GET(DT_BUS(NODE_TWIM)),
	.addr = DT_REG_ADDR(NODE_TWIM),
	.master_buffer = i2c_master_buffer,
	.slave_buffer = i2c_slave_buffer,
};

static void i2s_slave_handler(nrfx_twis_event_t const *p_event)
{
	switch (p_event->type) {
	case NRFX_TWIS_EVT_READ_REQ:
		nrfx_twis_tx_prepare(&twis, i2c_slave_buffer, MAX_TEST_DATA_SIZE);
		break;
	case NRFX_TWIS_EVT_READ_DONE:
		break;
	case NRFX_TWIS_EVT_WRITE_REQ:
		nrfx_twis_rx_prepare(&twis, i2c_slave_buffer, MAX_TEST_DATA_SIZE);
		break;
	case NRFX_TWIS_EVT_WRITE_DONE:
		break;
	default:
		break;
	}
}

static int setup(void)
{
	int ret;

	const nrfx_twis_config_t config = {
		.addr = {fixture.addr, 0},
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
	};

	ret = device_is_ready(fixture.dev);
	if (ret != 1) {
		LOG_ERR("TWIM device not ready: %d", ret);
		return -1;
	}

	ret = nrfx_twis_init(&twis, &config, i2s_slave_handler);
	if (ret != 0) {
		LOG_ERR("TWIS initialization failed: %d", ret);
		return -2;
	}

	PINCTRL_DT_DEFINE(NODE_TWIS);

	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(NODE_TWIS), PINCTRL_STATE_DEFAULT);

	if (ret != 0) {
		LOG_ERR("TWIS pinctrl - failed to apply state: %d", ret);
		return -3;
	}

	IRQ_CONNECT(DT_IRQN(NODE_TWIS), DT_IRQ(NODE_TWIS, priority), nrfx_twis_irq_handler, &twis,
		    0);

	nrfx_twis_enable(&twis);

	memset(fixture.slave_buffer, 0, MAX_TEST_DATA_SIZE);
	memset(fixture.master_buffer, 0, MAX_TEST_DATA_SIZE);

	return 0;
}

static void i2c_load_thread_worker(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;

	ret = setup();
	if (ret != 0) {
		LOG_ERR("I2C setup failed: %d\n", ret);
		return;
	}

	atomic_inc(&started_threads);

	LOG_INF("I2C load thread stared");
	while (1) {
		ret = i2c_write(fixture.dev, fixture.master_buffer, MAX_TEST_DATA_SIZE,
				fixture.addr);
		if (ret != 0) {
			LOG_ERR("I2C write failed: %d", ret);
		}
		ret = i2c_read(fixture.dev, fixture.master_buffer, MAX_TEST_DATA_SIZE,
			       fixture.addr);
		if (ret != 0) {
			LOG_ERR("I2C read failed: %d", ret);
		}
		k_msleep(I2C_THREAD_SLEEP);
	}
}

K_THREAD_DEFINE(i2c_load_thread, I2C_THREAD_STACKSIZE, i2c_load_thread_worker, NULL, NULL, NULL,
		K_PRIO_PREEMPT(I2C_THREAD_PRIORITY), 0, 0);

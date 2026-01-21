/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/drivers/i2c.h>
#include <nrfx_twis.h>

#include <zephyr/ztest.h>

#define NODE_TWIM	    DT_NODELABEL(sensor)
#define NODE_TWIS	    DT_ALIAS(i2c_slave)
#define MEASUREMENT_REPEATS 10

#define TWIS_MEMORY_SECTION                                                                        \
	COND_CODE_1(DT_NODE_HAS_PROP(NODE_TWIS, memory_regions),                                   \
		    (__attribute__((__section__(                                                   \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(NODE_TWIS, memory_regions)))))), \
		    ())

#define TEST_BUFFER_SIZE 128

static nrfx_twis_t twis = {.p_reg = (NRF_TWIS_Type *)DT_REG_ADDR(NODE_TWIS)};
static uint8_t i2c_slave_buffer[TEST_BUFFER_SIZE] TWIS_MEMORY_SECTION;
static uint8_t i2c_master_buffer[TEST_BUFFER_SIZE];

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
		nrfx_twis_tx_prepare(&twis, i2c_slave_buffer, TEST_BUFFER_SIZE);
		break;
	case NRFX_TWIS_EVT_READ_DONE:
		break;
	case NRFX_TWIS_EVT_WRITE_REQ:
		nrfx_twis_rx_prepare(&twis, i2c_slave_buffer, TEST_BUFFER_SIZE);
		break;
	case NRFX_TWIS_EVT_WRITE_DONE:
		break;
	default:
		break;
	}
}

static void *test_setup(void)
{
	const nrfx_twis_config_t config = {
		.addr = {fixture.addr, 0},
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
	};
	int ret;

	zassert_true(device_is_ready(fixture.dev), "TWIM device is not ready");
	zassert_equal(0, nrfx_twis_init(&twis, &config, i2s_slave_handler),
		      "TWIS initialization failed");

	PINCTRL_DT_DEFINE(NODE_TWIS);
	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(NODE_TWIS), PINCTRL_STATE_DEFAULT);
	zassert_ok(ret);

	IRQ_CONNECT(DT_IRQN(NODE_TWIS), DT_IRQ(NODE_TWIS, priority), nrfx_twis_irq_handler, &twis,
		    0);

	nrfx_twis_enable(&twis);

	return NULL;
}

static void prepare_test_data(uint8_t *const buffer)
{
	for (size_t counter = 0; counter < TEST_BUFFER_SIZE; counter++) {
		buffer[counter] = counter;
	}
}

static void cleanup_buffers(void *nullp)
{
	memset(fixture.slave_buffer, 0, TEST_BUFFER_SIZE);
	memset(fixture.master_buffer, 0, TEST_BUFFER_SIZE);
}

static void configure_twim(void)
{
	uint32_t i2c_config = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;

	zassert_ok(i2c_configure(fixture.dev, i2c_config));
}

/*
 * Verify driver ability to recover after long clock stretching (corner case)
 * During I2C transaction, TWIM SCL is 0 either before or after disabling TWIM.
 */
ZTEST(i2c_pan, test_clock_stretching_recovery)
{
	int ret;
	uint32_t scl_pin;

	configure_twim();
	prepare_test_data(fixture.master_buffer);
	memset(fixture.slave_buffer, 0, TEST_BUFFER_SIZE);

	TC_PRINT("STEP 1: TWIM - TWIS transmission with valid configuration\n");
	scl_pin = nrf_twis_scl_pin_get(twis.p_reg);
	TC_PRINT("TWIS SCL pin: 0x%x\n", scl_pin);
	ret = i2c_read(fixture.dev, fixture.master_buffer, TEST_BUFFER_SIZE, fixture.addr);
	zassert_ok(ret, "i2c_read failed: %d\n", ret);
	zassert_mem_equal(fixture.master_buffer, fixture.slave_buffer, TEST_BUFFER_SIZE);

	TC_PRINT("STEP 2: TWIM - TWIS transmission with SCL pulled low (EIO error is expected)\n");
	/* Pull TWIS SCL pin low */
	nrf_twis_scl_pin_set(twis.p_reg, 1 << 31);
	nrf_gpio_cfg_output(scl_pin);
	nrf_gpio_pin_clear(scl_pin);
	TC_PRINT("TWIS SCL pin (disconnected): 0x%x\n", nrf_twis_scl_pin_get(twis.p_reg));
	memset(fixture.slave_buffer, 0, TEST_BUFFER_SIZE);
	ret = i2c_read(fixture.dev, fixture.master_buffer, TEST_BUFFER_SIZE, fixture.addr);
	zassert_equal(ret, -EIO, "i2c_read failed with different error than expeced (EIO) %d\n",
		      ret);

	TC_PRINT("STEP 3: TWIM - TWIS transmission after TWIS pin reconfiguration\n");
	/* Restore original TWIS pin configuration */
	PINCTRL_DT_DEFINE(NODE_TWIS);
	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(NODE_TWIS), PINCTRL_STATE_DEFAULT);
	zassert_ok(ret);
	TC_PRINT("TWIS SCL pin (reconfigured): 0x%x\n", nrf_twis_scl_pin_get(twis.p_reg));

	ret = i2c_read(fixture.dev, fixture.master_buffer, TEST_BUFFER_SIZE, fixture.addr);
	zassert_ok(ret, "i2c_read failed (after SCL release): %d\n", ret);
	zassert_mem_equal(fixture.master_buffer, fixture.slave_buffer, TEST_BUFFER_SIZE);
}

ZTEST_SUITE(i2c_pan, NULL, test_setup, NULL, cleanup_buffers, NULL);

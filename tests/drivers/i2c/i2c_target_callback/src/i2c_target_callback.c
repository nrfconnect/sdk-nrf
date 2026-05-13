/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/i2c_nrfx_twim.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <nrfx_twis.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_target_callback, LOG_LEVEL_INF);

BUILD_ASSERT(IS_ENABLED(CONFIG_I2C_TARGET_BUFFER_MODE),
	     "CONFIG_I2C_TARGET_BUFFER_MODE must be enabled");

#define I2C_TARGET_ADDR                  0x07
#define TEST_TRANSFER_BUF_SIZE           16

static const struct device *dev_controller = DEVICE_DT_GET(DT_ALIAS(i2c_controller));
static const struct device *dev_target = DEVICE_DT_GET(DT_ALIAS(i2c_target));

static uint8_t target_internal_buf[TEST_TRANSFER_BUF_SIZE];
static int cb_return_value;
static int cb_delay_value;

static void target_buf_write_received_cb(struct i2c_target_config *config,
					 uint8_t *ptr, uint32_t len)
{
	memcpy(target_internal_buf, ptr, MIN(len, TEST_TRANSFER_BUF_SIZE));
	LOG_INF("Target got write request");
	LOG_HEXDUMP_DBG(target_internal_buf, TEST_TRANSFER_BUF_SIZE,
			"Target got with write request");
}

static int target_buf_read_requested_cb(struct i2c_target_config *config,
					uint8_t **ptr, uint32_t *len)
{
	*ptr = target_internal_buf;
	*len = TEST_TRANSFER_BUF_SIZE;
	LOG_INF("Target got read request");
	LOG_HEXDUMP_DBG(target_internal_buf, TEST_TRANSFER_BUF_SIZE,
			"Target sends in read request");
	if (cb_delay_value) {
		k_busy_wait(cb_delay_value);
	}
	return cb_return_value;
}

static const struct i2c_target_callbacks target_callbacks = {
	.buf_write_received = target_buf_write_received_cb,
	.buf_read_requested = target_buf_read_requested_cb,
};

static struct i2c_target_config target_cfg = {
	.address = I2C_TARGET_ADDR,
	.callbacks = &target_callbacks
};

static void *suite_setup(void)
{
	zassert_true(device_is_ready(dev_controller), "dev_controller is not ready");
	zassert_true(device_is_ready(dev_target), "dev_target is not ready");
	zassert_equal(i2c_target_register(dev_target, &target_cfg), 0,
		      "dev_target can't register target");

	return NULL;
}

static void fill_buf(uint8_t *tx_block, uint8_t size, uint8_t offset)
{
	for (int i = 0; i < size; i++) {
		tx_block[i] = i + offset;
	}
}

ZTEST(i2c_target_callback, test_01_write_read_compare)
{
	int ret;
	uint8_t data_tx[TEST_TRANSFER_BUF_SIZE];
	uint8_t data_rx[TEST_TRANSFER_BUF_SIZE];

	/* Generate pseudo-random data. */
	fill_buf(data_tx, TEST_TRANSFER_BUF_SIZE, (uint8_t) k_cycle_get_32());
	LOG_HEXDUMP_DBG(data_tx, TEST_TRANSFER_BUF_SIZE, "Generated Data");

	/* Write data to the target device. */
	ret = i2c_write(dev_controller, data_tx, sizeof(data_tx), I2C_TARGET_ADDR);
	zassert_equal(ret, 0, "i2c_write failed (%d)", ret);

	/* Wait for I2C transmission. */
	k_msleep(100);

	/* Due to HW limitation callback must return 0. */
	cb_return_value = 0;
	cb_delay_value = 0;

	/* Read data from the target device. */
	ret = i2c_read(dev_controller, data_rx, sizeof(data_rx), I2C_TARGET_ADDR);
	zassert_equal(ret, 0, "i2c_read failed (%d)", ret);

	/* Compare if received data is identical to the transmitted one. */
	zassert_mem_equal(data_tx, data_rx, TEST_TRANSFER_BUF_SIZE);
}

ZTEST(i2c_target_callback, test_02_write_read_compare_with_delay)
{
	int ret;
	uint8_t data_tx[TEST_TRANSFER_BUF_SIZE];
	uint8_t data_rx[TEST_TRANSFER_BUF_SIZE];

	/* Generate pseudo-random data. */
	fill_buf(data_tx, TEST_TRANSFER_BUF_SIZE, (uint8_t) k_cycle_get_32());
	LOG_HEXDUMP_DBG(data_tx, TEST_TRANSFER_BUF_SIZE, "Generated Data");

	/* Write data to the target device. */
	ret = i2c_write(dev_controller, data_tx, sizeof(data_tx), I2C_TARGET_ADDR);
	zassert_equal(ret, 0, "i2c_write failed (%d)", ret);

	/* Wait for I2C transmission. */
	k_msleep(100);

	/* Due to HW limitation callback must return 0.
	 * Target pauses communication with clock stretching.
	 */
	cb_return_value = 0;
	cb_delay_value = (CONFIG_I2C_NRFX_TRANSFER_TIMEOUT - 10) * 1000;

	/* Read data from the target device. */
	ret = i2c_read(dev_controller, data_rx, sizeof(data_rx), I2C_TARGET_ADDR);
	zassert_equal(ret, 0, "i2c_read failed (%d)", ret);

	/* Compare if received data is identical to the transmitted one. */
	zassert_mem_equal(data_tx, data_rx, TEST_TRANSFER_BUF_SIZE);
}

ZTEST(i2c_target_callback, test_03_invalid_callback)
{
	int ret;
	uint8_t data_tx[TEST_TRANSFER_BUF_SIZE];
	uint8_t data_rx[TEST_TRANSFER_BUF_SIZE];

	/* Generate pseudo-random data. */
	fill_buf(data_tx, TEST_TRANSFER_BUF_SIZE, (uint8_t) k_cycle_get_32());
	LOG_HEXDUMP_DBG(data_tx, TEST_TRANSFER_BUF_SIZE, "Generated Data");

	/* Write data to the target device. */
	ret = i2c_write(dev_controller, data_tx, sizeof(data_tx), I2C_TARGET_ADDR);
	zassert_equal(ret, 0, "i2c_write failed (%d)", ret);

	/* Wait for I2C transmission. */
	k_msleep(100);

	/* Due to HW limitation callback must return 0.
	 * Otherwise, target blocks communication indefinitely (with clock stretching).
	 */
	cb_return_value = -EIO;
	cb_delay_value = 0;

	/* Read data from the target device.
	 * This is expected to fail.
	 */
	ret = i2c_read(dev_controller, data_rx, sizeof(data_rx), I2C_TARGET_ADDR);
	zassert_equal(ret, -ETIMEDOUT, "i2c_read returned (%d)", ret);

	/* I2C interface is now permamently blocked.
	 * Target forces CLK low (clock stretching).
	 * Controller is unable to generate START symbol.
	 */
	cb_return_value = 0;
	ret = i2c_write(dev_controller, data_tx, sizeof(data_tx), I2C_TARGET_ADDR);
	zassert_equal(ret, -ETIMEDOUT, "i2c_write returned (%d)", ret);
	ret = i2c_read(dev_controller, data_rx, sizeof(data_rx), I2C_TARGET_ADDR);
	zassert_equal(ret, -ETIMEDOUT, "i2c_read returned (%d)", ret);
}

ZTEST_SUITE(i2c_target_callback, NULL, suite_setup, NULL, NULL, NULL);

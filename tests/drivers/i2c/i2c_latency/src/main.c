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

#include <zephyr/drivers/counter.h>
#include <dk_buttons_and_leds.h>

#include <zephyr/ztest.h>

#define NODE_TWIM	    DT_NODELABEL(sensor)
#define NODE_TWIS	    DT_ALIAS(i2c_slave)
#define MEASUREMENT_REPEATS 10

#define TWIS_MEMORY_SECTION                                                                        \
	COND_CODE_1(DT_NODE_HAS_PROP(NODE_TWIS, memory_regions),                                   \
		    (__attribute__((__section__(                                                   \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(NODE_TWIS, memory_regions)))))), \
		    ())

#define TEST_TIMER_COUNT_TIME_LIMIT_MS 500
#define MAX_TEST_DATA_SIZE	       255
static nrfx_twis_t twis = {
	.p_reg = (NRF_TWIS_Type *)DT_REG_ADDR(NODE_TWIS)
};
const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_ALIAS(tst_timer));

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

static void configure_test_timer(const struct device *timer_dev, uint32_t count_time_ms)
{
	struct counter_alarm_cfg counter_cfg;

	counter_cfg.flags = 0;
	counter_cfg.ticks = counter_us_to_ticks(timer_dev, (uint64_t)count_time_ms * 1000);
	counter_cfg.user_data = &counter_cfg;
}

static uint32_t calculate_theoretical_transsmison_time_us(size_t buffer_size,
							  uint8_t i2c_speed_setting)
{
	/* i2c_get_config is not imeplemnted on NRF devices
	 * configuration here is hard-coded
	 */
	uint32_t i2c_speed;
	float ratio;
	/* 7 address bits + 1 R/W bit + 1 ACK bit */
	uint32_t address_bits = 7 + 1 + 1;
	/* 8 data bits + 1 ACK bit */
	uint32_t data_bits = 8 + 1;
	/* dead time bits */
	uint32_t dead_time_bits = 2;
	/*
	 *  clock stretching bits
	 * note that this is very rough approximation
	 * it is hard to estimate the actual
	 * time required for clock stretching
	 */
	uint32_t clk_stretch_bits;

	switch (i2c_speed_setting) {
	case I2C_SPEED_STANDARD:
		i2c_speed = 100000;
		clk_stretch_bits = 0;
		break;
	case I2C_SPEED_FAST:
		i2c_speed = 400000;
		clk_stretch_bits = 2;
		break;
	case I2C_SPEED_FAST_PLUS:
		i2c_speed = 1000000;
		clk_stretch_bits = 4;
		break;
	default:
		i2c_speed = 100000;
		break;
	}

	ratio = 1000000.0 / i2c_speed;

	/* Address is sent once at the beginning
	 * when i2c_write/read methods are used
	 * there is one START and one STOP bit
	 */
	return (uint32_t)(ratio * (float)(1 + address_bits + dead_time_bits +
					  (data_bits + clk_stretch_bits) * buffer_size + 1));
}

/*
 * Note that the calculated 'theoretical_transmission_time_us'
 * is not accurate enough to perform a stric assesment
 * especially for 1 byte transmission with higher speeds
 */
static void assess_measurement_result(uint64_t timer_value_us,
				      uint32_t theoretical_transmission_time_us, size_t buffer_size)
{
	uint64_t maximal_allowed_transmission_time_us;

	if (buffer_size == 1) {
		/* 500% */
		maximal_allowed_transmission_time_us = (1 + 5) * theoretical_transmission_time_us;
	} else {
		/* 200% */
		maximal_allowed_transmission_time_us = (1 + 2) * theoretical_transmission_time_us;
	}

	zassert_true(timer_value_us < maximal_allowed_transmission_time_us,
		     "Measured call latency is over the specified limit");
}

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

static void *test_setup(void)
{
	const nrfx_twis_config_t config = {
		.addr = {fixture.addr, 0},
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
	};
	int ret;

	zassert_true(device_is_ready(fixture.dev), "TWIM device is not ready");
	zassert_equal(dk_leds_init(), 0, "DK leds init failed");

	dk_set_led_off(DK_LED1);
	zassert_equal(0, nrfx_twis_init(&twis, &config, i2s_slave_handler),
		      "TWIS initialization failed");

	PINCTRL_DT_DEFINE(NODE_TWIS);
	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(NODE_TWIS), PINCTRL_STATE_DEFAULT);
	zassert_ok(ret);

	IRQ_CONNECT(DT_IRQN(NODE_TWIS), DT_IRQ(NODE_TWIS, priority),
		    nrfx_twis_irq_handler, &twis, 0);

	nrfx_twis_enable(&twis);

	return NULL;
}

static void prepare_test_data(uint8_t *const buffer, size_t buffer_size)
{
	for (size_t counter = 0; counter < buffer_size; counter++) {
		buffer[counter] = counter;
	}
}

static void cleanup_buffers(void *nullp)
{
	memset(fixture.slave_buffer, 0, MAX_TEST_DATA_SIZE);
	memset(fixture.master_buffer, 0, MAX_TEST_DATA_SIZE);
}

/*
 * Select valu from
 * I2C_SPEED_STANDARD
 * I2C_SPEED_FAST
 * I2C_SPEED_FAST_PLUS
 */
static void configure_twim_speed(uint8_t i2c_speed_setting)
{
	uint32_t i2c_config = I2C_SPEED_SET(i2c_speed_setting) | I2C_MODE_CONTROLLER;

	TC_PRINT("TWIM speed setting: %d\n", i2c_speed_setting);
	zassert_ok(i2c_configure(fixture.dev, i2c_config));
}

static void test_i2c_read_latency(size_t buffer_size, uint8_t i2c_speed_setting)
{
	int ret;
	uint32_t tst_timer_value;
	uint64_t timer_value_us[MEASUREMENT_REPEATS];
	uint64_t average_timer_value_us = 0;
	uint32_t theoretical_transmission_time_us;

	TC_PRINT("I2C read latency in test with buffer size: %u bytes and speed setting: %u\n",
		 buffer_size, i2c_speed_setting);
	configure_twim_speed(i2c_speed_setting);

	prepare_test_data(fixture.slave_buffer, buffer_size);
	configure_test_timer(tst_timer_dev, TEST_TIMER_COUNT_TIME_LIMIT_MS);

	theoretical_transmission_time_us =
		calculate_theoretical_transsmison_time_us(buffer_size, i2c_speed_setting);

	for (uint32_t repeat_counter = 0; repeat_counter < MEASUREMENT_REPEATS; repeat_counter++) {
		memset(fixture.slave_buffer, 0, buffer_size);
		counter_reset(tst_timer_dev);
		dk_set_led_on(DK_LED1);
		counter_start(tst_timer_dev);
		ret = i2c_read(fixture.dev, fixture.master_buffer, buffer_size, fixture.addr);
		counter_get_value(tst_timer_dev, &tst_timer_value);
		dk_set_led_off(DK_LED1);
		counter_stop(tst_timer_dev);
		timer_value_us[repeat_counter] =
			counter_ticks_to_us(tst_timer_dev, tst_timer_value);
		average_timer_value_us += timer_value_us[repeat_counter];

		zassert_ok(ret);
		zassert_mem_equal(fixture.master_buffer, fixture.slave_buffer, buffer_size);
	}

	average_timer_value_us /= MEASUREMENT_REPEATS;

	TC_PRINT("Calculated transmission time (for %u bytes) [us]: %u\n", buffer_size,
		 theoretical_transmission_time_us);
	TC_PRINT("i2c_read: measured transmission time (for %u bytes) [us]: %llu\n", buffer_size,
		 average_timer_value_us);
	TC_PRINT("i2c_read: measured - claculated time delta (for %d bytes) [us]: %lld\n",
		 buffer_size, average_timer_value_us - theoretical_transmission_time_us);

	assess_measurement_result(average_timer_value_us, theoretical_transmission_time_us,
				  buffer_size);
}

static void test_i2c_write_latency(size_t buffer_size, uint8_t i2c_speed_setting)
{
	int ret;
	uint32_t tst_timer_value;
	uint64_t timer_value_us[MEASUREMENT_REPEATS];
	uint64_t average_timer_value_us = 0;
	uint32_t theoretical_transmission_time_us;

	TC_PRINT("I2C write latency in test with buffer size: %u bytes and speed setting: %u\n",
		 buffer_size, i2c_speed_setting);
	configure_twim_speed(i2c_speed_setting);

	prepare_test_data(fixture.master_buffer, buffer_size);
	configure_test_timer(tst_timer_dev, TEST_TIMER_COUNT_TIME_LIMIT_MS);

	theoretical_transmission_time_us =
		calculate_theoretical_transsmison_time_us(buffer_size, i2c_speed_setting);

	for (uint32_t repeat_counter = 0; repeat_counter < MEASUREMENT_REPEATS; repeat_counter++) {
		memset(fixture.slave_buffer, 0, buffer_size);
		counter_reset(tst_timer_dev);
		dk_set_led_on(DK_LED1);
		counter_start(tst_timer_dev);
		ret = i2c_write(fixture.dev, fixture.master_buffer, buffer_size, fixture.addr);
		counter_get_value(tst_timer_dev, &tst_timer_value);
		dk_set_led_off(DK_LED1);
		counter_stop(tst_timer_dev);
		timer_value_us[repeat_counter] =
			counter_ticks_to_us(tst_timer_dev, tst_timer_value);
		average_timer_value_us += timer_value_us[repeat_counter];

		zassert_ok(ret);
		zassert_mem_equal(fixture.slave_buffer, fixture.slave_buffer, buffer_size);
	}

	average_timer_value_us /= MEASUREMENT_REPEATS;

	TC_PRINT("Calculated transmission time (for %u bytes) [us]: %u\n", buffer_size,
		 theoretical_transmission_time_us);
	TC_PRINT("i2c_write: measured transmission time (for %u bytes) [us]: %llu\n", buffer_size,
		 average_timer_value_us);
	TC_PRINT("i2c_write: measured - claculated time delta (for %d bytes) [us]: %lld\n",
		 buffer_size, average_timer_value_us - theoretical_transmission_time_us);

	assess_measurement_result(average_timer_value_us, theoretical_transmission_time_us,
				  buffer_size);
}

ZTEST(i2c_transmission_latency, test_i2c_read_call_latency_standard_speed)
{
	test_i2c_read_latency(1, I2C_SPEED_STANDARD);
	test_i2c_read_latency(10, I2C_SPEED_STANDARD);
	test_i2c_read_latency(128, I2C_SPEED_STANDARD);
	test_i2c_read_latency(240, I2C_SPEED_STANDARD);
}

ZTEST(i2c_transmission_latency, test_i2c_read_call_latency_fast_speed)
{
	test_i2c_read_latency(1, I2C_SPEED_FAST);
	test_i2c_read_latency(10, I2C_SPEED_FAST);
	test_i2c_read_latency(128, I2C_SPEED_FAST);
	test_i2c_read_latency(240, I2C_SPEED_FAST);
}

ZTEST(i2c_transmission_latency, test_i2c_read_call_latency_fast_plus_speed)
{
	Z_TEST_SKIP_IFDEF(CONFIG_SOC_NRF54L15_CPUAPP);
	Z_TEST_SKIP_IFDEF(CONFIG_SOC_NRF52840);

	test_i2c_read_latency(1, I2C_SPEED_FAST_PLUS);
	test_i2c_read_latency(10, I2C_SPEED_FAST_PLUS);
	test_i2c_read_latency(128, I2C_SPEED_FAST_PLUS);
	test_i2c_read_latency(240, I2C_SPEED_FAST_PLUS);
}

ZTEST(i2c_transmission_latency, test_i2c_write_call_latency_standard_speed)
{
	test_i2c_write_latency(1, I2C_SPEED_STANDARD);
	test_i2c_write_latency(10, I2C_SPEED_STANDARD);
	test_i2c_write_latency(128, I2C_SPEED_STANDARD);
	test_i2c_write_latency(240, I2C_SPEED_STANDARD);
}

ZTEST(i2c_transmission_latency, test_i2c_write_call_latency_fast_speed)
{
	test_i2c_write_latency(1, I2C_SPEED_FAST);
	test_i2c_write_latency(10, I2C_SPEED_FAST);
	test_i2c_write_latency(128, I2C_SPEED_FAST);
	test_i2c_write_latency(240, I2C_SPEED_FAST);
}

ZTEST(i2c_transmission_latency, test_i2c_write_call_latency_fast_plus_speed)
{
	Z_TEST_SKIP_IFDEF(CONFIG_SOC_NRF54L15_CPUAPP);
	Z_TEST_SKIP_IFDEF(CONFIG_SOC_NRF52840);

	test_i2c_write_latency(1, I2C_SPEED_FAST_PLUS);
	test_i2c_write_latency(10, I2C_SPEED_FAST_PLUS);
	test_i2c_write_latency(128, I2C_SPEED_FAST_PLUS);
	test_i2c_write_latency(240, I2C_SPEED_FAST_PLUS);
}

ZTEST_SUITE(i2c_transmission_latency, NULL, test_setup, NULL, cleanup_buffers, NULL);

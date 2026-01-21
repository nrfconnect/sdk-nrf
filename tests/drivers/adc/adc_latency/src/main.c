/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/ztest.h>
#include <dmm.h>

#define ADC_NODE			      DT_NODELABEL(dut_adc)
#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),
#define MEASUREMENT_REPEATS		      10
#define TEST_TIMER_COUNT_TIME_LIMIT_MS	      500
#define ADC_MINIMAL_READING_FOR_HIGH_LEVEL    4000
#define ADC_BUFFER_MAX_SIZE		      500
#define MAX_TOLERANCE			      3.0

static int16_t adc_sample_buffer[ADC_BUFFER_MAX_SIZE] DMM_MEMORY_SECTION(DT_NODELABEL(adc));

static const struct device *adc = DEVICE_DT_GET(ADC_NODE);
static const struct gpio_dt_spec test_gpio = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), test_gpios);
static struct adc_channel_cfg channel_cfgs[] = {
	DT_FOREACH_CHILD_SEP(ADC_NODE, ADC_CHANNEL_CFG_DT, (,))};
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};
const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_ALIAS(tst_timer));

void configure_test_timer(const struct device *timer_dev, uint32_t count_time_ms)
{
	struct counter_alarm_cfg counter_cfg;

	counter_cfg.flags = 0;
	counter_cfg.ticks = counter_us_to_ticks(timer_dev, (uint64_t)count_time_ms * 1000);
	counter_cfg.user_data = &counter_cfg;
}

void *test_setup(void)
{
	zassert_true(adc_is_ready_dt(&adc_channels[0]), "ADC channel 0 is not ready\n");
	zassert_true(gpio_is_ready_dt(&test_gpio), "Test GPIO is not ready\n");
	zassert_ok(gpio_pin_configure_dt(&test_gpio, GPIO_OUTPUT),
		   "Failed to configure test pin\n");
	gpio_pin_set_dt(&test_gpio, 1);

	return NULL;
}

static uint32_t calculate_theoretical_sampling_time_us(uint32_t sampling_interval_us,
						       uint16_t extra_samples)
{
	return sampling_interval_us * (1 + extra_samples);
}

/*
 * Note that interval_us >> (tACQ + tconv) must be fulfilled
 * to achieve constant sampling rate
 * tconv is ~2us
 */
static void test_adc_latency(uint32_t acquisition_time_us, uint32_t sampling_interval_us,
			     uint16_t extra_samples)
{
	int ret;
	uint32_t tst_timer_value;
	uint64_t timer_value_us[MEASUREMENT_REPEATS];
	uint64_t average_timer_value_us = 0;
	uint64_t theoretical_sampling_time_us;
	uint64_t maximal_allowed_sampling_time_us;
#if !defined(CONFIG_SOC_NRF52840)
	uint16_t adc_buffer_length = 1 + extra_samples;
#endif
	const struct adc_sequence_options options = {
		.interval_us = sampling_interval_us,
		.extra_samplings = extra_samples,
	};
	struct adc_sequence sequence = {
		.options = &options,
		.buffer = adc_sample_buffer,
		.buffer_size = sizeof(adc_sample_buffer),
		.channels = 1,
		.resolution = 12,
	};

	channel_cfgs[0].acquisition_time =
		ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, acquisition_time_us);

	TC_PRINT("ADC latency test for %uus acquisition time, %uus sampling interval, %u extra "
		 "samples\n",
		 acquisition_time_us, sampling_interval_us, extra_samples);

	ret = adc_channel_setup(adc, &channel_cfgs[0]);
	zassert_ok(ret, "ADC channel 0 setup failed\n: %d", ret);

	configure_test_timer(tst_timer_dev, TEST_TIMER_COUNT_TIME_LIMIT_MS);

	theoretical_sampling_time_us =
		calculate_theoretical_sampling_time_us(sampling_interval_us, extra_samples);
	maximal_allowed_sampling_time_us = MAX_TOLERANCE * theoretical_sampling_time_us;

	for (uint32_t repeat_counter = 0; repeat_counter < MEASUREMENT_REPEATS; repeat_counter++) {
		counter_reset(tst_timer_dev);
		counter_start(tst_timer_dev);
		ret = adc_read(adc, &sequence);
		counter_get_value(tst_timer_dev, &tst_timer_value);
		counter_stop(tst_timer_dev);
		zassert_ok(ret, "ADC read failed: %d\n", ret);

		timer_value_us[repeat_counter] =
			counter_ticks_to_us(tst_timer_dev, tst_timer_value);
		average_timer_value_us += timer_value_us[repeat_counter];

#if !defined(CONFIG_SOC_NRF52840)
		/*
		 * There is no available loopback option
		 * to verify the sample data on nrf52840
		 */
		for (int i = 0; i < adc_buffer_length; i++) {
			zassert_true(adc_sample_buffer[i] > ADC_MINIMAL_READING_FOR_HIGH_LEVEL,
				     "Sample [%u] = %d, is below the  minimal ADC reading for high level\n",
				     i, adc_sample_buffer[i]);
		}
#endif
	}

	average_timer_value_us /= MEASUREMENT_REPEATS;

	TC_PRINT("Calculated ADC sampling time for %uus acquisition time, %uus sampling interval, "
		 "%u "
		 "extra samples [us]: %llu\n",
		 acquisition_time_us, sampling_interval_us, extra_samples,
		 theoretical_sampling_time_us);
	TC_PRINT("Measured ADC sampling time for %uus acquisition time, %uus sampling interval, %u "
		 "extra samples [us]: %llu\n",
		 acquisition_time_us, sampling_interval_us, extra_samples, average_timer_value_us);
	TC_PRINT("Measured - calculated ADC sampling time for %uus acquisition time, %uus sampling "
		 "interval, %u "
		 "extra samples [us]: %lld\n",
		 acquisition_time_us, sampling_interval_us, extra_samples,
		 average_timer_value_us - theoretical_sampling_time_us);

	zassert_true(average_timer_value_us < maximal_allowed_sampling_time_us,
		     "Measured sampling time is over the specified limit\n");
}

ZTEST(adc_latency, test_adc_read_call_latency)
{
#if defined(CONFIG_SOC_NRF54H20) || defined(CONFIG_SOC_NRF54L15)
	test_adc_latency(5, 70, 9);
	test_adc_latency(5, 70, 99);
	test_adc_latency(5, 70, 499);
	test_adc_latency(10, 80, 9);
	test_adc_latency(10, 80, 99);
	test_adc_latency(10, 80, 499);
	test_adc_latency(20, 100, 9);
	test_adc_latency(20, 100, 99);
	test_adc_latency(20, 100, 499);
#endif
	test_adc_latency(40, 200, 9);
	test_adc_latency(40, 200, 99);
	test_adc_latency(40, 200, 499);
}

ZTEST_SUITE(adc_latency, NULL, test_setup, NULL, NULL, NULL);

/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <hal/nrf_saadc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/ztest.h>
#include <dmm.h>

#define ADC_NODE			      DT_NODELABEL(dut_adc)
#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),
#define ADC_MINIMAL_READING_FOR_HIGH_LEVEL    4000
#define ADC_BUFFER_SIZE			      2
#define ADC_TESCTRL_PWRUP_MASK		      (1 << 29 | 1 << 28)
#define ADC_TESCTRL_PWRUP_POS		      28
#define TESCTRL_REG_OFFSET		      0x63CU
#define ADC_MINIMAL_READING_FOR_HIGH_LEVEL    4000

static int16_t adc_sample_buffer[ADC_BUFFER_SIZE] DMM_MEMORY_SECTION(DT_NODELABEL(adc));
NRF_SAADC_Type *const adc_reg = (NRF_SAADC_Type *const)DT_REG_ADDR(DT_NODELABEL(adc));

static const struct device *adc = DEVICE_DT_GET(ADC_NODE);
static const struct gpio_dt_spec test_gpio = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), test_gpios);
static struct adc_channel_cfg channel_cfgs[] = {
	DT_FOREACH_CHILD_SEP(ADC_NODE, ADC_CHANNEL_CFG_DT, (,))};
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};
const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_ALIAS(tst_timer));

void *test_setup(void)
{
	zassert_true(adc_is_ready_dt(&adc_channels[0]), "ADC channel 0 is not ready\n");
	zassert_true(gpio_is_ready_dt(&test_gpio), "Test GPIO is not ready\n");
	zassert_ok(gpio_pin_configure_dt(&test_gpio, GPIO_OUTPUT),
		   "Failed to configure test pin\n");
	gpio_pin_set_dt(&test_gpio, 1);

	return NULL;
}

void verify_samples(int16_t *sample_buffer)
{
	for (int i = 0; i < ADC_BUFFER_SIZE; i++) {
		zassert_true(adc_sample_buffer[i] > ADC_MINIMAL_READING_FOR_HIGH_LEVEL,
			     "Sample [%u] = %d, is below the  minimal ADC reading for high level\n",
			     i, adc_sample_buffer[i]);
	}
}

/*
 * HMPAN-210
 * PAN conditions:
 * TASKS_SAMPLE is triggered after
 * the initial enabling of SAADC or after TASKS_STOP
 * PAN symptoms:
 * The first SAADC output sample
 * after TASKS_SAMPLE might be incorrect.
 * The SAADC raw measurements are correct
 * except for the case we choose T_ACQ = 0,T_CONV = 0.
 */
ZTEST(adc_pan, test_adc_hmpan_210_workaround)
{
	int ret;
	uint32_t channel_0_config_reg;
	uint32_t testctrl_pwrupctrl_val;
	uint32_t *tesctrl_reg = (uint32_t *)(adc_reg) + TESCTRL_REG_OFFSET / sizeof(uint32_t);

	const struct adc_sequence_options options = {
		.interval_us = 0,
		.extra_samplings = 1,
	};

	struct adc_sequence sequence = {
		.options = &options,
		.buffer = adc_sample_buffer,
		.buffer_size = sizeof(adc_sample_buffer),
		.channels = 1,
		.resolution = 12,
	};

	channel_cfgs[0].acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 3);

	ret = adc_channel_setup(adc, &channel_cfgs[0]);
	zassert_ok(ret, "ADC channel 0 setup failed\n: %d", ret);
	ret = adc_read(adc, &sequence);
	zassert_ok(ret, "ADC read failed: %d\n", ret);

	TC_PRINT("ADC TESTCTRL register address: %p\n", tesctrl_reg);
	testctrl_pwrupctrl_val = (*tesctrl_reg & ADC_TESCTRL_PWRUP_MASK) >> ADC_TESCTRL_PWRUP_POS;
	TC_PRINT("ADC TESTCTRL.PWRUPCTRL = %u\n", testctrl_pwrupctrl_val);

	channel_0_config_reg = adc_reg->CH[0].CONFIG;
	TC_PRINT("ADC CH0 TACQ = %u\n",
		 (uint32_t)((channel_0_config_reg & SAADC_CH_CONFIG_TACQ_Msk) >>
			    SAADC_CH_CONFIG_TACQ_Pos));
	TC_PRINT("ADC CH0 TCONV = %u\n",
		 (uint32_t)((channel_0_config_reg & SAADC_CH_CONFIG_TCONV_Msk) >>
			    SAADC_CH_CONFIG_TCONV_Pos));

	zassert_true(testctrl_pwrupctrl_val == 1, "TESTCTRL.PWRUPCTRL should be equal to 1\n");

	TC_PRINT("After ADC enable\n");
	verify_samples(adc_sample_buffer);

	TC_PRINT("After ADC TASK_STOP\n");
	ret = adc_read(adc, &sequence);
	zassert_ok(ret, "ADC read failed: %d\n", ret);
	verify_samples(adc_sample_buffer);
}

ZTEST_SUITE(adc_pan, NULL, test_setup, NULL, NULL, NULL);

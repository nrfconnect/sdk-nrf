/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"
#include <nrfx_i2s.h>
#include <zephyr/drivers/pinctrl.h>

static nrfx_i2s_t i2s_instance = NRFX_I2S_INSTANCE(DT_REG_ADDR(DT_NODELABEL(i2s_dut)));
const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_ALIAS(tst_timer));

typedef struct {
	uint32_t buffer_rx[2][WORDS_COUNT];
	uint32_t buffer_tx[2][WORDS_COUNT];
} tst_buffers_t;

typedef struct {
	nrf_i2s_mck_t mck_setup;
	nrf_i2s_ratio_t ratio;
} tst_clk_setup_t;

static tst_buffers_t tst_buffers;

static nrfx_i2s_config_t tst_config;

static volatile uint16_t m_iteration_counter;
static uint16_t m_sample_value_to_send;
static uint16_t m_sample_value_expected;
static uint16_t m_zero_samples_to_ignore;
static volatile bool m_stopped_arrived;

/*
 * These functions below are taken from
 * the 'nrfx-verifcation' i2s test suite
 */
static void fill_buffer(uint32_t *p_buffer)
{
	uint16_t i;

	for (i = 0; i < WORDS_COUNT; ++i) {
		((uint16_t *)p_buffer)[2 * i] = m_sample_value_to_send - 1;
		((uint16_t *)p_buffer)[2 * i + 1] = m_sample_value_to_send + 1;
		++m_sample_value_to_send;
	}
}

static void check_buffer(uint32_t const *p_buffer)
{
	uint16_t i;

	for (i = 0; i < WORDS_COUNT; ++i) {
		uint16_t sample_l = ((uint16_t const *)p_buffer)[2 * i];
		uint16_t sample_r = ((uint16_t const *)p_buffer)[2 * i + 1];

		if (m_zero_samples_to_ignore > 0 && (sample_l == 0 || sample_r == 0)) {
			--m_zero_samples_to_ignore;
		} else {
			m_zero_samples_to_ignore = 0;
			__ASSERT((uint16_t)(m_sample_value_expected - 1) == sample_l, "sample L invalid\n");
			__ASSERT((uint16_t)(m_sample_value_expected + 1) == sample_r, "sample R invalid\n");

			++m_sample_value_expected;
		}
	}
}

static void data_handler(nrfx_i2s_buffers_t const *p_released, uint32_t status)
{
	__ASSERT_NO_MSG(p_released);

	if (status & NRFX_I2S_STATUS_TRANSFER_STOPPED) {
		m_stopped_arrived = true;
	}

	if (!(status & NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED)) {
		return;
	}

	__ASSERT(m_stopped_arrived, "New buffers were requested after stopped event.");

	if (!p_released->p_rx_buffer) {
		nrfx_i2s_buffers_t const next_buffers = {
			.p_rx_buffer = tst_buffers.buffer_rx[1],
			.p_tx_buffer = tst_buffers.buffer_tx[1],
			.buffer_size = WORDS_COUNT,
		};
		__ASSERT_NO_MSG(nrfx_i2s_next_buffers_set(&i2s_instance, &next_buffers) == 0);

		fill_buffer(tst_buffers.buffer_tx[1]);
	} else {
		check_buffer(p_released->p_rx_buffer);
		__ASSERT_NO_MSG(nrfx_i2s_next_buffers_set(&i2s_instance, p_released) == 0);
		++m_iteration_counter;

		__ASSERT(p_released->p_tx_buffer, "TX bufffer not released\n");
		fill_buffer((uint32_t *)p_released->p_tx_buffer);
	}
}

static void setup_clks_for_given_sample_rate(uint32_t frame_clk_freq,
					     tst_clk_setup_t *tst_clk_setup)
{
	if (frame_clk_freq == 16000) {
		tst_clk_setup->mck_setup = NRF_I2S_MCK_32MDIV21;
        tst_clk_setup->ratio = NRF_I2S_RATIO_96X;
	} else if (frame_clk_freq == 44100) {
		tst_clk_setup->mck_setup = NRF_I2S_MCK_32MDIV23;
        tst_clk_setup->ratio = NRF_I2S_RATIO_32X;
	} else if (frame_clk_freq == 48000) {
		tst_clk_setup->mck_setup = NRF_I2S_MCK_32MDIV21;
        tst_clk_setup->ratio = NRF_I2S_RATIO_32X;
	} else {
		__ASSERT(false, "Frame clock %u [Hz] is not supported in test\n", frame_clk_freq);
	}
}

static void configure_i2s(nrfx_i2s_t *i2s_inst, uint32_t frame_clk_freq)
{
	int ret;
	tst_clk_setup_t tst_clk_setup;

	tst_config.mode = NRF_I2S_MODE_MASTER;
	tst_config.format = NRF_I2S_FORMAT_I2S;
	tst_config.alignment = NRF_I2S_ALIGN_LEFT;
	tst_config.sample_width = NRF_I2S_SWIDTH_16BIT;
	tst_config.channels = NRF_I2S_CHANNELS_STEREO;
	tst_config.sck_pin = i2s_inst->p_reg->PSEL.SCK;
	tst_config.lrck_pin = i2s_inst->p_reg->PSEL.FSYNC;
	tst_config.mck_pin = i2s_inst->p_reg->PSEL.MCK;
	tst_config.sdout_pin = i2s_inst->p_reg->PSEL.SDOUT;
	tst_config.sdin_pin = i2s_inst->p_reg->PSEL.SDIN;

	tst_config.skip_gpio_cfg = true;
	tst_config.skip_psel_cfg = true;

	PINCTRL_DT_DEFINE(DT_NODELABEL(i2s_dut));
	pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(i2s_dut)), PINCTRL_STATE_DEFAULT);

	setup_clks_for_given_sample_rate(frame_clk_freq, &tst_clk_setup);

	tst_config.prescalers.mck_setup = tst_clk_setup.mck_setup;
	tst_config.prescalers.ratio = tst_clk_setup.ratio;

	ret = nrfx_i2s_init(i2s_inst, &tst_config, data_handler);
	__ASSERT(ret == 0, "I2S init failed: %d\n", ret);
}

int main(void)
{
	int ret;
	uint32_t tst_timer_value;
	uint64_t timer_value_us;
	int64_t uptime;

	dk_leds_init();

	/* Zephyr boot done */
	dk_set_led_on(DK_LED1);

	uptime = k_uptime_get();
	printk("Uptime (after boot) [ms]: %llu\n", uptime);

	configure_test_timer(tst_timer_dev, TEST_TIMER_COUNT_TIME_LIMIT_MS);

	/* Time measurement start */
	counter_reset(tst_timer_dev);
	counter_start(tst_timer_dev);

	/* Configure I2S */
	configure_i2s(&i2s_instance, SAMPLE_RATE);

	/* Alloc memory and fill TX buffer */
	memset(tst_buffers.buffer_rx[0], 0xCC, WORDS_COUNT * sizeof(uint32_t));
	memset(tst_buffers.buffer_rx[1], 0xCC, WORDS_COUNT * sizeof(uint32_t));
	fill_buffer(tst_buffers.buffer_tx[0]);

	nrfx_i2s_buffers_t const buffers = {
		.p_rx_buffer = tst_buffers.buffer_rx[0],
		.p_tx_buffer = tst_buffers.buffer_tx[0],
		.buffer_size = WORDS_COUNT,
	};

	ret = nrfx_i2s_start(&i2s_instance, &buffers, 0);
	__ASSERT(ret == 0, "I2S start failed: %d\n", ret);

	/* I2S ready */
	dk_set_led_on(DK_LED2);
	printk("TDM ready\n");
	uptime = k_uptime_get();

	timer_value_us = counter_ticks_to_us(tst_timer_dev, tst_timer_value);

	printk("[NRFX] TDM, sample width: 16, words count: %d, channels: %d, sample rate [Hz]: %ld\n",
	       WORDS_COUNT, NUMBER_OF_CHANNELS, SAMPLE_RATE);
	printk("Time from boot to TDM started [us]: %llu\n", timer_value_us);
	printk("Uptime [ms]: %llu\n", uptime);

	return 0;
}
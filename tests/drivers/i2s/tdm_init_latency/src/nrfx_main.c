/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"
#include <nrfx_i2s.h>

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

static nrfx_i2s_config_t tst_config =
	NRFX_I2S_DEFAULT_CONFIG(NRFX_I2S_SCK_PIN, NRFX_I2S_LRCLK_PIN, NRF_I2S_PIN_NOT_CONNECTED,
				NRFX_I2S_SDOUT_PIN, NRFX_I2S_SDIN_PIN);

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
		//
		if (m_zero_samples_to_ignore > 0 && (sample_l == 0 || sample_r == 0)) {
			--m_zero_samples_to_ignore;
		} else {
			m_zero_samples_to_ignore = 0;
			TEST_ASSERT_EQUAL_HEX16((uint16_t)(m_sample_value_expected - 1), sample_l);
			TEST_ASSERT_EQUAL_HEX16((uint16_t)(m_sample_value_expected + 1), sample_r);

			++m_sample_value_expected;
		}
	}
}

static void data_handler(nrfx_i2s_buffers_t const *p_released, uint32_t status)
{
	// 'nrfx_i2s_next_buffers_set' is called directly from the handler
	// each time next buffers are requested, so data corruption is not
	// expected.
	__ASSERT_NO_MSG(p_released);

	if (status & NRFX_I2S_STATUS_TRANSFER_STOPPED) {
		m_stopped_arrived = true;
	}

	// When the handler is called after the transfer has been stopped
	// (no next buffers are needed, only the used buffers are to be
	// released), there is nothing to do.
	if (!(status & NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED)) {
		return;
	}

	// Check if the driver ignored all other events after stopped event
	// this is particularly useful for testing nRF52 Series erratum 55
	__ASSERT(m_stopped_arrived, "New buffers were requested after stopped event.");

	// First call of this handler occurs right after the transfer is started.
	// No data has been transferred yet at this point, so there is nothing to
	// check. Only the buffers for the next part of the transfer should be
	// provided.
	if (!p_released->p_rx_buffer) {
		nrfx_i2s_buffers_t const next_buffers = {
			.p_rx_buffer = m_buffers.buffer_rx[1],
			.p_tx_buffer = m_buffers.buffer_tx[1],
			.buffer_size = WORDS_COUNT,
		};
		__ASSERT_NO_MSG(nrfx_i2s_next_buffers_set(&m_inst, &next_buffers) == 0);

		fill_buffer(m_buffers.buffer_tx[1]);
	} else {
		check_buffer(p_released->p_rx_buffer);
		// The driver has just finished accessing the buffers pointed by
		// 'p_released'. They can be used for the next part of the transfer
		// that will be scheduled now.
		__ASSERT_NO_MSG(nrfx_i2s_next_buffers_set(&m_inst, p_released) == 0);
		++m_iteration_counter;

		// The pointer needs to be typecasted here, so that it is possible to
		// modify the content it is pointing to (it is marked in the structure
		// as pointing to constant data, because the driver is not supposed to
		// modify the provided data).
		__ASSERT(p_released->p_tx_buffer);
		fill_buffer((uint32_t *)p_released->p_tx_buffer);
	}
}

static void setup_clks_for_given_sample_rate(uint32_t frame_clk_freq,
					     tst_clk_setup_t *tst_clk_setup)
{
	if (frame_clk_freq == 16000) {
		tst_setup->mck_setup = NRF_I2S_MCK_32MDIV21;
        tst_setup->ratio = NRF_I2S_RATIO_96X;
	} else if (frame_clk_freq == 44100) {
		tst_setup->mck_setup = NRF_I2S_MCK_32MDIV23;
        tst_setup->ratio = NRF_I2S_RATIO_32X;
	} else if (frame_clk_freq == 48000) {
		tst_setup->mck_setup = NRF_I2S_MCK_32MDIV21;
        tst_setup->ratio = NRF_I2S_RATIO_32X;
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
	tst_config.skip_gpio_cfg = true;
	tst_config.skip_psel_cfg = true;

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
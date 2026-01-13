/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spim_mosi_toggles, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>
#include <gpiote_nrfx.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/linker/devicetree_regions.h>

/* Number of bytes transmitted in single transaction. */
#define TEST_DATA_SIZE 6

#define SPI_MODE (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB)

static struct spi_dt_spec spim_spec = SPI_DT_SPEC_GET(DT_NODELABEL(dut_spi_dt), SPI_MODE);

#define MEMORY_SECTION(node)                                                                       \
	COND_CODE_1(DT_NODE_HAS_PROP(node, memory_regions),                                        \
		    (__attribute__((__section__(                                                   \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(node, memory_regions)))))),      \
		    ())

static uint8_t spim_buffer[2 * TEST_DATA_SIZE] MEMORY_SECTION(DT_BUS(DT_NODELABEL(dut_spi_dt)));

/* Variables used to count edges on SPI MOSI line. */
#define CLOCK_INPUT_PIN	NRF_DT_GPIOS_TO_PSEL(DT_PATH(zephyr_user), test_gpios)

#if defined(NRF_TIMER00)
static nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(NRF_TIMER00);
#elif defined(NRF_TIMER130)
static nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(NRF_TIMER130);
#else
#error "No timer instance found"
#endif

static const uint32_t edges_exp = 6;

void timer_handler(nrf_timer_event_t event, void *p_context)
{
	ARG_UNUSED(event);
	ARG_UNUSED(p_context);
}

static void capture_callback(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	nrfx_timer_capture(&timer_instance, NRF_TIMER_CC_CHANNEL0);
}

K_TIMER_DEFINE(capture_timer, capture_callback, NULL);

int main(void)
{
	int ret;

	/* SPI buffer sets */
	struct spi_buf tx_spi_buf = {.buf = &spim_buffer[0], .len = TEST_DATA_SIZE};
	struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};

	struct spi_buf rx_spi_buf = {.buf = &spim_buffer[TEST_DATA_SIZE],
				     .len = TEST_DATA_SIZE};
	struct spi_buf_set rx_spi_buf_set = {.buffers = &rx_spi_buf, .count = 1};

	LOG_INF("SPIM MOSI toggling test on %s", CONFIG_BOARD_TARGET);
	LOG_INF("Testing SPIM device %s", spim_spec.bus->name);
	LOG_INF("%d bytes of data exchanged at once", TEST_DATA_SIZE);
	LOG_INF("Expect %d edges (both rising and falling) on MOSI line", edges_exp);
	LOG_INF("===================================================================");

	ret = spi_is_ready_dt(&spim_spec);
	if (!ret) {
		LOG_ERR("Error: SPI device is not ready");
		return -1;
	}

	/* Configure GPIOTE. */
	uint8_t gpiote_channel;
	nrfx_gpiote_t gpiote_instance =
		GPIOTE_NRFX_INST_BY_NODE(NRF_DT_GPIOTE_NODE(DT_PATH(zephyr_user), test_gpios));

	ret = nrfx_gpiote_channel_alloc(&gpiote_instance, &gpiote_channel);
	if (ret != 0) {
		LOG_ERR("nrfx_gpiote_channel_alloc(), err: %d", ret);
	}

	nrfx_gpiote_trigger_config_t trigger_cfg = {
		.p_in_channel = &gpiote_channel,
		.trigger = NRFX_GPIOTE_TRIGGER_TOGGLE,
	};

	nrf_gpio_pin_pull_t pull_cfg = NRFX_GPIOTE_DEFAULT_PULL_CONFIG;

	nrfx_gpiote_input_pin_config_t gpiote_cfg = {
		.p_pull_config = &pull_cfg,
		.p_trigger_config = &trigger_cfg,
	};

	ret = nrfx_gpiote_input_configure(&gpiote_instance, CLOCK_INPUT_PIN, &gpiote_cfg);
	if (ret != 0) {
		LOG_ERR("nrfx_gpiote_input_configure(), err: %d", ret);
	}

	nrfx_gpiote_trigger_enable(&gpiote_instance, CLOCK_INPUT_PIN, false);

	/* Configure Timer. */
	nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(
					   NRFX_TIMER_BASE_FREQUENCY_GET(&timer_instance));
	timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
	timer_config.mode      = NRF_TIMER_MODE_COUNTER;

	ret = nrfx_timer_init(&timer_instance, &timer_config, timer_handler);
	if (ret != 0) {
		LOG_ERR("nrfx_timer_init(), err: %d", ret);
	}

	nrfx_timer_enable(&timer_instance);

	/* Configure GPPI from GPIOTE to Timer. */
	nrfx_gppi_handle_t gppi_handle;
	uint32_t eep = nrfx_gpiote_in_event_address_get(&gpiote_instance, CLOCK_INPUT_PIN);
	uint32_t tep = nrfx_timer_task_address_get(&timer_instance, NRF_TIMER_TASK_COUNT);

	ret = nrfx_gppi_conn_alloc(eep, tep, &gppi_handle);
	if (ret != 0) {
		LOG_ERR("GPPI channel allocation failed, err: %d", ret);
	}
	nrfx_gppi_conn_enable(gppi_handle);

	/* Set tx_data for current test. Test scenario reqires:
	 * The first transmitted bit in the final byte is 1.
	 * The last trnasmitted bit in the final byte is 0.
	 * After changing the spim_buffer, align
	 * TEST_DATA_SIZE and edges_exp (count both edges).
	 */
	spim_buffer[0] = 0x0;
	spim_buffer[1] = 0xff;
	spim_buffer[2] = 0x0;
	spim_buffer[3] = 0xfe;
	spim_buffer[4] = 0x0;
	spim_buffer[5] = 0xfe;

	/* Enable the timer. */
	k_timer_start(&capture_timer, K_MSEC(1), K_NO_WAIT);

	/* Transmit data. */
	ret = spi_transceive_dt(&spim_spec, &tx_spi_buf_set, &rx_spi_buf_set);
	if (ret != 0) {
		LOG_ERR("spi_transceive_dt, err: %d", ret);
	}
	k_sleep(K_MSEC(200));

	uint32_t pulses = nrfx_timer_capture_get(&timer_instance, NRF_TIMER_CC_CHANNEL0);

	LOG_INF("MOSI edges: %u", pulses);

	if (pulses == edges_exp) {
		LOG_INF("Test passed");
	} else {
		LOG_INF("Test failed - found %u edges while %u are expected",
			pulses, edges_exp);
	}

	return 0;
}

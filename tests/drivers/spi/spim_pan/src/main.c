/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/spi.h>
#include <nrfx_spim.h>
#include <nrfx_timer.h>
#include <zephyr/linker/devicetree_regions.h>

#include <zephyr/drivers/counter.h>
#include <helpers/nrfx_gppi.h>

/* SPI MODE 0 */
#define SPI_MODE (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB)

#if DT_NODE_EXISTS(DT_NODELABEL(dut_spi_fast))
#define DUT_SPI_NODE	    DT_NODELABEL(dut_spi_fast)
#define DUT_SPI_FAST	    1
#define TEST_BUFFER_SIZE    4
#define MX25R64_RDID	    0x9F
#define MX25R64_MFG_ID	    0xC2
#define MX25R64_MEM_TYPE    0x28
#define MX25R64_MEM_DENSITY 0x17
#define MAX_READ_REPEATS    10
#else
#define DUT_SPI_NODE	 DT_NODELABEL(dut_spi)
#define TEST_BUFFER_SIZE 64
#endif

static struct spi_dt_spec spim_spec = SPI_DT_SPEC_GET(DT_NODELABEL(dut_spi_dt), SPI_MODE);
NRF_SPIM_Type *spim_reg = (NRF_SPIM_Type *)DT_REG_ADDR(DUT_SPI_NODE);

static nrfx_timer_t test_timer = NRFX_TIMER_INSTANCE(DT_REG_ADDR(DT_NODELABEL(tst_timer)));

#define MEMORY_SECTION(node)                                                                       \
	COND_CODE_1(DT_NODE_HAS_PROP(node, memory_regions),                                        \
		    (__attribute__((__section__(                                                   \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(node, memory_regions)))))),      \
		    ())

static uint8_t tx_buffer[TEST_BUFFER_SIZE] MEMORY_SECTION(DT_BUS(DT_NODELABEL(dut_spi_dt)));
static uint8_t rx_buffer[TEST_BUFFER_SIZE] MEMORY_SECTION(DT_BUS(DT_NODELABEL(dut_spi_dt)));

static void *test_setup(void)
{
	zassert_true(spi_is_ready_dt(&spim_spec), "SPIM device is not ready");

	return NULL;
}

static void set_buffers(void)
{
	memset(tx_buffer, 0x8B, TEST_BUFFER_SIZE);
	memset(rx_buffer, 0xFF, TEST_BUFFER_SIZE);
}

static uint32_t configure_test_timer(nrfx_timer_t *timer)
{
	uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer->p_reg);
	nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);

	timer_config.bit_width = NRF_TIMER_BIT_WIDTH_16;
	timer_config.mode = NRF_TIMER_MODE_COUNTER;

	TC_PRINT("Timer base frequency: %d Hz\n", base_frequency);

	zassert_ok(nrfx_timer_init(timer, &timer_config, NULL), "Timer init failed\n");
	nrfx_timer_enable(timer);

	return nrfx_timer_task_address_get(timer, NRF_TIMER_TASK_COUNT);
}

/*
 * Reference: MLTPAN-8
 * Requirements to trigger the PAN workaround
 * CPHA = 0 (configured in SPI_MODE)
 * PRESCALER > 2 (4 for 4MHz, 16 for 1MHz)
 * First transmitted bit is 1 (0x8B, MSB)
 */

ZTEST(spim_pan, test_spim_mltpan_8_workaround)
{
	Z_TEST_SKIP_IFDEF(DUT_SPI_FAST);

	int err;

	struct spi_buf tx_spi_buf = {.buf = tx_buffer, .len = TEST_BUFFER_SIZE};
	struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};

	struct spi_buf rx_spi_buf = {.buf = rx_buffer, .len = TEST_BUFFER_SIZE};
	struct spi_buf_set rx_spi_buf_set = {.buffers = &rx_spi_buf, .count = 1};

	set_buffers();

	err = spi_transceive_dt(&spim_spec, &tx_spi_buf_set, &rx_spi_buf_set);
	TC_PRINT("SPIM prescaler: %u\n", spim_reg->PRESCALER);
	zassert_true(spim_reg->PRESCALER > 2, "SPIM prescaler is not greater than 2\n");
	zassert_ok(err, "SPI transceive failed: %d\n", err);

	zassert_mem_equal(tx_buffer, rx_buffer, TEST_BUFFER_SIZE, "TX buffer != RX buffer\n");
}

/*
 * Reference: MLTPAN-55
 * Requirements to trigger the PAN workaround
 * MODE 0 or MODE 2 (CPHA = 0)
 * PRESCALER = 4 (4 MHz) for SPIM2x and SPIM3x
 * RX.MAXCNT lower than TX.MAXCNT
 */

ZTEST(spim_pan, test_spim_mltpan_55_workaround)
{
	Z_TEST_SKIP_IFDEF(DUT_SPI_FAST);

	int err;

	nrfx_gppi_handle_t gppi_handle;

	uint32_t timer_cc_before, timer_cc_after;

	uint32_t timer_task;
	uint32_t spim_event;

	struct spi_buf tx_spi_buf = {.buf = tx_buffer, .len = TEST_BUFFER_SIZE};
	struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};

	struct spi_buf rx_spi_buf = {.buf = rx_buffer, .len = TEST_BUFFER_SIZE - 1};
	struct spi_buf_set rx_spi_buf_set = {.buffers = &rx_spi_buf, .count = 1};

	set_buffers();

	timer_task = configure_test_timer(&test_timer);
	spim_event = nrf_spim_event_address_get(spim_reg, NRF_SPIM_EVENT_END);

	zassert_ok(nrfx_gppi_conn_alloc(spim_event, timer_task, &gppi_handle),
		   "Failed to allocate DPPI connection\n");
	nrfx_gppi_conn_enable(gppi_handle);

	timer_cc_before = nrfx_timer_capture(&test_timer, NRF_TIMER_CC_CHANNEL0);
	err = spi_transceive_dt(&spim_spec, &tx_spi_buf_set, &rx_spi_buf_set);
	zassert_ok(err, "SPI transceive failed: %d\n", err);
	timer_cc_after = nrfx_timer_capture(&test_timer, NRF_TIMER_CC_CHANNEL0);

	nrfx_gppi_conn_disable(gppi_handle);
	nrfx_timer_uninit(&test_timer);

	zassert_true((timer_cc_after - timer_cc_before) > 0,
		     "NRF_SPIM_EVENT_END did not trigger\n");
	zassert_mem_equal(tx_buffer, rx_buffer, TEST_BUFFER_SIZE - 1, "TX buffer != RX buffer\n");
}

/*
 * Reference: MLTPAN-57
 * SPIM00 does not operate as expected
 * SPIM00 is the fast SPIM instance
 * Requires workaround
 */

ZTEST(spim_pan, test_spim_mltpan_57_workaround)
{
	Z_TEST_SKIP_IFDEF(CONFIG_COVERAGE);
	Z_TEST_SKIP_IFNDEF(DUT_SPI_FAST);
#if defined(DUT_SPI_FAST)
	int err;

	nrfx_gppi_handle_t gppi_handle;

	uint32_t timer_cc_before, timer_cc_after, tx_amount;

	uint32_t timer_task;
	uint32_t spim_event;

	memset(tx_buffer, 0xFF, TEST_BUFFER_SIZE);
	tx_buffer[0] = MX25R64_RDID;

	timer_task = configure_test_timer(&test_timer);
	spim_event = nrf_spim_event_address_get(spim_reg, NRF_SPIM_EVENT_END);

	zassert_ok(nrfx_gppi_conn_alloc(spim_event, timer_task, &gppi_handle),
		   "Failed to allocate DPPI connection\n");
	nrfx_gppi_conn_enable(gppi_handle);

	struct spi_buf tx_spi_buf = {.buf = tx_buffer, .len = TEST_BUFFER_SIZE / 2};
	struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};

	struct spi_buf rx_spi_buf = {.buf = rx_buffer, .len = TEST_BUFFER_SIZE / 2 + 1};
	struct spi_buf_set rx_spi_buf_set = {.buffers = &rx_spi_buf, .count = 1};

	for (int i = 0; i < MAX_READ_REPEATS; i++) {
		TC_PRINT("RDID attempt %u\n", i + 1);

		timer_cc_before = nrfx_timer_capture(&test_timer, NRF_TIMER_CC_CHANNEL0);
		err = spi_transceive_dt(&spim_spec, &tx_spi_buf_set, &rx_spi_buf_set);
		timer_cc_after = nrfx_timer_capture(&test_timer, NRF_TIMER_CC_CHANNEL0);
		TC_PRINT("SPIM prescaler: %u\n", spim_reg->PRESCALER);
		zassert_ok(err, "SPI transceive failed: %d\n", err);

		tx_amount = nrf_spim_tx_amount_get(spim_reg);
		TC_PRINT("NRF_SPIM_EVENT_END events count: %u\n", timer_cc_after - timer_cc_before);
		TC_PRINT("TX.AMOUNT: %u\n", tx_amount);

		zassert_true((timer_cc_after - timer_cc_before > 0),
			     "NRF_SPIM_EVENT_END event has not been generated\n");
		zassert_equal(tx_amount, ARRAY_SIZE(tx_buffer), "TX.AMOUNT != TX Buffer size\n");

		for (int i = 0; i < ARRAY_SIZE(rx_buffer); i++) {
			TC_PRINT("rx_buffer[%d] = 0x%x\n", i, rx_buffer[i]);
		}

		/* First RX byte is dummy */
		zassert_equal(rx_buffer[1], MX25R64_MFG_ID,
			      "Read MX25R64 device ID is different than expected\n");
		zassert_equal(rx_buffer[2], MX25R64_MEM_TYPE,
			      "Read MX25R64 memory type is different than expected\n");
		zassert_equal(rx_buffer[3], MX25R64_MEM_DENSITY,
			      "Read MX25R64 memory density is different than expected\n");
	}

	nrfx_timer_uninit(&test_timer);
	nrfx_gppi_conn_disable(gppi_handle);
#endif
}

/*
 * Reference: MLTPAN-69
 * Requirements to trigger the PAN workaround
 * SPIM is configured using IFTIMING.RXDELAY > 0
 * STOP task is triggered after
 * the TX data have been transferred
 * but before the final RX byte has been received
 */
ZTEST(spim_pan, test_spim_mltpan_69_workaround)
{
	Z_TEST_SKIP_IFDEF(DUT_SPI_FAST);

	int err;

	nrfx_gppi_handle_t gppi_handle;

	uint32_t timer_cc_before, timer_cc_after;

	uint32_t timer_task;
	uint32_t spim_event;

	struct spi_buf tx_spi_buf = {.buf = tx_buffer, .len = TEST_BUFFER_SIZE};
	struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};

	struct spi_buf rx_spi_buf = {.buf = rx_buffer, .len = TEST_BUFFER_SIZE};
	struct spi_buf_set rx_spi_buf_set = {.buffers = &rx_spi_buf, .count = 1};

	set_buffers();

	timer_task = configure_test_timer(&test_timer);
	spim_event = nrf_spim_event_address_get(spim_reg, NRF_SPIM_EVENT_STOPPED);

	zassert_ok(nrfx_gppi_conn_alloc(spim_event, timer_task, &gppi_handle),
		   "Failed to allocate DPPI connection\n");
	nrfx_gppi_conn_enable(gppi_handle);

	timer_cc_before = nrfx_timer_capture(&test_timer, NRF_TIMER_CC_CHANNEL0);
	err = spi_transceive_dt(&spim_spec, &tx_spi_buf_set, &rx_spi_buf_set);
	timer_cc_after = nrfx_timer_capture(&test_timer, NRF_TIMER_CC_CHANNEL0);

	TC_PRINT("SPI IFTIMING.RXDELAY=%u\n", spim_reg->IFTIMING.RXDELAY);
	zassert_true(spim_reg->IFTIMING.RXDELAY > 0,
		     "SPIM  IFTIMING.RXDELAY is not greater than 0\n");
	zassert_ok(err, "SPI transceive failed: %d\n", err);

	nrfx_timer_uninit(&test_timer);
	nrfx_gppi_conn_disable(gppi_handle);

	TC_PRINT("NRF_SPIM_EVENT_STOPPED events count: %u\n", timer_cc_after - timer_cc_before);
	zassert_true((timer_cc_after - timer_cc_before) > 0,
		     "NRF_SPIM_EVENT_STOPPED did not trigger\n");
	zassert_mem_equal(tx_buffer, rx_buffer, TEST_BUFFER_SIZE, "TX buffer != RX buffer\n");
}

ZTEST_SUITE(spim_pan, NULL, test_setup, NULL, NULL, NULL);

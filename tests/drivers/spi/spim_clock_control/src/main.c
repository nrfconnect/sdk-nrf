/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/ztest.h>

/* SPI MODE 0 */
#define SPI_MODE (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB)

#define DUT_SPI_NODE		     DT_NODELABEL(dut_spi)
#define TEST_BUFFER_SIZE	     512
#define REQUEST_SERVING_WAIT_TIME_US 10000

static struct spi_dt_spec spim_spec = SPI_DT_SPEC_GET(DT_NODELABEL(dut_spi_dt), SPI_MODE);
const struct device *const fll16m_dev = DEVICE_DT_GET(DT_NODELABEL(fll16m));

#define MEMORY_SECTION(node)                                                                       \
	COND_CODE_1(DT_NODE_HAS_PROP(node, memory_regions),                                        \
		    (__attribute__((__section__(                                                   \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(node, memory_regions)))))),      \
		    ())

static uint8_t tx_buffer[TEST_BUFFER_SIZE] MEMORY_SECTION(DT_BUS(DT_NODELABEL(dut_spi_dt)));
static uint8_t rx_buffer[TEST_BUFFER_SIZE] MEMORY_SECTION(DT_BUS(DT_NODELABEL(dut_spi_dt)));

const struct nrf_clock_spec fll16m_open_loop_mode = {
	.frequency = MHZ(16),
	.accuracy = NRF_CLOCK_CONTROL_ACCURACY_PPM(20000),
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

const struct nrf_clock_spec fll16m_bypass_mode = {
	.frequency = MHZ(16),
	.accuracy = NRF_CLOCK_CONTROL_ACCURACY_PPM(30),
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

static void request_clock_spec(const struct device *clk_dev, const struct nrf_clock_spec *clk_spec)
{
	int ret = 0;
	int res = 0;
	struct onoff_client cli;
	uint32_t rate;

	TC_PRINT("Clock: %s, requesting frequency=%d, accuracy=%d, precision=%d\n", clk_dev->name,
		 clk_spec->frequency, clk_spec->accuracy, clk_spec->precision);
	sys_notify_init_spinwait(&cli.notify);
	ret = nrf_clock_control_request(clk_dev, clk_spec, &cli);
	zassert_true((ret >= 0 && ret <= 2),
		     "Clock control request response outside of range, ret = %d\n", ret);
	do {
		ret = sys_notify_fetch_result(&cli.notify, &res);
		k_yield();
	} while (ret == -EAGAIN);
	zassert_ok(ret, "ret != 0, ret = %d\n", ret);
	zassert_ok(res, "ret != 0, ret = %d\n", res);
	ret = clock_control_get_rate(clk_dev, NULL, &rate);
	if (ret != -ENOSYS) {
		zassert_ok(ret, "ret != 0, ret = %d\n", ret);
		zassert_equal(rate, clk_spec->frequency,
			      "Clock frequency != requested frequnecy\n");
		k_busy_wait(REQUEST_SERVING_WAIT_TIME_US);
	}
}

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

static void test_spim_with_clock_control(void)
{
	int err;

	struct spi_buf tx_spi_buf = {.buf = tx_buffer, .len = TEST_BUFFER_SIZE};
	struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};

	struct spi_buf rx_spi_buf = {.buf = rx_buffer, .len = TEST_BUFFER_SIZE};
	struct spi_buf_set rx_spi_buf_set = {.buffers = &rx_spi_buf, .count = 1};

	set_buffers();

	err = spi_transceive_dt(&spim_spec, &tx_spi_buf_set, &rx_spi_buf_set);
	zassert_ok(err, "SPI transceive failed: %d\n", err);

	zassert_mem_equal(tx_buffer, rx_buffer, TEST_BUFFER_SIZE, "TX buffer != RX buffer\n");
}

ZTEST(spim_clock_control, test_spim_with_clock_control_active_fll16m_open_loop)
{
	request_clock_spec(fll16m_dev, &fll16m_open_loop_mode);
	test_spim_with_clock_control();
}

ZTEST(spim_clock_control, test_spim_with_clock_control_active_fll16m_bypass)
{
	request_clock_spec(fll16m_dev, &fll16m_bypass_mode);
	test_spim_with_clock_control();
}

ZTEST_SUITE(spim_clock_control, NULL, test_setup, NULL, NULL, NULL);

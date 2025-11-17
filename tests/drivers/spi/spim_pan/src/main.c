/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/spi.h>
#include <nrfx_spim.h>
#include <zephyr/linker/devicetree_regions.h>

#define SPI_MODE (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB)

#define TEST_BUFFER_SIZE 8

static struct spi_dt_spec spim_spec = SPI_DT_SPEC_GET(DT_NODELABEL(dut_spi_dt), SPI_MODE, 0);
NRF_SPIM_Type *spim_reg = (NRF_SPIM_Type *)DT_REG_ADDR(DT_NODELABEL(dut_spi));

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

/*
 * Reference: MLTPAN-8
 * Requirements to trigger the PAN workaround
 * CPHA = 0 (configured in SPI_MODE)
 * PRESCALER > 2 (16 for 1MHz)
 * First transmitted bit is 1 (0x8B, MSB)
 */

ZTEST(spim_pan, test_spim_pan_workaround)
{
	int err;

	struct spi_buf tx_spi_buf = {.buf = tx_buffer, .len = TEST_BUFFER_SIZE};
	struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};

	struct spi_buf rx_spi_buf = {.buf = rx_buffer, .len = TEST_BUFFER_SIZE};
	struct spi_buf_set rx_spi_buf_set = {.buffers = &rx_spi_buf, .count = 1};

	set_buffers();

	TC_PRINT("SPIM prescaler: %u\n", spim_reg->PRESCALER);
	zassert_true(spim_reg->PRESCALER > 2, "SPIM prescaler is not greater than 2\n");

	err = spi_transceive_dt(&spim_spec, &tx_spi_buf_set, &rx_spi_buf_set);
	zassert_ok(err, "SPI transceive failed: %d\n", err);

	zassert_mem_equal(tx_buffer, rx_buffer, TEST_BUFFER_SIZE, "TX buffer != RX buffer\n");
}

ZTEST_SUITE(spim_pan, NULL, test_setup, NULL, NULL, NULL);

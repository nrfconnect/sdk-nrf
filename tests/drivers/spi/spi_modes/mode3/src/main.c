/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spi_modes_common.h"

#define SPI_MODE (SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE)
#define SPIM_OP	 (SPI_OP_MODE_MASTER | SPI_MODE)
#define SPIS_OP	 (SPI_OP_MODE_SLAVE | SPI_MODE)

struct spi_dt_spec spim = SPI_DT_SPEC_GET(DT_NODELABEL(dut_spi_dt), SPIM_OP, 0);

const struct spi_config spis_config = {
	.operation = SPIS_OP
};

ZTEST(spi_mode3, test_basic)
{
	test_basic(false);
}

ZTEST(spi_mode3, test_basic_async)
{
	test_basic(true);
}

ZTEST(spi_mode3, test_basic_zero_len)
{
	test_basic_zero_len(false);
}

ZTEST(spi_mode3, test_basic_zero_len_async)
{
	test_basic_zero_len(true);
}

ZTEST(spi_mode3, test_short_rx)
{
	test_short_rx(false);
}

ZTEST(spi_mode3, test_short_rx_async)
{
	test_short_rx(true);
}

ZTEST(spi_mode3, test_only_tx)
{
	test_only_tx(false);
}

ZTEST(spi_mode3, test_only_tx_async)
{
	test_only_tx(true);
}

ZTEST(spi_mode3, test_only_tx_in_chunks)
{
	test_only_tx_in_chunks(false);
}

ZTEST(spi_mode3, test_only_tx_in_chunks_async)
{
	test_only_tx_in_chunks(true);
}

ZTEST(spi_mode3, test_only_rx)
{
	test_only_rx(false);
}

ZTEST(spi_mode3, test_only_rx_async)
{
	test_only_rx(true);
}

ZTEST(spi_mode3, test_only_rx_in_chunks)
{
	test_only_rx_in_chunks(false);
}

ZTEST(spi_mode3, test_only_rx_in_chunks_async)
{
	test_only_rx_in_chunks(true);
}

ZTEST_SUITE(spi_mode3, NULL, suite_setup, before, NULL, NULL);

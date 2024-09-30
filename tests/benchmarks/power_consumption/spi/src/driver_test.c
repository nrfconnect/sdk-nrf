/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#define SPIOP	SPI_WORD_SET(8) | SPI_TRANSFER_MSB

void thread_definition(void)
{
	int ret;
	struct spi_dt_spec spispec = SPI_DT_SPEC_GET(DT_ALIAS(accel0), SPIOP, 0);

	uint8_t tx_buffer = 0x15;
	uint8_t data[3] = {1, 2, 3};
	struct spi_buf tx_spi_buf		= {.buf = (void *)&tx_buffer, .len = 1};
	struct spi_buf_set tx_spi_buf_set	= {.buffers = &tx_spi_buf, .count = 1};
	struct spi_buf rx_spi_bufs		= {.buf = data, .len = 3};
	struct spi_buf_set rx_spi_buf_set	= {.buffers = &rx_spi_bufs, .count = 1};

	while (1) {
		ret = spi_transceive_dt(&spispec, &tx_spi_buf_set, &rx_spi_buf_set);
		if (ret < 0) {
			printk("Error during SPI transfer");
			return;
		}
	};
};

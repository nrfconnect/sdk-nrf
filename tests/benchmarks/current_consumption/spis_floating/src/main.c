/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>

#define BUF_SIZE 32
static const struct device *spis_dev = DEVICE_DT_GET(DT_ALIAS(spis));
static const struct spi_config spis_config = {.operation = SPI_OP_MODE_SLAVE | SPI_WORD_SET(8)};

int main(void)
{
	bool status;
	static char rx_buffer[BUF_SIZE];
	struct spi_buf rx_spi_bufs = {.buf = rx_buffer, .len = sizeof(rx_buffer)};
	struct spi_buf_set rx_spi_buf_set = {.buffers = &rx_spi_bufs, .count = 1};

	status = device_is_ready(spis_dev);
	__ASSERT(status, "Error: SPI device is not ready");

	/* Arbitrary delay for CI purposes.
	 * Catch moment when device enters low power state.
	 */
	k_busy_wait(2000000);

	memset(rx_buffer, 0x00, sizeof(rx_buffer));

	/* No Controller device on the SPI bus -> spi_read() will never complete. */
	spi_read(spis_dev, &spis_config, &rx_spi_buf_set);
	k_msleep(2000);

	__ASSERT(false, "Code execution shall not get here.");

	return 0;
}

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>

LOG_MODULE_REGISTER(idle_spim);

#define SPI_MODE		 SPI_WORD_SET(8) | SPI_TRANSFER_MSB
#define SPI_READ_MASK		 0x80
#define CHIP_ID_REGISTER_ADDRESS 0x00
#define SPI_READ_COUNT		 50

static struct spi_dt_spec spim_spec =
	SPI_DT_SPEC_GET(DT_NODELABEL(bmi270), SPI_OP_MODE_MASTER | SPI_MODE, 0);

int spi_read_register(uint8_t register_address, uint8_t *register_value)
{
	int err;
	uint8_t tx_buffer[3] = {register_address | SPI_READ_MASK, 0xFF, 0xFF};
	uint8_t rx_buffer[3];

	struct spi_buf tx_spi_buf = {.buf = tx_buffer, .len = sizeof(tx_buffer)};
	struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};
	struct spi_buf rx_spi_bufs = {.buf = rx_buffer, .len = sizeof(rx_buffer)};
	struct spi_buf_set rx_spi_buf_set = {.buffers = &rx_spi_bufs, .count = 1};

	err = spi_transceive_dt(&spim_spec, &tx_spi_buf_set, &rx_spi_buf_set);
	*register_value = rx_buffer[2];

	printk("'spi_transceive_dt', err: %d, rx_data: %x %x %x\n", err, rx_buffer[0], rx_buffer[1],
	       rx_buffer[2]);
	return err;
}

int main(void)
{
	int err;
	uint8_t response;

	err = spi_is_ready_dt(&spim_spec);
	if (!err) {
		printk("Error: SPI device is not ready, err: %d\n", err);
		return -1;
	}

	while (1) {
		for (int read_index = 0; read_index < SPI_READ_COUNT; read_index++) {
			err = spi_read_register(CHIP_ID_REGISTER_ADDRESS, &response);
			if (!err) {
				printk("SPI read returned error: %d\n", err);
				return -1;
			}
			printk("Chip ID: 0x%x\n", response);
		}
		k_msleep(2000);
	}

	return 0;
}

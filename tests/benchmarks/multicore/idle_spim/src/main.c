/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

LOG_MODULE_REGISTER(idle_spim);

#define SPI_MODE		 SPI_WORD_SET(8) | SPI_TRANSFER_MSB
#define SPI_READ_MASK		 0x80
#define CHIP_ID_REGISTER_ADDRESS 0x00
#define SPI_READ_COUNT		 50

static struct spi_dt_spec spim_spec =
	SPI_DT_SPEC_GET(DT_NODELABEL(bmi270), SPI_OP_MODE_MASTER | SPI_MODE);

void spi_read_register(uint8_t register_address, uint8_t *register_value)
{
	int rc;
	uint8_t tx_buffer[3] = {register_address | SPI_READ_MASK, 0xFF, 0xFF};
	uint8_t rx_buffer[3];

	struct spi_buf tx_spi_buf = {.buf = tx_buffer, .len = sizeof(tx_buffer)};
	struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};
	struct spi_buf rx_spi_bufs = {.buf = rx_buffer, .len = sizeof(rx_buffer)};
	struct spi_buf_set rx_spi_buf_set = {.buffers = &rx_spi_bufs, .count = 1};

	rc = spi_transceive_dt(&spim_spec, &tx_spi_buf_set, &rx_spi_buf_set);
	__ASSERT(rc == 0, "Error: spi_transceive_dt, err: %d\n", rc);
	*register_value = rx_buffer[2];

	__ASSERT(rx_buffer[2] == 0x24, "Invalid CHIP ID %x (0x24)", rx_buffer[2]);

	printk("'spi_transceive_dt', rx_data: %x %x %x\n", rx_buffer[0], rx_buffer[1],
	       rx_buffer[2]);
}

int main(void)
{
	bool status;
	uint8_t response;
	int test_repetitions = 3;

	status = gpio_is_ready_dt(&led);
	__ASSERT(status, "Error: GPIO Device not ready");

	status = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	__ASSERT(status == 0, "Could not configure led GPIO");

	status = spi_is_ready_dt(&spim_spec);
	__ASSERT(status, "Error: SPI device is not ready");

#if defined(CONFIG_COVERAGE)
	printk("Coverage analysis enabled\n");
	while (test_repetitions--)
#else
	while (test_repetitions)
#endif
	{
		for (int read_index = 0; read_index < SPI_READ_COUNT; read_index++) {
			spi_read_register(CHIP_ID_REGISTER_ADDRESS, &response);
			printk("Chip ID: 0x%x\n", response);
		}
		gpio_pin_set_dt(&led, 0);
		k_msleep(1000);
		gpio_pin_set_dt(&led, 1);
	}

#if defined(CONFIG_COVERAGE)
	printk("Coverage analysis start\n");
#endif
	return 0;
}

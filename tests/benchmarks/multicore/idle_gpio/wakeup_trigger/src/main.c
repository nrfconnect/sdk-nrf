/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

#define SPI_MODE (SPI_OP_MODE_MASTER | SPI_WORD_SET(8))
static const struct spi_dt_spec spim = SPI_DT_SPEC_GET(DT_NODELABEL(spim_dt), SPI_MODE);

LOG_MODULE_REGISTER(wakeup_trigger);

int main(void)
{
	static char tx_buffer[] = {0x5};
	struct spi_buf tx_spi_bufs = {.buf = tx_buffer, .len = sizeof(tx_buffer)};
	struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_bufs, .count = 1};
	int rc;

	rc = gpio_is_ready_dt(&led);
	if (rc < 0) {
		LOG_ERR("GPIO Device not ready (%d)", rc);
		return 0;
	}

	rc = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (rc < 0) {
		LOG_ERR("Could not configure led GPIO (%d)", rc);
		return 0;
	}

	rc = spi_is_ready_dt(&spim);
	if (rc < 0) {
		LOG_ERR("SPI device is not ready (%d)", rc);
		return 0;
	}

	while (1) {
		LOG_INF("SPIM: going to sleep for 1.5s...");
		gpio_pin_set_dt(&led, 0);
		k_msleep(1500);
		gpio_pin_set_dt(&led, 1);
		LOG_INF("SPIM: will be active for 500ms before transfer");
		k_busy_wait(500000);
		LOG_INF("SPIM: transferring one byte: '%#02x'", tx_buffer[0]);
		spi_write_dt(&spim, &tx_spi_buf_set);
	}

	return 0;
}

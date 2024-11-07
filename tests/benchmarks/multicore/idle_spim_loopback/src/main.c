/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(idle_spim_loopback, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/pm/device_runtime.h>

#define	DELTA			(1)

#define SPI_MODE (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB \
				| SPI_MODE_CPHA | SPI_MODE_CPOL)

static struct spi_dt_spec spim_spec = SPI_DT_SPEC_GET(DT_NODELABEL(dut_spi_dt), SPI_MODE, 0);

#define MEMORY_SECTION(node)                                                           \
	COND_CODE_1(DT_NODE_HAS_PROP(node, memory_regions),                                \
		    (__attribute__((__section__(                                               \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(node, memory_regions)))))),      \
		    ())

static uint8_t spim_buffer[2 * CONFIG_DATA_FIELD]
	MEMORY_SECTION(DT_BUS(DT_NODELABEL(dut_spi_dt)));

/* Variables used to make SPI active for ~1 second */
static struct k_timer my_timer;
static bool timer_expired;

void my_timer_handler(struct k_timer *dummy)
{
	timer_expired = true;
}


int main(void)
{
	int ret;
	int counter = 0;
	uint8_t acc = 0;
	bool test_pass;

	/* SPI buffer sets */
	struct spi_buf tx_spi_buf = {
		.buf = &spim_buffer[0],
		.len = CONFIG_DATA_FIELD
	};
	struct spi_buf_set tx_spi_buf_set = {
		.buffers = &tx_spi_buf,
		.count = 1
	};

	struct spi_buf rx_spi_buf = {
		.buf = &spim_buffer[CONFIG_DATA_FIELD],
		.len = CONFIG_DATA_FIELD
	};
	struct spi_buf_set rx_spi_buf_set = {
		.buffers = &rx_spi_buf,
		.count = 1
	};

	LOG_INF("%s runs as a SPI HOST", CONFIG_BOARD_TARGET);
	LOG_INF("%d bytes of data exchanged at once", CONFIG_DATA_FIELD);

	ret = spi_is_ready_dt(&spim_spec);
	if (!ret) {
		LOG_ERR("Error: SPI device is not ready");
	}
	__ASSERT(ret, "Error: SPI device is not ready\n");

	k_timer_init(&my_timer, my_timer_handler, NULL);

	/* Run test forever */
	while (1) {
		test_pass = true;
		timer_expired = false;

		/* start a one-shot timer that expires after 1 second */
		k_timer_start(&my_timer, K_MSEC(1000), K_NO_WAIT);

		/* SPI active transmissions for ~ 1 second */
		while (!timer_expired) {
			/* Generate pseudo random tx_data for current test */
			for (int i = 0; i < CONFIG_DATA_FIELD; i++) {
				*((uint8_t *) tx_spi_buf.buf + i) = acc;
				acc += DELTA;
			}

			/* Transmit data */
			ret = spi_transceive_dt(&spim_spec, &tx_spi_buf_set, &rx_spi_buf_set);
			if (ret != 0) {
				LOG_ERR("spi_transceive_dt, err: %d", ret);
			}
			__ASSERT(ret == 0, "Error: spi_transceive_dt, err: %d\n", ret);

			/* Check if the received data is consistent with the data sent */
			for (int i = 0; i < CONFIG_DATA_FIELD; i++) {
				uint8_t received = *((uint8_t *) rx_spi_buf.buf + i);
				uint8_t transmitted = *((uint8_t *) tx_spi_buf.buf + i);

				if (received != transmitted) {
					LOG_ERR("FAIL: rx[%d] = %d, expected %d",
						i, received, transmitted);
					test_pass = false;
					__ASSERT(false, "Run %d - FAILED\n", counter);
				}
			}
		} /* while (!timer_expired) */

		/* Report if communication was successful */
		if (test_pass) {
			LOG_INF("Run %d - PASS", counter);
		} else {
			LOG_INF("Run %d - FAILED", counter);
		}
		counter++;

		/* Sleep / enter low power state */
		k_msleep(CONFIG_TEST_SLEEP_DURATION_MS);
	}

	return 0;
}

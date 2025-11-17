/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include "nrfx_spim.h"

#define READ_COMMAND (0x0B)
#define DEVID_AD_ADDRESS (0x0)
#define DEVID_AD_VAL (0xAD)
#define DEVID_MST_VAL (0x1D)
#define PARTID_VAL (0xF2)

#define SPIOP	SPI_WORD_SET(8) | SPI_TRANSFER_MSB

/*
 * Additonaly to the standard test procedure, PAN woraround is verified
 *
 * Required condtiton to verify MLTPAN-8 workaround:
 * SPIM is configured with CPHA to 0,
 * PRESCALER is larger than 2
 * and the first transmitted bit is 1.
 *
 * Test SPIM confuration:
 * COPOL = 0
 * CPHA = 0 (OK)
 * With 1MHz overlay PRESCALER = 10 (OK)
 * First transmitted bit is 1 (0xB = 1011, OK)
 */

static bool suspend_req;

bool self_suspend_req(void)
{
	suspend_req = true;
	return true;
}

void thread_definition(void)
{
	int ret;
	struct spi_dt_spec spispec = SPI_DT_SPEC_GET(DT_ALIAS(accel0), SPIOP, 0);
	NRF_SPIM_Type *spi_reg = (NRF_SPIM_Type *)DT_REG_ADDR(DT_NODELABEL(dut_spi));

	uint8_t tx_buffer[5] = {READ_COMMAND, DEVID_AD_ADDRESS, 0xFF, 0xFF, 0xFF};
	uint8_t rx_buffer[5] = {0, 0, 0, 0, 0};
	struct spi_buf tx_spi_buf		= {.buf = tx_buffer, .len = 5};
	struct spi_buf_set tx_spi_buf_set	= {.buffers = &tx_spi_buf, .count = 1};
	struct spi_buf rx_spi_bufs		= {.buf = rx_buffer, .len = 5};
	struct spi_buf_set rx_spi_buf_set	= {.buffers = &rx_spi_bufs, .count = 1};


	while (1) {
		if (suspend_req) {
			printk("SPI PRESCALER %x\n", spi_reg->PRESCALER);
			suspend_req = false;
			k_thread_suspend(k_current_get());
		}

		ret = spi_transceive_dt(&spispec, &tx_spi_buf_set, &rx_spi_buf_set);
		__ASSERT(ret == 0, "spi_transceive_dt() returned %d", ret);

		/* Check if communication with adxl362 was successful. */
		__ASSERT(rx_buffer[2] == DEVID_AD_VAL,
			"Invalid DEVID_AD, got %u, expected %u\n", rx_buffer[2], DEVID_AD_VAL);
		__ASSERT(rx_buffer[3] == DEVID_MST_VAL,
			"Invalid DEVID_MST, got %u, expected %u\n", rx_buffer[3], DEVID_MST_VAL);
		__ASSERT(rx_buffer[4] == PARTID_VAL,
			"Invalid PARTID, got %u, expected %u\n", rx_buffer[4], PARTID_VAL);

		rx_buffer[2] = 0;
		rx_buffer[3] = 0;
		rx_buffer[4] = 0;
	};
};

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/linker/devicetree_regions.h>


#if CONFIG_TESTED_SPI_MODE == 0
#define SPI_MODE (SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_LSB)
#elif CONFIG_TESTED_SPI_MODE == 1
#define SPI_MODE (SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB | SPI_MODE_CPHA)
#elif CONFIG_TESTED_SPI_MODE == 2
#define SPI_MODE (SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_LSB | SPI_MODE_CPOL)
#elif CONFIG_TESTED_SPI_MODE == 3
#define SPI_MODE (SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB | SPI_MODE_CPHA \
				| SPI_MODE_CPOL)
#endif

#define SPIM_OP	 (SPI_OP_MODE_MASTER | SPI_MODE)
#define SPIS_OP	 (SPI_OP_MODE_SLAVE | SPI_MODE)

/* Test configuration: */
#define DATA_FIELD_LEN	(16)
#define	DELTA			(1)

#if CONFIG_ROLE_HOST == 1
static struct spi_dt_spec spim = SPI_DT_SPEC_GET(DT_NODELABEL(dut_spi_dt), SPIM_OP);
#else
static const struct device *spis_dev = DEVICE_DT_GET(DT_NODELABEL(dut_spis));
static const struct spi_config spis_config = {
	.operation = SPIS_OP
};
#endif

#define MEMORY_SECTION(node)                                                           \
	COND_CODE_1(DT_NODE_HAS_PROP(node, memory_regions),                                \
		    (__attribute__((__section__(                                               \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(node, memory_regions)))))),      \
		    ())

static uint8_t spim_buffer[32] MEMORY_SECTION(DT_BUS(DT_NODELABEL(dut_spi_dt)));
static uint8_t spis_buffer[32] MEMORY_SECTION(DT_NODELABEL(dut_spis));

struct test_data {
	int spim_alloc_idx;
	int spis_alloc_idx;
	struct spi_buf_set sets[4];
	struct spi_buf_set *mtx_set;
	struct spi_buf_set *mrx_set;
	struct spi_buf_set *stx_set;
	struct spi_buf_set *srx_set;
	struct spi_buf bufs[4];
};

static struct test_data tdata;

/* Allocate buffer from spim or spis space. */
static uint8_t *buf_alloc(size_t len, bool spim)
{
	int *idx = spim ? &tdata.spim_alloc_idx : &tdata.spis_alloc_idx;
	uint8_t *buf = spim ? spim_buffer : spis_buffer;
	size_t total = spim ? sizeof(spim_buffer) : sizeof(spis_buffer);
	uint8_t *rv;

	if (*idx + len > total) {
		return NULL;
	}

	rv = &buf[*idx];
	*idx += len;

	return rv;
}

void allocate_buffers(void)
{
	/* DEVICE sends 0's to the HOST in the first transaction after power on */
	memset(&tdata, 0, sizeof(tdata));

	for (int i = 0; i < 4; i++) {
		tdata.bufs[i].buf = buf_alloc(DATA_FIELD_LEN, i < 2);
		tdata.bufs[i].len = DATA_FIELD_LEN;
		tdata.sets[i].buffers = &tdata.bufs[i];
		tdata.sets[i].count = 1;
	}

	tdata.mtx_set = &tdata.sets[0];
	tdata.mrx_set = &tdata.sets[1];
	tdata.stx_set = &tdata.sets[2];
	tdata.srx_set = &tdata.sets[3];
}

int main(void)
{
	int ret;
	int counter = 0;
	bool test_pass;

	/* HOST sends DATA_FIELD_LEN bytes and receives DATA_FIELD_LEN bytes at the same time. */
	/* HOST confirms that received data is equal to data sent in previous transaction. */
	/* DEVICE sends back what it received in previous transaction. */

	/* Allocate buffers */
	allocate_buffers();

#if CONFIG_ROLE_HOST == 1
	uint8_t previous_data[DATA_FIELD_LEN] = { 0 };
	/* Variables used to generate pseudo-random values: */
	uint8_t acc = 0;

	LOG_INF("%s runs as a SPI HOST", CONFIG_BOARD_TARGET);

	/* Run test forever */
	while (1) {
		test_pass = true;

		/* Generate pseudo random tx_data for current test */
		for (int i = 0; i < DATA_FIELD_LEN; i++) {
			*((uint8_t *) tdata.bufs[0].buf + i) = acc;
			acc += DELTA;
		}

		/* Transmit data */
		ret = spi_transceive_dt(&spim, tdata.mtx_set, tdata.mrx_set);
		if (ret != 0) {
			LOG_ERR("spi_transceive_dt returned %d", ret);
		}

		/* Check that received data is equal to data send previously */
		for (int i = 0; i < DATA_FIELD_LEN; i++) {
			if (*((uint8_t *) tdata.bufs[1].buf + i) != previous_data[i]) {
				LOG_ERR("FAIL: rx[%d] = %d, expected %d",
					i, *((uint8_t *) tdata.bufs[1].buf + i), previous_data[i]);
				test_pass = false;
			}
		}

		/* Backup tx_data for next transaction */
		for (int i = 0; i < DATA_FIELD_LEN; i++) {
			previous_data[i] = *((uint8_t *) tdata.bufs[0].buf + i);
		}

		if (test_pass) {
			LOG_INF("Run %d - PASS", counter);
		} else {
			LOG_INF("Run %d - FAILED", counter);
		}

		counter++;
		k_msleep(500);
	}
#else	/* ROLE DEVICE: */
	uint8_t previous = 0;

	LOG_INF("%s runs as a SPI DEVICE", CONFIG_BOARD_TARGET);

	/* Run test forever */
	while (1) {
		test_pass = true;

		/* Receive new data, send what was received previously */
		ret = spi_transceive(spis_dev, &spis_config, tdata.stx_set, tdata.srx_set);
		if (ret <= 0) {
			LOG_ERR("spi_transceive_dt returned %d", ret);
		}

		/* Verify that received bytes differ by DELTA */
		/* First byte shall be (last byte from previous transaction + DELTA) */
		uint8_t byte_0 = *((uint8_t *) tdata.bufs[3].buf);
		uint8_t expected = previous + DELTA;

		if (byte_0 != expected) {
			LOG_ERR("FAIL: rx[0] = %d, expected %d", byte_0, expected);
			test_pass = false;
		}
		/* Next byte shall be (previous byte + DELTA) */
		for (int i = 1; i < DATA_FIELD_LEN; i++) {
			uint8_t byte_i = *((uint8_t *) tdata.bufs[3].buf + i);
			uint8_t byte_i_less_1 = *((uint8_t *) tdata.bufs[3].buf + i - 1);

			if (byte_i != byte_i_less_1 + DELTA) {
				LOG_ERR("FAIL: rx[%d] = %d, expected %d",
					i, byte_i, (byte_i_less_1 + DELTA));
				test_pass = false;
			}
		}

		/* Copy received data to TX buffer, so it will be returned back to sender */
		for (int i = 0; i < DATA_FIELD_LEN; i++) {
			*((uint8_t *) tdata.bufs[2].buf + i) = *((uint8_t *) tdata.bufs[3].buf + i);
		}

		/* Backup last byte for next transaction */
		previous = *((uint8_t *) tdata.bufs[3].buf + DATA_FIELD_LEN - 1);

		if (test_pass) {
			LOG_INF("Run %d - PASS", counter);
		} else {
			LOG_INF("Run %d - FAILED", counter);
		}
		counter++;

		LOG_DBG("----");
		LOG_DBG("prev = %d", previous);
		LOG_DBG("rx[0] = %d", *((uint8_t *) tdata.bufs[3].buf + 0));
		LOG_DBG("rx[1] = %d", *((uint8_t *) tdata.bufs[3].buf + 1));
		LOG_DBG("rx[2] = %d", *((uint8_t *) tdata.bufs[3].buf + 2));
		LOG_DBG("rx[%d] = %d", (DATA_FIELD_LEN - 1),
			*((uint8_t *) tdata.bufs[3].buf + DATA_FIELD_LEN - 1));
	}

#endif
	return 0;
}

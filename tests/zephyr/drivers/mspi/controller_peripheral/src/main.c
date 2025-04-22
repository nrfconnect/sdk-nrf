/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/ztest.h>
#include <drivers/mspi/hpf_mspi.h>

#if CONFIG_TESTED_SPI_MODE == 0
#define SPI_MODE (SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB)
#elif CONFIG_TESTED_SPI_MODE == 1
#define SPI_MODE (SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_LSB | SPI_MODE_CPHA)
#elif CONFIG_TESTED_SPI_MODE == 2
#define SPI_MODE (SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_LSB | SPI_MODE_CPOL)
#elif CONFIG_TESTED_SPI_MODE == 3
#define SPI_MODE                                                                                   \
	(SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB | SPI_MODE_CPHA | SPI_MODE_CPOL)
#else
#error "Invalid SPI mode"
#endif

#define MSPI_BUS_NODE	    DT_ALIAS(mspi0)
#define SPIS_OP		    (SPI_OP_MODE_SLAVE | SPI_MODE)
#define TEST_AREA_OFFSET    0xff000
#define TEST_MEMORY_DEV_IDX 0
#define TEST_SPIS_DEV_IDX   1

static const struct device *mspi_devices[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, DEVICE_DT_GET, (,))
};

static struct mspi_dev_id dev_id[] = {
	{
		.dev_idx = TEST_MEMORY_DEV_IDX,
	},
	{
		.dev_idx = TEST_SPIS_DEV_IDX,
	},
};

static const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);
static const struct device *spis_dev = DEVICE_DT_GET(DT_NODELABEL(dut_spis));
static const struct spi_config spis_config = {
	.operation = SPIS_OP
};

static uint8_t mspi_buffer[32];
static uint8_t spis_buffer[32];

struct test_data {
	struct k_work_delayable test_work;
	struct k_sem sem;
	int mspi_alloc_idx;
	int spis_alloc_idx;
	struct spi_buf_set sets[4];
	struct spi_buf_set *mtx_set;
	struct spi_buf_set *mrx_set;
	struct spi_buf_set *stx_set;
	struct spi_buf_set *srx_set;
	struct spi_buf bufs[8];
	bool test_emu_spis_dev;
};

static struct test_data tdata;

/* Allocate buffer from mspi or spis space. */
static uint8_t *buf_alloc(size_t len, bool mspi)
{
	int *idx = mspi ? &tdata.mspi_alloc_idx : &tdata.spis_alloc_idx;
	uint8_t *buf = mspi ? mspi_buffer : spis_buffer;
	size_t total = mspi ? sizeof(mspi_buffer) : sizeof(spis_buffer);
	uint8_t *rv;

	if (*idx + len > total) {
		zassert_false(true);

		return NULL;
	}

	rv = &buf[*idx];
	*idx += len;

	return rv;
}

static void work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct test_data *td = CONTAINER_OF(dwork, struct test_data, test_work);
	int ret = 0;
	struct mspi_xfer_packet tx_packet = {
		.cmd = 0x55,
		.address = 0xAA,
		.dir = MSPI_TX,
		.num_bytes = (td->mtx_set != NULL) ? td->mtx_set->buffers[0].len : 0,
		.data_buf = (td->mtx_set != NULL) ? td->mtx_set->buffers[0].buf : NULL,
	};
	struct mspi_xfer_packet rx_packet = {
		.cmd = 0x55,
		.address = 0xAA,
		.dir = MSPI_RX,
		.num_bytes = (td->mrx_set != NULL) ? td->mrx_set->buffers[0].len : 0,
		.data_buf = (td->mrx_set != NULL) ? td->mrx_set->buffers[0].buf : NULL,
	};
	struct mspi_xfer xfer = {
		.xfer_mode = MSPI_PIO,
		.rx_dummy = 0,
		.cmd_length = 0,
		.addr_length = 0,
		.packets = &tx_packet,
		.num_packet = 1,
		.priority = 1,
		.timeout = 30,
	};

	if (td->test_emu_spis_dev == TEST_SPIS_DEV_IDX) {
		struct mspi_dev_cfg device_cfg = {
			.ce_num = TEST_SPIS_DEV_IDX,
			.freq = CONFIG_TESTED_SPI_FREQUENCY,
			.io_mode = MSPI_IO_MODE_SINGLE,
			.data_rate = MSPI_DATA_RATE_SINGLE,
#if CONFIG_TESTED_SPI_MODE == 0
			.cpp = MSPI_CPP_MODE_0,
			.endian = MSPI_XFER_BIG_ENDIAN,
#elif CONFIG_TESTED_SPI_MODE == 1
			.cpp = MSPI_CPP_MODE_1,
			.endian = MSPI_XFER_LITTLE_ENDIAN,
#elif CONFIG_TESTED_SPI_MODE == 2
			.cpp = MSPI_CPP_MODE_2,
			.endian = MSPI_XFER_LITTLE_ENDIAN,
#elif CONFIG_TESTED_SPI_MODE == 3
			.cpp = MSPI_CPP_MODE_3,
			.endian = MSPI_XFER_BIG_ENDIAN,
#endif
			.ce_polarity = MSPI_CE_ACTIVE_LOW,
		};

		ret = mspi_dev_config(mspi_bus, &dev_id[TEST_SPIS_DEV_IDX], MSPI_DEVICE_CONFIG_ALL,
				      &device_cfg);
		zassert_false(ret < 0, "mspi_dev_config() failed: %d", ret);

		/* write or/and read data to/from emulated spis device */
		if (td->mtx_set) {
			xfer.packets = &tx_packet;
			ret = mspi_transceive(mspi_bus, &dev_id[TEST_SPIS_DEV_IDX], &xfer);
		}
		if (td->mrx_set) {
			xfer.packets = &rx_packet;
			ret = mspi_transceive(mspi_bus, &dev_id[TEST_SPIS_DEV_IDX], &xfer);
		}
	} else {
		/* transfer data to memory */
		if (td->mtx_set) {
			ret = flash_write(mspi_devices[TEST_MEMORY_DEV_IDX], TEST_AREA_OFFSET,
					  td->mtx_set->buffers[0].buf, td->mtx_set->buffers[0].len);
			zassert_equal(ret, 0, "Cannot write flash");
		}
	}

	if (ret == 0) {
		k_sem_give(&td->sem);
	};
	zassert_equal(ret, 0, "transceive failed.");
}

/** Copies data from buffers in the set to a single buffer which makes it easier
 * to compare transmitted and received data.
 *
 * @param buf Output buffer.
 * @param len Buffer length.
 * @param set Set of buffers.
 *
 * @return Number of bytes copied.
 */
static int cpy_data(uint8_t *buf, size_t len, struct spi_buf_set *set)
{
	int idx = 0;

	for (size_t i = 0; i < set->count; i++) {
		size_t bytes = set->buffers[i].len;

		if (len - idx >= bytes) {
			memcpy(&buf[idx], set->buffers[i].buf, bytes);
			idx += bytes;
		} else {
			return -1;
		}
	}

	return idx;
}

static void to_display_format(const uint8_t *src, size_t size, char *dst)
{
	size_t i;

	for (i = 0; i < size; i++) {
		sprintf(dst + 5 * i, "0x%02x,", src[i]);
	}
}

/** Compare two sets.
 *
 * @param tx_set TX set.
 * @param rx_set RX set.
 * @param same_size True if it is expected to have the same amount of data in both sets.
 *
 * @return 0 if data is the same and other value indicate that check failed.
 */
static int check_buffers(struct spi_buf_set *tx_set, struct spi_buf_set *rx_set, bool same_size)
{
	static uint8_t tx_data[256];
	static uint8_t rx_data[256];
	int rx_len;
	int tx_len;

	if (!tx_set || !rx_set) {
		return 0;
	}

	rx_len = cpy_data(rx_data, sizeof(rx_data), rx_set);
	tx_len = cpy_data(tx_data, sizeof(tx_data), tx_set);
	if (same_size && (rx_len != tx_len)) {
		return -1;
	}

	if (memcmp(tx_data, rx_data, rx_len)) {
		/*
		 * We need 5x(buffer size) + 1 to print a comma-separated list of each
		 * byte in hex, plus a null.
		 */
		uint8_t buffer_print_tx[rx_len * 5 + 1];
		uint8_t buffer_print_rx[rx_len * 5 + 1];

		to_display_format(tx_data, rx_len, buffer_print_tx);
		to_display_format(rx_data, rx_len, buffer_print_rx);
		printf("Buffer contents are different:\nTX: %s\n", buffer_print_tx);
		printf("RX: %s\n", buffer_print_rx);
		return -1;
	}

	return 0;
}

/** Calculate expected number of received bytes by the SPI peripheral.
 *
 * It is used to check if SPI API call for peripheral SPI device returns correct value.
 * @param tx_set TX set.
 * @param rx_set RX set.
 *
 * @return Expected amount of received bytes.
 */
static int peripheral_rx_len(struct spi_buf_set *tx_set, struct spi_buf_set *rx_set)
{
	size_t tx_len = 0;
	size_t rx_len = 0;

	if (!tx_set || !rx_set) {
		return 0;
	}

	for (size_t i = 0; i < tx_set->count; i++) {
		tx_len += tx_set->buffers[i].len;
	}

	for (size_t i = 0; i < rx_set->count; i++) {
		rx_len += rx_set->buffers[i].len;
	}

	return MIN(rx_len, tx_len);
}

/** Generic function which runs the test with sets prepared in the test data structure. */
static void run_test(bool m_same_size, bool s_same_size, bool emu_spis_dev)
{
	int rv;
	int periph_rv;
	int srx_len;

	tdata.test_emu_spis_dev = emu_spis_dev;

	rv = k_work_schedule(&tdata.test_work, K_MSEC(10));
	zassert_equal(rv, 1);

	if (emu_spis_dev) {
		if (tdata.srx_set) {
			periph_rv = spi_transceive(spis_dev, &spis_config, NULL, tdata.srx_set);
			if (periph_rv == -ENOTSUP) {
				ztest_test_skip();
			}
		}

		if (tdata.stx_set) {
			if (-ENOTSUP ==
			    spi_transceive(spis_dev, &spis_config, tdata.stx_set, NULL)) {
				ztest_test_skip();
			}
		}
	} else {
		if (tdata.srx_set) {
			rv = flash_read(mspi_devices[TEST_MEMORY_DEV_IDX], TEST_AREA_OFFSET,
					tdata.srx_set->buffers[0].buf,
					tdata.srx_set->buffers[0].len);
			zassert_equal(rv, 0, "Cannot read flash");
		}
	}

	rv = k_sem_take(&tdata.sem, K_MSEC(100));
	zassert_equal(rv, 0);

	/* This releases the MSPI controller. */
	(void)mspi_get_channel_status(mspi_bus, 0);

	if (emu_spis_dev && tdata.srx_set) {
		srx_len = peripheral_rx_len(tdata.mtx_set, tdata.srx_set);
		zassert_equal(periph_rv, srx_len, "Got: %d but expected:%d", periph_rv, srx_len);
	}

	rv = check_buffers(tdata.mtx_set, tdata.srx_set, m_same_size);
	zassert_equal(rv, 0);

	rv = check_buffers(tdata.stx_set, tdata.mrx_set, s_same_size);
	zassert_equal(rv, 0);
}

/** Basic test where SPI controller and SPI peripheral have RX and TX sets which contains only one
 *  same size buffer.
 */
ZTEST(mspi_controller_peripheral, test_basic)
{
	size_t len = 16;

	for (int i = 0; i < 4; i++) {
		tdata.bufs[i].buf = buf_alloc(len, i < 2);
		tdata.bufs[i].len = len;
		tdata.sets[i].buffers = &tdata.bufs[i];
		tdata.sets[i].count = 1;
	}

	tdata.mtx_set = &tdata.sets[0];
	tdata.mrx_set = &tdata.sets[1];
	tdata.stx_set = &tdata.sets[2];
	tdata.srx_set = &tdata.sets[3];

	run_test(true, true, true);
	run_test(true, true, false);
}

/** Basic test with zero length buffers.
 */
ZTEST(mspi_controller_peripheral, test_basic_zero_len)
{
	size_t len = 8;

	/* SPIM */
	tdata.bufs[0].buf = buf_alloc(len, true);
	tdata.bufs[0].len = len;
	tdata.bufs[1].buf = buf_alloc(len, true);
	/* Intentionally len was set to 0 - second buffer "is empty". */
	tdata.bufs[1].len = 0;
	tdata.sets[0].buffers = &tdata.bufs[0];
	tdata.sets[0].count = 2;
	tdata.mtx_set = &tdata.sets[0];

	tdata.bufs[2].buf = buf_alloc(len, true);
	tdata.bufs[2].len = len;
	tdata.bufs[3].buf = buf_alloc(len, true);
	/* Intentionally len was set to 0 - second buffer "is empty". */
	tdata.bufs[3].len = 0;
	tdata.sets[1].buffers = &tdata.bufs[2];
	tdata.sets[1].count = 2;
	tdata.mrx_set = &tdata.sets[1];

	/* SPIS */
	tdata.bufs[4].buf = buf_alloc(len, false);
	tdata.bufs[4].len = len;
	tdata.sets[2].buffers = &tdata.bufs[4];
	tdata.sets[2].count = 1;
	tdata.stx_set = &tdata.sets[2];

	tdata.bufs[6].buf = buf_alloc(len, false);
	tdata.bufs[6].len = len;
	tdata.sets[3].buffers = &tdata.bufs[6];
	tdata.sets[3].count = 1;
	tdata.srx_set = &tdata.sets[3];

	run_test(true, true, true);
	run_test(true, true, false);
}

/** Setup a transfer where RX buffer on SPI controller and SPI peripheral are
 *  shorter than TX buffers. RX buffers shall contain beginning of TX data
 *  and last TX bytes that did not fit in the RX buffers shall be lost.
 */
ZTEST(mspi_controller_peripheral, test_short_rx)
{
	size_t len = 16;

	tdata.bufs[0].buf = buf_alloc(len, true);
	tdata.bufs[0].len = len;
	tdata.bufs[1].buf = buf_alloc(len, true);
	tdata.bufs[1].len = len - 3; /* RX buffer */
	tdata.bufs[2].buf = buf_alloc(len, false);
	tdata.bufs[2].len = len;
	tdata.bufs[3].buf = buf_alloc(len, false);
	tdata.bufs[3].len = len - 4; /* RX buffer */

	for (int i = 0; i < 4; i++) {
		tdata.sets[i].buffers = &tdata.bufs[i];
		tdata.sets[i].count = 1;
	}

	tdata.mtx_set = &tdata.sets[0];
	tdata.mrx_set = &tdata.sets[1];
	tdata.stx_set = &tdata.sets[2];
	tdata.srx_set = &tdata.sets[3];

	run_test(false, false, true);
	run_test(false, false, false);
}

/** Test where only master transmits. */
ZTEST(mspi_controller_peripheral, test_only_tx)
{
	size_t len = 16;

	/* MTX buffer */
	tdata.bufs[0].buf = buf_alloc(len, true);
	tdata.bufs[0].len = len;
	tdata.sets[0].buffers = &tdata.bufs[0];
	tdata.sets[0].count = 1;
	tdata.mtx_set = &tdata.sets[0];
	tdata.mrx_set = NULL;

	/* STX buffer */
	tdata.bufs[1].buf = buf_alloc(len, false);
	tdata.bufs[1].len = len;
	tdata.sets[1].buffers = &tdata.bufs[1];
	tdata.sets[1].count = 1;
	tdata.srx_set = &tdata.sets[1];
	tdata.stx_set = NULL;

	run_test(true, true, true);
	run_test(true, true, false);
}

/** Test where only SPI controller transmits and SPI peripheral receives in chunks. */
ZTEST(mspi_controller_peripheral, test_only_tx_in_chunks)
{
	size_t len1 = 7;
	size_t len2 = 8;

	/* MTX buffer */
	tdata.bufs[0].buf = buf_alloc(len1 + len2, true);
	tdata.bufs[0].len = len1 + len2;
	tdata.sets[0].buffers = &tdata.bufs[0];
	tdata.sets[0].count = 1;
	tdata.mtx_set = &tdata.sets[0];
	tdata.mrx_set = NULL;

	/* STX buffer */
	tdata.bufs[1].buf = buf_alloc(len1, false);
	tdata.bufs[1].len = len1;
	tdata.bufs[2].buf = buf_alloc(len2, false);
	tdata.bufs[2].len = len2;
	tdata.sets[1].buffers = &tdata.bufs[1];
	tdata.sets[1].count = 2;
	tdata.srx_set = &tdata.sets[1];
	tdata.stx_set = NULL;

	run_test(true, true, true);
	run_test(true, true, false);
}

/** Test where only SPI peripheral transmits. */
ZTEST(mspi_controller_peripheral, test_only_rx)
{
	size_t len = 16;

	/* MTX buffer */
	tdata.bufs[0].buf = buf_alloc(len, true);
	tdata.bufs[0].len = len;
	tdata.sets[0].buffers = &tdata.bufs[0];
	tdata.sets[0].count = 1;
	tdata.mrx_set = &tdata.sets[0];
	tdata.mtx_set = NULL;

	/* STX buffer */
	tdata.bufs[1].buf = buf_alloc(len, false);
	tdata.bufs[1].len = len;
	tdata.sets[1].buffers = &tdata.bufs[1];
	tdata.sets[1].count = 1;
	tdata.stx_set = &tdata.sets[1];
	tdata.srx_set = NULL;

	run_test(true, true, true);
	run_test(true, true, false);
}

/** Test where only SPI peripheral transmits in chunks. */
ZTEST(mspi_controller_peripheral, test_only_rx_in_chunks)
{
	size_t len1 = 7;
	size_t len2 = 9;

	/* MTX buffer */
	tdata.bufs[0].buf = buf_alloc(len1 + len2, true);
	tdata.bufs[0].len = len1 + len2;
	tdata.sets[0].buffers = &tdata.bufs[0];
	tdata.sets[0].count = 1;
	tdata.mrx_set = &tdata.sets[0];
	tdata.mtx_set = NULL;

	/* STX buffer */
	tdata.bufs[1].buf = buf_alloc(len1, false);
	tdata.bufs[1].len = len1;
	tdata.bufs[2].buf = buf_alloc(len2, false);
	tdata.bufs[2].len = len2;
	tdata.sets[1].buffers = &tdata.bufs[1];
	tdata.sets[1].count = 2;
	tdata.stx_set = &tdata.sets[1];
	tdata.srx_set = NULL;

	run_test(true, true, true);
	run_test(true, true, false);
}

static void before(void *not_used)
{
	ARG_UNUSED(not_used);

	memset(&tdata, 0, sizeof(tdata));
	for (size_t i = 0; i < sizeof(mspi_buffer); i++) {
		mspi_buffer[i] = (uint8_t)i;
	}
	for (size_t i = 0; i < sizeof(spis_buffer); i++) {
		spis_buffer[i] = (uint8_t)(i + 0x80);
	}

	k_work_init_delayable(&tdata.test_work, work_handler);
	k_sem_init(&tdata.sem, 0, 1);
}

static void after(void *not_used)
{
	ARG_UNUSED(not_used);

	k_work_cancel_delayable(&tdata.test_work);
}

ZTEST_SUITE(mspi_controller_peripheral, NULL, NULL, before, after, NULL);

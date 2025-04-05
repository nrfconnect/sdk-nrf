/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/ztest.h>

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

#define MSPI_BUS_NODE    DT_NODELABEL(sdp_mspi)
#define SPIS_OP	 (SPI_OP_MODE_SLAVE | SPI_MODE)

// static const struct device *mspi_devices[] = {
// 	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, DEVICE_DT_GET, (,))
// };

#define TEST_AREA_DEV_NODE0	DT_ALIAS(testdevice0) // DT_INST(0, jedec_mspi_nor)
#define TEST_AREA_DEV_NODE1	DT_ALIAS(testdevice1) // DT_INST(0, zephyr_mspi_emul_device)

#if !DT_NODE_EXISTS(TEST_AREA_DEV_NODE0)
#error "TEST_AREA_DEV_NODE0 does not exist"
#endif

#if !DT_NODE_EXISTS(TEST_AREA_DEV_NODE1)
#error "TEST_AREA_DEV_NODE1 does not exist"
#endif

static const struct device *const test_device0 = DEVICE_DT_GET(TEST_AREA_DEV_NODE0);
static const struct device *const test_device1 = DEVICE_DT_GET(TEST_AREA_DEV_NODE1);

static const struct device *const mspi_devices[] = {
	test_device0,
	test_device1,
};

// static struct gpio_dt_spec ce_gpios[] = MSPI_CE_GPIOS_DT_SPEC_GET(MSPI_BUS_NODE);

// const static struct mspi_cfg hardware_cfg = {
// 	.channel_num              = 0,
// 	.op_mode                  = DT_ENUM_IDX_OR(MSPI_BUS_NODE, op_mode, MSPI_OP_MODE_CONTROLLER),
// 	.duplex                   = DT_ENUM_IDX_OR(MSPI_BUS_NODE, duplex, MSPI_HALF_DUPLEX),
// 	.dqs_support              = false,
// 	.ce_group                 = ce_gpios,
// 	.num_ce_gpios             = ARRAY_SIZE(ce_gpios),
// 	.num_periph               = DT_CHILD_NUM(MSPI_BUS_NODE),
// 	.max_freq                 = DT_PROP(MSPI_BUS_NODE, clock_frequency),
// 	.re_init                  = true,
// };

// static struct mspi_dev_id dev_id[] = {
// 	{
// 		.dev_idx = 0,
// 	},
// 	{
// 		.dev_idx = 1,
// 	}
// };

static struct mspi_dev_id dev_id[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, MSPI_DEVICE_ID_DT, (,))
};

static struct mspi_dev_cfg device_cfg[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, MSPI_DEVICE_CONFIG_DT, (,))
};

static const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);
static const struct device *spis_dev = DEVICE_DT_GET(DT_NODELABEL(dut_spis));
static const struct spi_config spis_config = {
	.operation = SPIS_OP
};

#define MEMORY_SECTION(node)                                                                       \
	COND_CODE_1(DT_NODE_HAS_PROP(node, memory_regions),                                        \
		    (__attribute__((__section__(                                                   \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(node, memory_regions)))))),      \
		    ())

static uint8_t mspi_buffer[32];
static uint8_t spis_buffer[32] MEMORY_SECTION(DT_NODELABEL(dut_spis));

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
	int ret, i;
	int packet_count = td->mtx_set->count + td->mrx_set->count;
	struct mspi_xfer_packet xfer_packets[packet_count];
	struct mspi_xfer xfer = {
		.xfer_mode = MSPI_PIO,
		.rx_dummy = 0,
		.cmd_length = 8,
		.addr_length = 8,
		.packets = xfer_packets,
		.num_packet = sizeof(xfer_packets) / sizeof(struct mspi_xfer_packet),
		.priority = 1,
		.timeout = 30,
	};

	printf("work_handler\n");

	_Static_assert(ARRAY_SIZE(mspi_devices) == 2, "Test requires 2 MSPI devices");

	for (int dev_idx = 0; dev_idx < 1;/*ARRAY_SIZE(mspi_devices);*/ ++dev_idx) {
#if CONFIG_TESTED_SPI_MODE == 0
		device_cfg[dev_idx].io_mode = MSPI_IO_MODE_SINGLE;
		device_cfg[dev_idx].cpp = MSPI_CPP_MODE_0;
		device_cfg[dev_idx].endian = MSPI_XFER_LITTLE_ENDIAN;
#elif CONFIG_TESTED_SPI_MODE == 1
		device_cfg[dev_idx].io_mode = MSPI_IO_MODE_SINGLE;
		device_cfg[dev_idx].cpp = MSPI_CPP_MODE_1;
		device_cfg[dev_idx].endian = MSPI_XFER_BIG_ENDIAN;
#elif CONFIG_TESTED_SPI_MODE == 2
		device_cfg[dev_idx].io_mode = MSPI_IO_MODE_SINGLE;
		device_cfg[dev_idx].cpp = MSPI_CPP_MODE_2;
		device_cfg[dev_idx].endian = MSPI_XFER_LITTLE_ENDIAN;
#elif CONFIG_TESTED_SPI_MODE == 3
		device_cfg[dev_idx].io_mode = MSPI_IO_MODE_SINGLE;
		device_cfg[dev_idx].cpp = MSPI_CPP_MODE_3;
		device_cfg[dev_idx].endian = MSPI_XFER_BIG_ENDIAN;
#endif

		zassert_true(device_is_ready(mspi_devices[dev_idx]), "mspi_device is not ready");

		ret = mspi_dev_config(mspi_bus, &dev_id[dev_idx],
					MSPI_DEVICE_CONFIG_ALL, &device_cfg[dev_idx]);
		zassert_equal(ret, 0, "mspi_dev_config failed.");

		for (i = 0; i < td->mtx_set->count; ++i) {
			xfer_packets[i].cmd = 0;
			xfer_packets[i].address = 0;
			xfer_packets[i].dir = MSPI_TX,
			xfer_packets[i].num_bytes = td->mtx_set->buffers[i].len;
			xfer_packets[i].data_buf = td->mtx_set->buffers[i].buf;
		};

		for (i = td->mtx_set->count; i < packet_count; ++i) {
			xfer_packets[i].cmd = 0;
			xfer_packets[i].address = 0;
			xfer_packets[i].dir = MSPI_RX,
			xfer_packets[i].num_bytes = td->mrx_set->buffers[i].len;
			xfer_packets[i].data_buf = td->mrx_set->buffers[i].buf;
		};

		// ret = mspi_transceive(mspi_bus, &dev_id[dev_idx], &xfer);
		// if (ret == 0) {
		// 	k_sem_give(&td->sem);
		// };
		// zassert_equal(ret, 0, "mspi_transceive failed.");
	}
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
		size_t l = set->buffers[i].len;

		if (len - idx >= l) {
			memcpy(&buf[idx], set->buffers[i].buf, l);
			idx += l;
		} else {
			return -1;
		}
	}

	return idx;
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

	return memcmp(tx_data, rx_data, rx_len);
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
static void run_test(bool m_same_size, bool s_same_size)
{
	int rv;
	int periph_rv;
	int srx_len;

	rv = k_work_schedule(&tdata.test_work, K_MSEC(10));
	zassert_equal(rv, 1);

	periph_rv = spi_transceive(spis_dev, &spis_config, tdata.stx_set, tdata.srx_set);
	if (periph_rv == -ENOTSUP) {
		ztest_test_skip();
	}

	rv = k_sem_take(&tdata.sem, K_MSEC(100));
	zassert_equal(rv, 0);

	srx_len = peripheral_rx_len(tdata.mtx_set, tdata.srx_set);

	zassert_equal(periph_rv, srx_len, "Got: %d but expected:%d", periph_rv, srx_len);

	rv = check_buffers(tdata.mtx_set, tdata.srx_set, m_same_size);
	zassert_equal(rv, 0);

	rv = check_buffers(tdata.stx_set, tdata.mrx_set, s_same_size);
	zassert_equal(rv, 0);
}

/** Basic test where SPI controller and SPI peripheral have RX and TX sets which contains only one
 *  same size buffer.
 */
static void test_basic(void)
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

	run_test(true, true);
}

ZTEST(mspi_controller_peripheral, test_basic)
{
	test_basic();
}

ZTEST(mspi_controller_peripheral, test_basic_async)
{
	test_basic();
}

/** Basic test with zero length buffers.
 */
void test_basic_zero_len(void)
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

	run_test(true, true);
}

ZTEST(mspi_controller_peripheral, test_basic_zero_len)
{
	test_basic_zero_len();
}

ZTEST(mspi_controller_peripheral, test_basic_zero_len_async)
{
	test_basic_zero_len();
}

/** Setup a transfer where RX buffer on SPI controller and SPI peripheral are
 *  shorter than TX buffers. RX buffers shall contain beginning of TX data
 *  and last TX bytes that did not fit in the RX buffers shall be lost.
 */
static void test_short_rx(void)
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

	run_test(false, false);
}

ZTEST(mspi_controller_peripheral, test_short_rx)
{
	test_short_rx();
}

ZTEST(mspi_controller_peripheral, test_short_rx_async)
{
	test_short_rx();
}

/** Test where only master transmits. */
static void test_only_tx(void)
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

	run_test(true, true);
}

ZTEST(mspi_controller_peripheral, test_only_tx)
{
	test_only_tx();
}

ZTEST(mspi_controller_peripheral, test_only_tx_async)
{
	test_only_tx();
}

/** Test where only SPI controller transmits and SPI peripheral receives in chunks. */
static void test_only_tx_in_chunks(void)
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

	run_test(true, true);
}

ZTEST(mspi_controller_peripheral, test_only_tx_in_chunks)
{
	test_only_tx_in_chunks();
}

ZTEST(mspi_controller_peripheral, test_only_tx_in_chunks_async)
{
	test_only_tx_in_chunks();
}

/** Test where only SPI peripheral transmits. */
static void test_only_rx(void)
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

	run_test(true, true);
}

ZTEST(mspi_controller_peripheral, test_only_rx)
{
	test_only_rx();
}

ZTEST(mspi_controller_peripheral, test_only_rx_async)
{
	test_only_rx();
}

/** Test where only SPI peripheral transmits in chunks. */
static void test_only_rx_in_chunks(void)
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

	run_test(true, true);
}

ZTEST(mspi_controller_peripheral, test_only_rx_in_chunks)
{
	test_only_rx_in_chunks();
}

ZTEST(mspi_controller_peripheral, test_only_rx_in_chunks_async)
{
	test_only_rx_in_chunks();
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

static void *suite_setup(void)
{
	// int ret;
	// struct mspi_dt_spec mspi_spec = {
	// 	.bus    = mspi_bus,
	// 	.config = hardware_cfg,
	// };

	// zassert_true(device_is_ready(mspi_bus), "mspi_bus is not ready");

	// ret = mspi_config(&mspi_spec);
	// zassert_equal(ret, 0, "mspi_config failed.");

	// mspi_devices[0] = DEVICE_DT_GET(TEST_AREA_DEV_NODE0);
	// mspi_devices[1] = DEVICE_DT_GET(TEST_AREA_DEV_NODE1);

	printf("suite_setup\n");

	return NULL;
}

ZTEST_SUITE(mspi_controller_peripheral, NULL, suite_setup, before, after, NULL);

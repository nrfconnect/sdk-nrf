/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/ztest.h>

#define MSPI_NODE DT_NODELABEL(dut)

#define DATA_LINES_MAX 4

#define SCK_FREQUENCY MHZ(4)

#define CMD_LEN_MAX 2
#define ADDR_LEN_MAX 4
#define DATA_LEN_MAX 500

#define PRINT_RAW_DATA 0

static uint8_t packet_buf[DATA_LEN_MAX];

static const struct device *mspi_dev = DEVICE_DT_GET(MSPI_NODE);
static const struct mspi_dev_id mspi_id = {
	.dev_idx = 0,
};

static const struct spi_config spis_config = {
	.operation = SPI_OP_MODE_SLAVE | SPI_WORD_SET(8) | SPI_LINES_SINGLE |
		     SPI_TRANSFER_MSB
};
#define SPIS_DEV_ENTRY(i, _) DEVICE_DT_GET(DT_NODELABEL(test_line##i))
static const struct device *spis_dev[DATA_LINES_MAX] = {
	LISTIFY(DATA_LINES_MAX, SPIS_DEV_ENTRY, (,))
};
static struct k_poll_signal async_sig[DATA_LINES_MAX];
static struct k_poll_event async_evt[DATA_LINES_MAX];

/* Use buffers that are one byte longer than the maximum length of a transfer,
 * since for some mysterious reason it often happened that when the SPIS buffer
 * had exactly the same size as a transfer length, the last received byte had
 * wrong value, although sQSPI transferred it correctly.
 */
static uint8_t rx_arr[DATA_LINES_MAX]
		     [CMD_LEN_MAX + ADDR_LEN_MAX + DATA_LEN_MAX + 1];
#define RX_BUF_ENTRY(i, _) [i] = { .buf = rx_arr[i], .len = sizeof(rx_arr[i]), }
static const struct spi_buf rx_buf[] = {
	LISTIFY(DATA_LINES_MAX, RX_BUF_ENTRY, (,))
};
#define RX_SET_ENTRY(i, _) [i] = { .buffers = &rx_buf[i], .count = 1, }
static const struct spi_buf_set rx_set[] = {
	LISTIFY(DATA_LINES_MAX, RX_SET_ENTRY, (,))
};

static uint32_t get_octet(uint8_t *octet, uint8_t num_lines, uint32_t stream_pos)
{
	uint8_t octet_bit = BIT(7);

	*octet = 0;

	do {
		uint8_t line_no = num_lines;
		uint32_t rx_idx = stream_pos / 8;
		uint8_t rx_bit = 7 - (stream_pos % 8);

		do {
			if (rx_arr[line_no - 1][rx_idx] & BIT(rx_bit)) {
				*octet |= octet_bit;
			}
			octet_bit >>= 1;
		} while (--line_no);

		++stream_pos;
	} while (octet_bit);

	return stream_pos;
}

static void test_tx_transfer(struct mspi_xfer *xfer, uint8_t cmd_lines,
			     uint8_t addr_lines, uint8_t data_lines)
{
	const struct mspi_xfer_packet *packet = &xfer->packets[0];
	uint32_t stream_pos = 0;
	uint8_t octet;
	uint8_t cmd_addr_cycles;
	int rc;

	/* Clean up buffers and events for all SPIS peripherals before the next
	 * transfer and start asynchronous reception on all of them.
	 */
	for (int i = 0; i < ARRAY_SIZE(spis_dev); ++i) {
		memset(rx_arr[i], 0xAA, sizeof(rx_arr[i]));

		async_evt[i].signal->signaled = 0;
		async_evt[i].state = K_POLL_STATE_NOT_READY;

		rc = spi_read_signal(spis_dev[i], &spis_config, &rx_set[i],
				     &async_sig[i]);
		zassert_false(rc < 0, "spi_read_signal() %d failed: %d", i, rc);
	}

	/*
	 * SPIS peipherals can only receive full bytes, so use dummy cycles
	 * if needed for a given MSPI transfer to avoid incomplete last byte.
	 */
	cmd_addr_cycles = (xfer->cmd_length * 8 / cmd_lines)
			+ (xfer->addr_length * 8 / addr_lines);
	xfer->tx_dummy = 8 - (cmd_addr_cycles % 8);

	rc = mspi_transceive(mspi_dev, &mspi_id, xfer);
	zassert_false(rc < 0, "mspi_transceive() failed: %d", rc);

	rc = k_poll(async_evt, ARRAY_SIZE(async_evt), K_MSEC(100));
	zassert_false(rc < 0, "one or more events are not ready");

	if (PRINT_RAW_DATA) {
		for (int i = 0; i < ARRAY_SIZE(spis_dev); ++i) {
			TC_PRINT("%d", i);
			for (int n = 0; n < ARRAY_SIZE(rx_arr[i]); ++n) {
				TC_PRINT(" %02X", rx_arr[i][n]);
			}
			TC_PRINT("\n");
		}
	}

	for (int i = 0; i < xfer->cmd_length; ++i) {
		uint8_t shift = (xfer->cmd_length - 1 - i) * 8;
		uint8_t expected = (packet->cmd >> shift) & 0xff;

		stream_pos = get_octet(&octet, cmd_lines, stream_pos);
		zassert_equal(octet, expected,
			"command mismatch at index %d: 0x%02X != 0x%02X",
			i, octet, expected);
	}

	for (int i = 0; i < xfer->addr_length; ++i) {
		uint8_t shift = (xfer->addr_length - 1 - i) * 8;
		uint8_t expected = (packet->address >> shift) & 0xff;

		stream_pos = get_octet(&octet, addr_lines, stream_pos);
		zassert_equal(octet, expected,
			"address mismatch at index %d: 0x%02X != 0x%02X",
			i, octet, expected);
	}

	stream_pos += xfer->tx_dummy;

	for (int i = 0; i < packet->num_bytes; ++i) {
		uint8_t expected = packet->data_buf[i];

		stream_pos = get_octet(&octet, data_lines, stream_pos);
		zassert_equal(octet, expected,
			"data mismatch at index %d: 0x%02X != 0x%02X",
			i, octet, expected);
	}
}

static void test_tx_transfers(enum mspi_io_mode io_mode)
{
	struct mspi_dev_cfg dev_cfg = {
		.ce_num = 1,
		.freq = SCK_FREQUENCY,
		.io_mode = io_mode,
		.data_rate = MSPI_DATA_RATE_SINGLE,
		.cpp = MSPI_CPP_MODE_0,
		.endian = MSPI_XFER_BIG_ENDIAN,
		.ce_polarity = MSPI_CE_ACTIVE_LOW,
	};
	struct mspi_xfer_packet packet = {
		.dir = MSPI_TX,
		.cmd = 0x87654321,
		.address = 0x12345678,
		.data_buf = packet_buf,
	};
	struct mspi_xfer xfer = {
		.xfer_mode   = MSPI_PIO,
		.packets     = &packet,
		.num_packet  = 1,
		.timeout     = 10,
	};
	uint8_t cmd_lines, addr_lines, data_lines;
	int rc;

	switch (io_mode) {
	default:
	case MSPI_IO_MODE_SINGLE:
		cmd_lines = 1;
		addr_lines = 1;
		data_lines = 1;
		break;
	case MSPI_IO_MODE_DUAL:
		cmd_lines = 2;
		addr_lines = 2;
		data_lines = 2;
		break;
	case MSPI_IO_MODE_DUAL_1_1_2:
		cmd_lines = 1;
		addr_lines = 1;
		data_lines = 2;
		break;
	case MSPI_IO_MODE_DUAL_1_2_2:
		cmd_lines = 1;
		addr_lines = 2;
		data_lines = 2;
		break;
	case MSPI_IO_MODE_QUAD:
		cmd_lines = 4;
		addr_lines = 4;
		data_lines = 4;
		break;
	case MSPI_IO_MODE_QUAD_1_1_4:
		cmd_lines = 1;
		addr_lines = 1;
		data_lines = 4;
		break;
	case MSPI_IO_MODE_QUAD_1_4_4:
		cmd_lines = 1;
		addr_lines = 4;
		data_lines = 4;
		break;
	}

	rc = mspi_dev_config(mspi_dev, &mspi_id,
			     MSPI_DEVICE_CONFIG_ALL, &dev_cfg);
	zassert_false(rc < 0, "mspi_dev_config() failed: %d", rc);

	TC_PRINT("- 8-bit command only\n");
	xfer.cmd_length = 1;
	xfer.addr_length = 0;
	packet.num_bytes = 0;
	test_tx_transfer(&xfer, cmd_lines, addr_lines, data_lines);

	TC_PRINT("- 16-bit command only\n");
	xfer.cmd_length = 1;
	xfer.addr_length = 0;
	packet.num_bytes = 0;
	test_tx_transfer(&xfer, cmd_lines, addr_lines, data_lines);

	TC_PRINT("- 8-bit command, 24-bit address\n");
	xfer.cmd_length = 1;
	xfer.addr_length = 3;
	packet.num_bytes = DATA_LEN_MAX;
	test_tx_transfer(&xfer, cmd_lines, addr_lines, data_lines);

	TC_PRINT("- 8-bit command, 32-bit address\n");
	xfer.cmd_length = 1;
	xfer.addr_length = 4;
	packet.num_bytes = DATA_LEN_MAX;
	test_tx_transfer(&xfer, cmd_lines, addr_lines, data_lines);

	TC_PRINT("- 16-bit command, 24-bit address\n");
	xfer.cmd_length = 2;
	xfer.addr_length = 3;
	packet.num_bytes = DATA_LEN_MAX;
	test_tx_transfer(&xfer, cmd_lines, addr_lines, data_lines);

	TC_PRINT("- 16-bit command, 32-bit address\n");
	xfer.cmd_length = 2;
	xfer.addr_length = 4;
	packet.num_bytes = DATA_LEN_MAX;
	test_tx_transfer(&xfer, cmd_lines, addr_lines, data_lines);
}

ZTEST(mspi_with_spis, test_tx_single)
{
	test_tx_transfers(MSPI_IO_MODE_SINGLE);
}

ZTEST(mspi_with_spis, test_tx_dual_1_1_2)
{
	test_tx_transfers(MSPI_IO_MODE_DUAL_1_1_2);
}

ZTEST(mspi_with_spis, test_tx_dual_1_2_2)
{
	test_tx_transfers(MSPI_IO_MODE_DUAL_1_2_2);
}

ZTEST(mspi_with_spis, test_tx_quad_1_1_4)
{
	test_tx_transfers(MSPI_IO_MODE_QUAD_1_1_4);
}

ZTEST(mspi_with_spis, test_tx_quad_1_4_4)
{
	test_tx_transfers(MSPI_IO_MODE_QUAD_1_4_4);
}

static void *setup(void)
{
	for (int i = 0; i < ARRAY_SIZE(async_sig); ++i) {
		k_poll_signal_init(&async_sig[i]);
		k_poll_event_init(&async_evt[i],
				  K_POLL_TYPE_SIGNAL,
				  K_POLL_MODE_NOTIFY_ONLY,
				  &async_sig[i]);
	}

	for (int i = 0; i < DATA_LEN_MAX; ++i) {
		packet_buf[i] = (uint8_t)i;
	}

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_true(device_is_ready(mspi_dev),
		"MSPI device %s is not ready", mspi_dev->name);

	for (int i = 0; i < ARRAY_SIZE(spis_dev); ++i) {
		zassert_true(device_is_ready(spis_dev[i]),
			"SPIS device %d %s is not ready", i, spis_dev[i]->name);
	}
}

ZTEST_SUITE(mspi_with_spis, NULL, setup, before, NULL, NULL);

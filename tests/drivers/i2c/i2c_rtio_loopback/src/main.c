/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <nrfx_twis.h>
#include <zephyr/random/random.h>
#include <zephyr/ztest.h>

#define NODE_TWIM DT_NODELABEL(dut_twim)
#define NODE_TWIS DT_NODELABEL(dut_twis)

#define TWIM_DEV_CONFIG (I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER)

#define TWIS_MEMORY_SECTION                                                                        \
	COND_CODE_1(DT_NODE_HAS_PROP(NODE_TWIS, memory_regions),                                   \
		    (__attribute__((__section__(                                                   \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(NODE_TWIS, memory_regions)))))), \
		    ())

#define I2C_DEVICE_ADDR  DT_REG_ADDR(DT_NODELABEL(sensor))
/* Address with no target on the bus, transfers to it are NACKed */
#define I2C_NACK_ADDR    (I2C_DEVICE_ADDR + 1)
#define TEST_DATA_SIZE   8
#define TINY_TX_MAX_SIZE SIZEOF_FIELD(struct rtio_sqe, tiny_tx.buf)

#define TWIM_RTIO_SQ_SIZE     4
#define TWIM_I2C_RTIO_SQ_SIZE 4

/*
 * TWIM - RTIO driver
 * TWIS - standard TWIS driver
 */

RTIO_DEFINE(twim_rtio_context, TWIM_RTIO_SQ_SIZE, 2);

I2C_IODEV_DEFINE(twim_rtio_io_dev, NODE_TWIM, I2C_DEVICE_ADDR);
I2C_IODEV_DEFINE(twim_rtio_nack_io_dev, NODE_TWIM, I2C_NACK_ADDR);

/* Context for the blocking i2c_rtio_configure/transfer API */
I2C_RTIO_DEFINE(twim_i2c_rtio, TWIM_I2C_RTIO_SQ_SIZE, TWIM_I2C_RTIO_SQ_SIZE);

static nrfx_twis_t twis = {.p_reg = (NRF_TWIS_Type *)DT_REG_ADDR(NODE_TWIS)};

static uint8_t i2c_slave_buffer[TEST_DATA_SIZE] TWIS_MEMORY_SECTION;
static uint8_t i2c_master_buffer[TEST_DATA_SIZE];

struct i2c_api_twis_fixture {
	uint8_t addr;
	uint8_t *const master_buffer;
	uint8_t *const slave_buffer;
};

static struct i2c_api_twis_fixture fixture = {
	.addr = I2C_DEVICE_ADDR,
	.master_buffer = i2c_master_buffer,
	.slave_buffer = i2c_slave_buffer,
};

static void i2s_slave_handler(nrfx_twis_event_t const *p_event)
{
	switch (p_event->type) {
	case NRFX_TWIS_EVT_READ_REQ:
		nrfx_twis_tx_prepare(&twis, i2c_slave_buffer, TEST_DATA_SIZE);
		break;
	case NRFX_TWIS_EVT_READ_DONE:
		break;
	case NRFX_TWIS_EVT_WRITE_REQ:
		nrfx_twis_rx_prepare(&twis, i2c_slave_buffer, TEST_DATA_SIZE);
		break;
	case NRFX_TWIS_EVT_WRITE_DONE:
		break;
	default:
		break;
	}
}

static void cleanup_buffers(void *nullp)
{
	memset(fixture.slave_buffer, 0x00, TEST_DATA_SIZE);
	memset(fixture.master_buffer, 0xFF, TEST_DATA_SIZE);
}

static void prepare_test_data(uint8_t *buffer, size_t buffer_size)
{
	for (size_t counter = 0; counter < buffer_size; counter++) {
		buffer[counter] = sys_rand8_get();
	}
}

static void validate_buffers(void)
{
	for (unsigned int i = 0; i < TEST_DATA_SIZE; i++) {
		if (fixture.master_buffer[i] != fixture.slave_buffer[i]) {
			zexpect_equal(fixture.master_buffer[i], fixture.slave_buffer[i],
				      "master_buffer[%u] (0x%x) != slave_buffer[%u] (0x%x)\n", i,
				      fixture.master_buffer[i], i, fixture.slave_buffer[i]);
		}
	}
}

static void twim_rtio_configure(void)
{
	int ret;
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;

	sqe = rtio_sqe_acquire(&twim_rtio_context);
	rtio_sqe_prep_i2c_configure(sqe, &twim_rtio_io_dev, 0, TWIM_DEV_CONFIG, NULL);

	ret = rtio_submit(&twim_rtio_context, 1);

	zassert_ok(ret, "[RTIO CONFIG] rtio_submit failed: %d\n", ret);

	cqe = rtio_cqe_consume_block(&twim_rtio_context);
	zassert_ok(cqe->result, "[RTIO CONFIG] rtio_cqe_consume failed: %d\n", cqe->result);
	rtio_cqe_release(&twim_rtio_context, cqe);
}

/* Consume completions, expected results default to success when expected is NULL */
static void twim_rtio_consume_and_expect(unsigned int count, const int *expected)
{
	int result;
	struct rtio_cqe *cqe;

	for (unsigned int i = 0; i < count; i++) {
		cqe = rtio_cqe_consume_block(&twim_rtio_context);
		result = cqe->result;
		rtio_cqe_release(&twim_rtio_context, cqe);

		zexpect_equal(result, (expected != NULL) ? expected[i] : 0,
			      "[RTIO] completion %u unexpected result: %d\n", i, result);
	}
}

static void twim_rtio_submit_and_expect(unsigned int count, const int *expected)
{
	int ret;

	ret = rtio_submit(&twim_rtio_context, count);

	zassert_ok(ret, "[RTIO] rtio_submit failed: %d\n", ret);

	twim_rtio_consume_and_expect(count, expected);
}

static void twim_rtio_submit_and_check(unsigned int count)
{
	twim_rtio_submit_and_expect(count, NULL);
}

static void twim_rtio_write(void)
{
	struct rtio_sqe *wr_sqe;

	TC_PRINT("TWIM RTIO WRITE\n");

	wr_sqe = rtio_sqe_acquire(&twim_rtio_context);
	rtio_sqe_prep_write(wr_sqe, &twim_rtio_io_dev, 0, fixture.master_buffer, TEST_DATA_SIZE,
			    NULL);

	wr_sqe->iodev_flags = RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

	twim_rtio_submit_and_check(1);
}

static void twim_rtio_read(void)
{
	struct rtio_sqe *rd_sqe;

	TC_PRINT("TWIM RTIO READ\n");

	rd_sqe = rtio_sqe_acquire(&twim_rtio_context);
	rtio_sqe_prep_read(rd_sqe, &twim_rtio_io_dev, 0, fixture.master_buffer, TEST_DATA_SIZE,
			   NULL);

	rd_sqe->iodev_flags = RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

	twim_rtio_submit_and_check(1);
}

static void twim_rtio_tiny_write(const uint8_t *data, uint8_t len)
{
	struct rtio_sqe *sqe;

	sqe = rtio_sqe_acquire(&twim_rtio_context);
	zassert_not_null(sqe, "[RTIO TINY WRITE] rtio_sqe_acquire failed\n");

	rtio_sqe_prep_tiny_write(sqe, &twim_rtio_io_dev, RTIO_PRIO_NORM, data, len, NULL);
	sqe->iodev_flags = RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

	twim_rtio_submit_and_check(1);
}

static void validate_slave_buffer_tail(size_t offset)
{
	for (size_t i = offset; i < TEST_DATA_SIZE; i++) {
		zexpect_equal(fixture.slave_buffer[i], 0x00,
			      "slave_buffer[%zu] (0x%x) modified beyond written data\n", i,
			      fixture.slave_buffer[i]);
	}
}

static void fill_submission_queue(struct rtio *r, unsigned int count)
{
	for (unsigned int i = 0; i < count; i++) {
		zassert_not_null(rtio_sqe_acquire(r), "rtio_sqe_acquire %u failed\n", i);
	}
}

static void *test_setup(void)
{
	int ret;

	const nrfx_twis_config_t config = {
		.addr = {fixture.addr, 0},
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
	};

	zassert_equal(0, nrfx_twis_init(&twis, &config, i2s_slave_handler),
		      "TWIS initialization failed");

	PINCTRL_DT_DEFINE(NODE_TWIS);

	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(NODE_TWIS), PINCTRL_STATE_DEFAULT);

	zassert_ok(ret);

	IRQ_CONNECT(DT_IRQN(NODE_TWIS), DT_IRQ(NODE_TWIS, priority), nrfx_twis_irq_handler, &twis,
		    0);

	nrfx_twis_enable(&twis);
	TC_PRINT("TWIS CFG done\n");

	twim_rtio_configure();
	TC_PRINT("TWIM RTIO CFG done\n");

	zassert_true(device_is_ready(DEVICE_DT_GET(NODE_TWIM)), "TWIM device not ready");
	i2c_rtio_init(&twim_i2c_rtio, DEVICE_DT_GET(NODE_TWIM));

	return NULL;
}

ZTEST(twim_rtio_twis_loopback, test_twim_rtio_twis_write)
{
	TC_PRINT("TWIM RTIO write test\n");

	prepare_test_data(fixture.master_buffer, TEST_DATA_SIZE);
	twim_rtio_write();
	validate_buffers();
}

ZTEST(twim_rtio_twis_loopback, test_twim_rtio_twis_read)
{
	TC_PRINT("TWIM RTIO write test\n");

	prepare_test_data(fixture.slave_buffer, TEST_DATA_SIZE);
	twim_rtio_read();
	validate_buffers();
}

/*
 * Test case is only usable for devices that use a bounce buffer
 * for example NRF54H20, but can be run on any device
 */
ZTEST(twim_rtio_twis_loopback, test_twim_rtio_twis_rx_buffers_handling)
{
	struct rtio_sqe *wr_sqe;

	static uint8_t i2c_aux_buffer[TEST_DATA_SIZE];

	TC_PRINT("TWIM RTIO RX buffers handling test\n");

	memset(fixture.slave_buffer, 0xDB, TEST_DATA_SIZE);
	twim_rtio_read();

	memset(fixture.master_buffer, 0x00, TEST_DATA_SIZE);
	memset(i2c_aux_buffer, 0xCD, TEST_DATA_SIZE);

	wr_sqe = rtio_sqe_acquire(&twim_rtio_context);
	rtio_sqe_prep_write(wr_sqe, &twim_rtio_io_dev, 0, i2c_aux_buffer, TEST_DATA_SIZE, NULL);

	wr_sqe->iodev_flags = RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

	twim_rtio_submit_and_check(1);

	TC_PRINT("TWIM RTIO SEPARATE BUFFERS - done\n");

	zexpect_mem_equal(i2c_aux_buffer, fixture.slave_buffer, TEST_DATA_SIZE,
			  "Read data doe not match written data\n");

	zexpect_true(
		memcmp(i2c_aux_buffer, fixture.master_buffer, TEST_DATA_SIZE) != 0,
		"Buffer from the previous RTIO operation not used but modified by the driver anyway\n");
}

ZTEST(twim_rtio_twis_loopback, test_twim_rtio_twis_tiny_write)
{
	uint8_t tx_data[TINY_TX_MAX_SIZE];

	TC_PRINT("TWIM RTIO tiny write test\n");

	for (uint8_t len = 1; len <= sizeof(tx_data); len++) {
		memset(fixture.slave_buffer, 0x00, TEST_DATA_SIZE);
		prepare_test_data(tx_data, len);

		twim_rtio_tiny_write(tx_data, len);

		zexpect_mem_equal(tx_data, fixture.slave_buffer, len,
				  "Tiny write of %u bytes does not match received data\n", len);
		validate_slave_buffer_tail(len);
	}
}

ZTEST(twim_rtio_twis_loopback, test_twim_rtio_twis_tiny_reg_write_byte)
{
	uint8_t tx_data[2];
	struct rtio_sqe *sqe;

	TC_PRINT("TWIM RTIO tiny register write byte test\n");

	prepare_test_data(tx_data, sizeof(tx_data));

	sqe = i2c_rtio_copy_reg_write_byte(&twim_rtio_context, &twim_rtio_io_dev, tx_data[0],
					   tx_data[1]);
	zassert_not_null(sqe, "i2c_rtio_copy_reg_write_byte failed\n");

	twim_rtio_submit_and_check(1);

	zexpect_mem_equal(tx_data, fixture.slave_buffer, sizeof(tx_data),
			  "Register write does not match received data\n");
	validate_slave_buffer_tail(sizeof(tx_data));
}

/*
 * Tiny write of the register address followed by a read with repeated start
 * in a single RTIO transaction. The TWIS receives the register address into
 * the first byte of its buffer and sends the whole buffer back.
 */
ZTEST(twim_rtio_twis_loopback, test_twim_rtio_twis_tiny_reg_burst_read)
{
	uint8_t reg_addr;
	struct rtio_sqe *sqe;

	TC_PRINT("TWIM RTIO tiny register burst read test\n");

	prepare_test_data(&reg_addr, sizeof(reg_addr));
	prepare_test_data(fixture.slave_buffer, TEST_DATA_SIZE);

	sqe = i2c_rtio_copy_reg_burst_read(&twim_rtio_context, &twim_rtio_io_dev, reg_addr,
					   fixture.master_buffer, TEST_DATA_SIZE);
	zassert_not_null(sqe, "i2c_rtio_copy_reg_burst_read failed\n");

	twim_rtio_submit_and_check(2);

	zexpect_equal(reg_addr, fixture.slave_buffer[0],
		      "Register address 0x%x not received, got 0x%x\n", reg_addr,
		      fixture.slave_buffer[0]);
	validate_buffers();
}

ZTEST(twim_rtio_twis_loopback, test_i2c_rtio_blocking_configure)
{
	int ret;
	struct i2c_msg msg = {
		.buf = fixture.master_buffer,
		.len = TEST_DATA_SIZE,
		.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
	};

	TC_PRINT("I2C RTIO blocking configure test\n");

	ret = i2c_rtio_configure(&twim_i2c_rtio, TWIM_DEV_CONFIG);
	zassert_ok(ret, "i2c_rtio_configure failed: %d\n", ret);

	prepare_test_data(fixture.master_buffer, TEST_DATA_SIZE);

	ret = i2c_rtio_transfer(&twim_i2c_rtio, &msg, 1, fixture.addr);
	zassert_ok(ret, "i2c_rtio_transfer after configure failed: %d\n", ret);

	validate_buffers();
}

ZTEST(twim_rtio_twis_loopback, test_i2c_rtio_blocking_write)
{
	int ret;
	struct i2c_msg msg = {
		.buf = fixture.master_buffer,
		.len = TEST_DATA_SIZE,
		.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
	};

	TC_PRINT("I2C RTIO blocking write test\n");

	prepare_test_data(fixture.master_buffer, TEST_DATA_SIZE);

	ret = i2c_rtio_transfer(&twim_i2c_rtio, &msg, 1, fixture.addr);
	zassert_ok(ret, "i2c_rtio_transfer failed: %d\n", ret);

	validate_buffers();
}

ZTEST(twim_rtio_twis_loopback, test_i2c_rtio_blocking_read)
{
	int ret;
	struct i2c_msg msg = {
		.buf = fixture.master_buffer,
		.len = TEST_DATA_SIZE,
		.flags = I2C_MSG_READ | I2C_MSG_STOP,
	};

	TC_PRINT("I2C RTIO blocking read test\n");

	prepare_test_data(fixture.slave_buffer, TEST_DATA_SIZE);

	ret = i2c_rtio_transfer(&twim_i2c_rtio, &msg, 1, fixture.addr);
	zassert_ok(ret, "i2c_rtio_transfer failed: %d\n", ret);

	validate_buffers();
}

ZTEST(twim_rtio_twis_loopback, test_i2c_rtio_blocking_write_read)
{
	int ret;
	uint8_t reg[2];
	struct i2c_msg msgs[] = {
		{
			.buf = reg,
			.len = sizeof(reg),
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = fixture.master_buffer,
			.len = TEST_DATA_SIZE,
			.flags = I2C_MSG_READ | I2C_MSG_RESTART | I2C_MSG_STOP,
		},
	};

	TC_PRINT("I2C RTIO blocking write-read test\n");

	prepare_test_data(reg, sizeof(reg));
	prepare_test_data(fixture.slave_buffer, TEST_DATA_SIZE);

	ret = i2c_rtio_transfer(&twim_i2c_rtio, msgs, ARRAY_SIZE(msgs), fixture.addr);
	zassert_ok(ret, "i2c_rtio_transfer failed: %d\n", ret);

	zexpect_mem_equal(reg, fixture.slave_buffer, sizeof(reg),
			  "Written data does not match received data\n");
	validate_buffers();
}

ZTEST(twim_rtio_twis_loopback, test_twim_rtio_twis_nack_write)
{
	static const int expected[] = {-EIO};
	struct rtio_sqe *sqe;

	TC_PRINT("TWIM RTIO write to NACKing address test\n");

	prepare_test_data(fixture.master_buffer, TEST_DATA_SIZE);

	sqe = rtio_sqe_acquire(&twim_rtio_context);
	zassert_not_null(sqe, "rtio_sqe_acquire failed\n");
	rtio_sqe_prep_write(sqe, &twim_rtio_nack_io_dev, RTIO_PRIO_NORM, fixture.master_buffer,
			    TEST_DATA_SIZE, NULL);
	sqe->iodev_flags = RTIO_IODEV_I2C_STOP;

	twim_rtio_submit_and_expect(ARRAY_SIZE(expected), expected);
	validate_slave_buffer_tail(0);

	/* Bus must stay usable after the NACK */
	twim_rtio_write();
	validate_buffers();
}

/*
 * NACK on the first submission of a transaction fails it and cancels
 * the remaining submissions.
 */
ZTEST(twim_rtio_twis_loopback, test_twim_rtio_twis_nack_transaction)
{
	static const int expected[] = {-EIO, -ECANCELED};
	uint8_t reg_addr;
	struct rtio_sqe *sqe;

	TC_PRINT("TWIM RTIO transaction to NACKing address test\n");

	prepare_test_data(&reg_addr, sizeof(reg_addr));

	sqe = i2c_rtio_copy_reg_burst_read(&twim_rtio_context, &twim_rtio_nack_io_dev, reg_addr,
					   fixture.master_buffer, TEST_DATA_SIZE);
	zassert_not_null(sqe, "i2c_rtio_copy_reg_burst_read failed\n");

	twim_rtio_submit_and_expect(ARRAY_SIZE(expected), expected);

	for (size_t i = 0; i < TEST_DATA_SIZE; i++) {
		zexpect_equal(fixture.master_buffer[i], 0xFF,
			      "master_buffer[%zu] (0x%x) modified by canceled read\n", i,
			      fixture.master_buffer[i]);
	}
}

ZTEST(twim_rtio_twis_loopback, test_twim_rtio_twis_unsupported_op)
{
	static const int expected[] = {-EINVAL};
	struct rtio_sqe *sqe;

	TC_PRINT("TWIM RTIO unsupported operation test\n");

	sqe = rtio_sqe_acquire(&twim_rtio_context);
	zassert_not_null(sqe, "rtio_sqe_acquire failed\n");
	rtio_sqe_prep_transceive(sqe, &twim_rtio_io_dev, RTIO_PRIO_NORM, fixture.master_buffer,
				 fixture.master_buffer, TEST_DATA_SIZE, NULL);

	twim_rtio_submit_and_expect(ARRAY_SIZE(expected), expected);

	/* Driver must accept new submissions after rejecting one */
	prepare_test_data(fixture.master_buffer, TEST_DATA_SIZE);
	twim_rtio_write();
	validate_buffers();
}

/*
 * Await blocks the iodev inside a transaction, the write following it
 * must not start before the await submission is completed.
 */
ZTEST(twim_rtio_twis_loopback, test_twim_rtio_twis_await)
{
	int ret;
	struct rtio_sqe *await_sqe;
	struct rtio_sqe *wr_sqe;

	TC_PRINT("TWIM RTIO await test\n");

	prepare_test_data(fixture.master_buffer, TEST_DATA_SIZE);

	await_sqe = rtio_sqe_acquire(&twim_rtio_context);
	zassert_not_null(await_sqe, "rtio_sqe_acquire failed\n");
	rtio_sqe_prep_await_iodev(await_sqe, &twim_rtio_io_dev, RTIO_PRIO_NORM, NULL);
	await_sqe->flags |= RTIO_SQE_TRANSACTION;

	wr_sqe = rtio_sqe_acquire(&twim_rtio_context);
	zassert_not_null(wr_sqe, "rtio_sqe_acquire failed\n");
	rtio_sqe_prep_write(wr_sqe, &twim_rtio_io_dev, RTIO_PRIO_NORM, fixture.master_buffer,
			    TEST_DATA_SIZE, NULL);
	wr_sqe->iodev_flags = RTIO_IODEV_I2C_STOP;

	ret = rtio_submit(&twim_rtio_context, 0);
	zassert_ok(ret, "rtio_submit failed: %d\n", ret);

	k_msleep(10);

	zexpect_is_null(rtio_cqe_consume(&twim_rtio_context),
			"Completion generated before await was completed\n");
	validate_slave_buffer_tail(0);

	rtio_sqe_signal(await_sqe);

	twim_rtio_consume_and_expect(2, NULL);
	validate_buffers();
}

/*
 * Independent submissions queued in one rtio_submit call reach the driver
 * while the previous one is still in progress and are executed in order.
 */
ZTEST(twim_rtio_twis_loopback, test_twim_rtio_twis_queued_ops)
{
	uint8_t tx_data[TEST_DATA_SIZE];
	struct rtio_sqe *wr_sqe;
	struct rtio_sqe *rd_sqe;

	TC_PRINT("TWIM RTIO queued operations test\n");

	prepare_test_data(tx_data, sizeof(tx_data));

	wr_sqe = rtio_sqe_acquire(&twim_rtio_context);
	zassert_not_null(wr_sqe, "rtio_sqe_acquire failed\n");
	rtio_sqe_prep_write(wr_sqe, &twim_rtio_io_dev, RTIO_PRIO_NORM, tx_data, sizeof(tx_data),
			    NULL);
	wr_sqe->iodev_flags = RTIO_IODEV_I2C_STOP;

	rd_sqe = rtio_sqe_acquire(&twim_rtio_context);
	zassert_not_null(rd_sqe, "rtio_sqe_acquire failed\n");
	rtio_sqe_prep_read(rd_sqe, &twim_rtio_io_dev, RTIO_PRIO_NORM, fixture.master_buffer,
			   TEST_DATA_SIZE, NULL);
	rd_sqe->iodev_flags = RTIO_IODEV_I2C_STOP;

	twim_rtio_submit_and_check(2);

	zexpect_mem_equal(tx_data, fixture.slave_buffer, sizeof(tx_data),
			  "Written data does not match received data\n");
	validate_buffers();
}

ZTEST(twim_rtio_twis_loopback, test_twim_rtio_twis_sq_exhausted)
{
	struct rtio_sqe *sqe;

	TC_PRINT("TWIM RTIO submission queue exhausted test\n");

	fill_submission_queue(&twim_rtio_context, TWIM_RTIO_SQ_SIZE);
	sqe = i2c_rtio_copy_reg_write_byte(&twim_rtio_context, &twim_rtio_io_dev, 0x00, 0x00);
	zassert_is_null(sqe, "i2c_rtio_copy_reg_write_byte succeeded with full queue\n");

	/* Fails on the register address submission */
	fill_submission_queue(&twim_rtio_context, TWIM_RTIO_SQ_SIZE);
	sqe = i2c_rtio_copy_reg_burst_read(&twim_rtio_context, &twim_rtio_io_dev, 0x00,
					   fixture.master_buffer, TEST_DATA_SIZE);
	zassert_is_null(sqe, "i2c_rtio_copy_reg_burst_read succeeded with full queue\n");

	/* Fails on the read submission */
	fill_submission_queue(&twim_rtio_context, TWIM_RTIO_SQ_SIZE - 1);
	sqe = i2c_rtio_copy_reg_burst_read(&twim_rtio_context, &twim_rtio_io_dev, 0x00,
					   fixture.master_buffer, TEST_DATA_SIZE);
	zassert_is_null(sqe, "i2c_rtio_copy_reg_burst_read succeeded with one free entry\n");

	/* Failed copies must release all acquired submissions */
	prepare_test_data(fixture.master_buffer, TEST_DATA_SIZE);
	twim_rtio_write();
	validate_buffers();
}

ZTEST(twim_rtio_twis_loopback, test_i2c_rtio_blocking_nack)
{
	int ret;
	struct i2c_msg msg = {
		.buf = fixture.master_buffer,
		.len = TEST_DATA_SIZE,
		.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
	};

	TC_PRINT("I2C RTIO blocking transfer to NACKing address test\n");

	prepare_test_data(fixture.master_buffer, TEST_DATA_SIZE);

	ret = i2c_rtio_transfer(&twim_i2c_rtio, &msg, 1, I2C_NACK_ADDR);
	zassert_equal(ret, -EIO, "i2c_rtio_transfer unexpected result: %d\n", ret);
	validate_slave_buffer_tail(0);

	ret = i2c_rtio_transfer(&twim_i2c_rtio, &msg, 1, fixture.addr);
	zassert_ok(ret, "i2c_rtio_transfer after NACK failed: %d\n", ret);

	validate_buffers();
}

ZTEST(twim_rtio_twis_loopback, test_i2c_rtio_blocking_sq_exhausted)
{
	int ret;
	struct i2c_msg msgs[TWIM_I2C_RTIO_SQ_SIZE + 1];

	TC_PRINT("I2C RTIO blocking API submission queue exhausted test\n");

	for (size_t i = 0; i < ARRAY_SIZE(msgs); i++) {
		msgs[i].buf = fixture.master_buffer;
		msgs[i].len = TEST_DATA_SIZE;
		msgs[i].flags = I2C_MSG_WRITE | I2C_MSG_STOP;
	}

	ret = i2c_rtio_transfer(&twim_i2c_rtio, msgs, ARRAY_SIZE(msgs), fixture.addr);
	zassert_equal(ret, -ENOMEM, "i2c_rtio_transfer unexpected result: %d\n", ret);
	validate_slave_buffer_tail(0);

	fill_submission_queue(twim_i2c_rtio.r, TWIM_I2C_RTIO_SQ_SIZE);
	ret = i2c_rtio_configure(&twim_i2c_rtio, TWIM_DEV_CONFIG);
	rtio_sqe_drop_all(twim_i2c_rtio.r);
	zassert_equal(ret, -ENOMEM, "i2c_rtio_configure unexpected result: %d\n", ret);

	/* Context must be usable once submissions are released */
	ret = i2c_rtio_configure(&twim_i2c_rtio, TWIM_DEV_CONFIG);
	zassert_ok(ret, "i2c_rtio_configure failed: %d\n", ret);

	prepare_test_data(fixture.master_buffer, TEST_DATA_SIZE);
	ret = i2c_rtio_transfer(&twim_i2c_rtio, msgs, 1, fixture.addr);
	zassert_ok(ret, "i2c_rtio_transfer failed: %d\n", ret);

	validate_buffers();
}

ZTEST(twim_rtio_twis_loopback, test_i2c_api_transfer_no_msgs)
{
	int ret;
	struct i2c_msg msg = {
		.buf = fixture.master_buffer,
		.len = TEST_DATA_SIZE,
		.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
	};

	TC_PRINT("I2C API transfer without messages test\n");

	ret = i2c_transfer(DEVICE_DT_GET(NODE_TWIM), &msg, 0, fixture.addr);
	zassert_ok(ret, "i2c_transfer with no messages failed: %d\n", ret);
	validate_slave_buffer_tail(0);
}

ZTEST_SUITE(twim_rtio_twis_loopback, NULL, test_setup, cleanup_buffers, cleanup_buffers, NULL);

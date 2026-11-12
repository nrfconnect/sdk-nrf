/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spim_pm_sck_cs, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#if defined(CONFIG_SPI_RTIO)
#include <zephyr/rtio/rtio.h>
#endif

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(cpuflpr_vpr)),
	     "FLPR core is not enabled, build with the nordic-flpr snippet");

#define SPI_NODE DT_NODELABEL(dut_spi_dt)

#define SPI_CPOL ((CONFIG_TEST_SPI_MODE & 0x2) ? SPI_MODE_CPOL : 0)
#define SPI_CPHA ((CONFIG_TEST_SPI_MODE & 0x1) ? SPI_MODE_CPHA : 0)

#define SPI_OPERATION                                                                              \
	(SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_CPOL | SPI_CPHA)

#define TEST_BYTE 0x66

#define TRANSACTIONS_MAX 2

static uint8_t tx_buffer[TRANSACTIONS_MAX];

#if defined(CONFIG_TEST_SCENARIO_SYNC)

static struct spi_dt_spec spim_spec = SPI_DT_SPEC_GET(SPI_NODE, SPI_OPERATION);

static int scenario_run(void)
{
	struct spi_buf tx_spi_buf = {.buf = &tx_buffer[0], .len = 1};
	struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};

	if (!spi_is_ready_dt(&spim_spec)) {
		LOG_ERR("SPI device is not ready");
		return -ENODEV;
	}

	k_msleep(100);

	return spi_write_dt(&spim_spec, &tx_spi_buf_set);
}

#else /* RTIO scenarios */

#define CQE_TIMEOUT_MS 1000

RTIO_DEFINE(spi_rtio, 4, 4);
SPI_DT_IODEV_DEFINE(spi_iodev, SPI_NODE, SPI_OPERATION);
SPI_DT_IODEV_DEFINE(spi_iodev_hold_cs, SPI_NODE, SPI_OPERATION | SPI_HOLD_ON_CS);

static volatile int callback_err;

static int write_prepare(const struct rtio_iodev *iodev, const uint8_t *buf, uint32_t flags)
{
	struct rtio_sqe *sqe = rtio_sqe_acquire(&spi_rtio);

	if (sqe == NULL) {
		return -ENOMEM;
	}

	rtio_sqe_prep_write(sqe, iodev, RTIO_PRIO_NORM, buf, 1, NULL);
	sqe->flags |= flags;

	return 0;
}

/* Runs in the SPIM completion context of the first transaction. */
static void second_write_submit(struct rtio *r, const struct rtio_sqe *sqe, int res, void *arg0)
{
	ARG_UNUSED(sqe);
	ARG_UNUSED(arg0);

	if (res < 0) {
		callback_err = res;
		return;
	}

	callback_err = write_prepare(&spi_iodev, &tx_buffer[1], 0);
	if (callback_err == 0) {
		rtio_submit(r, 0);
	}
}

static int transactions_submit(void)
{
	struct rtio_sqe *sqe;
	int ret;

	if (IS_ENABLED(CONFIG_TEST_SCENARIO_RTIO_QUEUED)) {
		ret = write_prepare(&spi_iodev, &tx_buffer[0], 0);
		if (ret == 0) {
			ret = write_prepare(&spi_iodev, &tx_buffer[1], 0);
		}
	} else if (IS_ENABLED(CONFIG_TEST_SCENARIO_RTIO_HOLD_ON_CS)) {
		ret = write_prepare(&spi_iodev_hold_cs, &tx_buffer[0], 0);
		if (ret == 0) {
			ret = write_prepare(&spi_iodev, &tx_buffer[1], 0);
		}
	} else {
		ret = write_prepare(&spi_iodev, &tx_buffer[0], RTIO_SQE_CHAINED);
		if (ret == 0) {
			sqe = rtio_sqe_acquire(&spi_rtio);
			if (sqe == NULL) {
				ret = -ENOMEM;
			} else {
				rtio_sqe_prep_callback_no_cqe(sqe, second_write_submit, NULL, NULL);
			}
		}
	}

	if (ret != 0) {
		return ret;
	}

	return rtio_submit(&spi_rtio, 0);
}

static int completions_wait(void)
{
	int64_t timeout = k_uptime_get() + CQE_TIMEOUT_MS;
	uint32_t completed = 0;
	struct rtio_cqe *cqe;
	int ret = 0;

	while (completed < TRANSACTIONS_MAX) {
		cqe = rtio_cqe_consume(&spi_rtio);
		if (cqe == NULL) {
			if (k_uptime_get() > timeout) {
				LOG_ERR("Timeout, %u of %u transactions completed", completed,
					TRANSACTIONS_MAX);
				return -ETIMEDOUT;
			}
			k_msleep(1);
			continue;
		}

		if (cqe->result < 0) {
			LOG_ERR("Transaction %u failed, err: %d", completed, cqe->result);
			ret = cqe->result;
		}

		rtio_cqe_release(&spi_rtio, cqe);
		completed++;
	}

	if (callback_err != 0) {
		LOG_ERR("Callback failed, err: %d", callback_err);
		ret = callback_err;
	}

	return ret;
}

static int scenario_run(void)
{
	int ret;

	k_msleep(100);

	ret = transactions_submit();
	if (ret != 0) {
		LOG_ERR("RTIO submit failed, err: %d", ret);
		return ret;
	}

	return completions_wait();
}

#endif /* CONFIG_TEST_SCENARIO_SYNC */

int main(void)
{
	int ret;

	LOG_INF("SPIM PM SCK CS test on %s, SPI mode: %d", CONFIG_BOARD_TARGET,
		CONFIG_TEST_SPI_MODE);

	for (size_t i = 0; i < ARRAY_SIZE(tx_buffer); i++) {
		tx_buffer[i] = TEST_BYTE;
	}

	ret = scenario_run();
	if (ret != 0) {
		LOG_ERR("Scenario failed, err: %d", ret);
		return ret;
	}

	LOG_INF("Transfers done");

	return 0;
}

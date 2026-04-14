/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/devicetree.h>

#if IS_ENABLED(CONFIG_DT_HAS_JEDEC_MSPI_NOR_ENABLED)
#include <zephyr/drivers/flash.h>
#elif IS_ENABLED(CONFIG_DT_HAS_ZEPHYR_MSPI_EMUL_DEVICE_ENABLED)
#include <zephyr/drivers/mspi.h>
#else
#error "The test requires an enabled MSPI NOR flash memory or an emulated MSPI device"
#endif

#define BUFFER_SIZE	64
#define TIMEOUT		15000000

#if IS_ENABLED(CONFIG_DT_HAS_JEDEC_MSPI_NOR_ENABLED)
#define FLASH_TEST_AREA_DEV_NODE	DT_INST(0, jedec_mspi_nor)
#define FLASH_TEST_AREA_OFFSET		0x0

static const struct device *const flash_dev = DEVICE_DT_GET(FLASH_TEST_AREA_DEV_NODE);
#else
#define MSPI_BUS_NODE DT_NODELABEL(hpf_mspi)
#define EMUL_DEV_NODE DT_INST(0, zephyr_mspi_emul_device)

static const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);
static const struct mspi_dev_id emul_dev_id = {
	.dev_idx = 0xFF,
};
static const struct mspi_dev_cfg emul_dev_cfg = MSPI_DEVICE_CONFIG_DT(EMUL_DEV_NODE);
#endif

static const struct device *const flpr_fault_timer = DEVICE_DT_GET(DT_NODELABEL(fault_timer));
static volatile bool timer_irq;

static void timer_irq_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	timer_irq = true;
}

static void fault_timer_before(void *arg)
{
	ARG_UNUSED(arg);

	int rc;
	const struct counter_top_cfg top_cfg = {
		.callback = timer_irq_handler,
		.user_data = NULL,
		.flags = 0,
		.ticks = counter_us_to_ticks(flpr_fault_timer, CONFIG_MSPI_HPF_FAULT_TIMEOUT)
	};

#if IS_ENABLED(CONFIG_DT_HAS_JEDEC_MSPI_NOR_ENABLED)
	zassert_true(device_is_ready(flash_dev));
#else
	zassert_true(device_is_ready(mspi_bus));
	rc = mspi_dev_config(mspi_bus, &emul_dev_id, MSPI_DEVICE_CONFIG_ALL, &emul_dev_cfg);
	zassert_equal(rc, 0, "Cannot configure an emulated MSPI device");
#endif
	zassert_true(device_is_ready(flpr_fault_timer));
	rc = counter_set_top_value(flpr_fault_timer, &top_cfg);
	zassert_equal(rc, 0, "Cannot set top value");

	timer_irq = false;
}

/**
 * @brief Check if the timer is not triggered when the flash is being read.
 *        When we send commands to the flash the timer should be reset and stopped by FLPR.
 *        If it is not then we will get an IRQ after CONFIG_MSPI_HPF_FAULT_TIMEOUT.
 */
ZTEST(hpf_fault_timer, test_timer_timeout)
{
	int rc;
	uint8_t buf[BUFFER_SIZE] = { 0 };
	volatile uint32_t count = 0;

	/* 1. The timer is started and the flash is read. */
	rc = counter_start(flpr_fault_timer);
	zassert_equal(rc, 0, "Cannot start timer");
#if IS_ENABLED(CONFIG_DT_HAS_JEDEC_MSPI_NOR_ENABLED)
	rc = flash_read(flash_dev, FLASH_TEST_AREA_OFFSET, buf, BUFFER_SIZE);
	zassert_equal(rc, 0, "Cannot read flash");
#else
	struct mspi_xfer_packet packet = {
		.dir = MSPI_TX,
		.cmd = 0x87654321,
		.address = 0x12345678,
		.data_buf = buf,
		.num_bytes = sizeof(buf)
	};
	struct mspi_xfer xfer = {
		.xfer_mode   = MSPI_PIO,
		.packets     = &packet,
		.num_packet  = 1,
		.timeout     = 10,
		.cmd_length  = 4,
		.addr_length = 4
	};

	rc = mspi_transceive(mspi_bus, &emul_dev_id, &xfer);
	zassert_equal(rc, 0, "Cannot write on MSPI bus");
#endif
	while (timer_irq == false && count < TIMEOUT) {
		count++;
	}
	count = 0;

	/* 2. The timer should not trigger an IRQ. */
	zassert_false(timer_irq, "Timer IRQ triggered");

	/* 3. The timer is started again and the flash is not read. */
	rc = counter_start(flpr_fault_timer);
	zassert_equal(rc, 0, "Cannot start timer");

	zassert_false(timer_irq, "Timer IRQ triggered");

	while (timer_irq == false && count < TIMEOUT) {
		count++;
	}

	/* 4. FLPR is not supposed to stop the counter here,
	 * because it does not receive any message.
	 * The timer should trigger an IRQ.
	 */
	zassert_true(timer_irq, "Timer IRQ not triggered");

	rc = counter_stop(flpr_fault_timer);
	zassert_equal(rc, 0, "Cannot stop timer");
}

ZTEST_SUITE(hpf_fault_timer, NULL, NULL, fault_timer_before, NULL, NULL);

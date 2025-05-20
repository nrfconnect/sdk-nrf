/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/devicetree.h>

#define TEST_AREA_DEV_NODE	DT_INST(0, jedec_mspi_nor)
#define TEST_AREA_OFFSET	0x0
#define EXPECTED_SIZE		64
#define TIMEOUT			15000000

static const struct device *const flash_dev = DEVICE_DT_GET(TEST_AREA_DEV_NODE);
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

	zassert_true(device_is_ready(flash_dev));
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
	uint8_t buf[EXPECTED_SIZE];
	volatile uint32_t count;

	/* 1. The timer is started and the flash is read. */
	rc = counter_start(flpr_fault_timer);
	zassert_equal(rc, 0, "Cannot start timer");

	rc = flash_read(flash_dev, TEST_AREA_OFFSET, buf, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot read flash");

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

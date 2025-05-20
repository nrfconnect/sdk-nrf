/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi_emul.h>
#include <zephyr/ztest.h>

#define MSPI_BUS_NODE DT_NODELABEL(hpf_mspi)

static const struct device *mspi_devices[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, DEVICE_DT_GET, (,))
};

static struct mspi_dev_cfg device_cfg[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, MSPI_DEVICE_CONFIG_DT, (,))
};

ZTEST(hpf_trap_handler, test_trap_handler)
{
	int ret = 0;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);
	const struct mspi_dev_id dev_id = {
		.dev_idx = 0xFF,
	};

	zassert_true(device_is_ready(mspi_bus), "mspi_bus is not ready");
	zassert_true(device_is_ready(mspi_devices[0]), "mspi_device is not ready");

	/* Push wrong device id to trigger assert on flpr app side */
	ret = mspi_dev_config(mspi_bus, &dev_id, MSPI_DEVICE_CONFIG_ALL, &device_cfg[0]);
	zassert_equal(ret, -ETIMEDOUT, "Assert not triggered?");
}

ZTEST_SUITE(hpf_trap_handler, NULL, NULL, NULL, NULL, NULL);

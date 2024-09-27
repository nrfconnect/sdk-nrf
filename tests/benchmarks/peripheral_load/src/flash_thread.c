/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_thd, LOG_LEVEL_INF);

#include <zephyr/drivers/flash.h>
#include <zephyr/devicetree.h>
#include "common.h"

#if DT_HAS_COMPAT_STATUS_OKAY(jedec_spi_nor)
#define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(jedec_spi_nor)
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_qspi_nor)
#define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_qspi_nor)
#else
#error Unsupported flash driver
#define FLASH_NODE DT_INVALID_NODE
#endif

#define TEST_AREA_OFFSET	(0xff000)
#define EXPECTED_SIZE		(512)

static const struct device *const flash_dev = DEVICE_DT_GET(FLASH_NODE);

/* Flash thread */
static void flash_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;
	uint8_t buf[EXPECTED_SIZE];

	atomic_inc(&started_threads);

	if (!device_is_ready(flash_dev)) {
		LOG_ERR("Device %s is not ready.", flash_dev->name);
		atomic_inc(&completed_threads);
		return;
	}

	for (int i = 0; i < FLASH_THREAD_COUNT_MAX; i++) {
		/* Read external memory */
		ret = flash_read(flash_dev, TEST_AREA_OFFSET, buf, EXPECTED_SIZE);
		if (ret < 0) {
			LOG_ERR("flash_read() failed (%d)", ret);
			atomic_inc(&completed_threads);
			return;
		}
		LOG_INF("flash_read() #%d done", i);

		k_msleep(FLASH_THREAD_SLEEP);
	}
	LOG_INF("Flash thread has completed");

	atomic_inc(&completed_threads);
}

K_THREAD_DEFINE(thread_flash_id, FLASH_THREAD_STACKSIZE, flash_thread, NULL, NULL, NULL,
	K_PRIO_PREEMPT(FLASH_THREAD_PRIORITY), 0, 0);

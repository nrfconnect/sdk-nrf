/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "flash_test.h"

LOG_MODULE_REGISTER(flash_test, LOG_LEVEL_INF);

static const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(dut_flash));

extern atomic_t started_threads;
static uint8_t test_buffer[MAX_TEST_BUFFER_SIZE];

static int setup(void)
{
	int ret;
	static uint64_t flash_size;
	uint32_t pages_count;
	static size_t page_size;

	ret = device_is_ready(flash_dev);
	if (ret != 1) {
		LOG_ERR("FLASH device not ready: %d", ret);
		return -1;
	}

	flash_get_size(flash_dev, &flash_size);
	pages_count = flash_get_page_count(flash_dev);
	page_size = (size_t)(flash_size / pages_count);

	LOG_INF("Flash size: %llu", flash_size);
	LOG_INF("Pages: %u", pages_count);
	LOG_INF("Minimal write block size: %u", flash_get_write_block_size(flash_dev));
	LOG_INF("Page size: %u", page_size);

	ret = flash_erase(flash_dev, 0, page_size);
	if (ret != 0) {
		LOG_ERR("FLASH erase failed: %d", ret);
		return -2;
	}

	return 0;
}

static void flash_load_thread_worker(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;

	ret = setup();
	if (ret != 0) {
		LOG_ERR("FLASH setup failed: %d\n", ret);
		return;
	}

	atomic_inc(&started_threads);

	LOG_INF("FLASH load thread started");
	while (1) {
		ret = flash_read(flash_dev, 0, test_buffer, sizeof(test_buffer));
		if (ret != 0) {
			LOG_ERR("FLASH read failed: %d", ret);
		}
		k_msleep(FLASH_THREAD_SLEEP);
	}
}

K_THREAD_DEFINE(flash_load_thread, FLASH_THREAD_STACKSIZE, flash_load_thread_worker, NULL, NULL,
		NULL, K_PRIO_PREEMPT(FLASH_THREAD_PRIORITY), 0, 0);

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/counter.h>


#define TEST_DATA_PARTITION	   storage_partition
#define TEST_DATA_PARTITION_OFFSET PARTITION_OFFSET(TEST_DATA_PARTITION)
#define TEST_DATA_PARTITION_DEVICE PARTITION_DEVICE(TEST_DATA_PARTITION)

#define FLASH_PAGE_SIZE 4096
#define FLASH_WORD_SIZE 16
#define FLASH_PAGE_ID	0
#define COUNT_TIME_MS 1000

const struct device *const soc_flash_dev = TEST_DATA_PARTITION_DEVICE;
const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_NODELABEL(tst_timer));

uint8_t test_data_pattern[FLASH_WORD_SIZE] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
					      0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10};

static void counter_callback(const struct device *dev, void *user_data) {

	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	int err;

	counter_stop(dev);
	
	err = flash_write(soc_flash_dev, TEST_DATA_PARTITION_OFFSET, test_data_pattern, FLASH_WORD_SIZE);
	printk("flash_write (inside isr) err = %d\n", err);

}

int main(void) {

	int err;
	const struct counter_top_cfg counter_top_config = {
		.callback = counter_callback,
		.ticks = counter_us_to_ticks(tst_timer_dev, (uint64_t)COUNT_TIME_MS * 1000)
	};

	err = counter_set_top_value(tst_timer_dev, &counter_top_config);
	counter_reset(tst_timer_dev);
	counter_start(tst_timer_dev);

	return 0;
}

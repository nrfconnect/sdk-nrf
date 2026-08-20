/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#if defined(CONFIG_MMC_STACK)
#include <zephyr/sd/mmc.h>
#else
#include <zephyr/sd/sdmmc.h>
#endif
#include <zephyr/drivers/disk.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/debug/cpu_load.h>
#include <zephyr/drivers/gpio.h>

#define MAX_CPU_LOAD_VALUES_HELD	   32
#define TEST_TIMER_COUNT_TIME_LIMIT_MS	   10000
#define DEAD_TIME_MS			   1000
#define CPU_LOAD_MONITOR_THREAD_STACK_SIZE 4096
#define CPU_LOAD_MONITOR_PERIOD_MS	   25

#define MAX_BLOCK_COUNT	     64
#define BLOCK_SIZE	     512
#define MAX_TEST_BUFFER_SIZE (BLOCK_SIZE * MAX_BLOCK_COUNT)
#define SDMMC_UNALIGN_OFFSET 1
#define START_OFFSET	     0U

#define MAX_BUSY_RETRIERS 2

static const struct device *const sdmmc_dev = DEVICE_DT_GET(DT_ALIAS(sdhc0));
const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_NODELABEL(tst_timer));
static const struct gpio_dt_spec ppk_signal = GPIO_DT_SPEC_GET(DT_ALIAS(ppk_signal), gpios);

static struct sd_card card;
static uint8_t test_buffer[MAX_TEST_BUFFER_SIZE] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
static uint32_t block_size;
static uint32_t block_count;

static K_SEM_DEFINE(cpu_load_start_sem, 0, 1);
static K_SEM_DEFINE(cpu_load_stop_sem, 0, 1);
static K_SEM_DEFINE(cpu_load_calc_done_sem, 0, 1);
static K_SEM_DEFINE(cpu_load_thread_terminate_sem, 0, 1);

typedef int (*sdmmc_operation_fn)(struct sd_card *card, uint8_t *buf, uint32_t start_block,
				  uint32_t num_blocks);

static int sdmmc_read_operation(struct sd_card *card, uint8_t *buf, uint32_t start_block,
				uint32_t num_blocks)
{
#if defined(CONFIG_MMC_STACK)
	return mmc_read_blocks(card, buf, start_block, num_blocks);
#else
	return sdmmc_read_blocks(card, buf, start_block, num_blocks);
#endif
}

static int sdmmc_write_operation(struct sd_card *card, uint8_t *buf, uint32_t start_block,
				 uint32_t num_blocks)
{
#if defined(CONFIG_MMC_STACK)
	return mmc_write_blocks(card, (const uint8_t *)buf, start_block, num_blocks);
#else
	return sdmmc_write_blocks(card, (const uint8_t *)buf, start_block, num_blocks);
#endif
}

#if !defined(CONFIG_MMC_STACK)
static int sdmmc_erase_operation(struct sd_card *card, uint8_t *buf, uint32_t start_block,
				 uint32_t num_blocks)
{
	ARG_UNUSED(buf);

	return sdmmc_erase_blocks(card, start_block, num_blocks);
}
#endif

/*
 * Test timer setup
 * for flash operations duration measurement
 */
static void configure_test_timer(const struct device *timer_dev, uint32_t count_time_ms)
{
	struct counter_alarm_cfg counter_cfg;

	counter_cfg.flags = 0;
	counter_cfg.ticks = counter_us_to_ticks(timer_dev, (uint64_t)count_time_ms * 1000);
	counter_cfg.user_data = &counter_cfg;
}

static int test_setup(void)
{
	int ret;

	ret = gpio_is_ready_dt(&ppk_signal);
	if (ret != 1) {
		printk("PPK signaling GPIO is not ready\n");
		return -1;
	}

	ret = gpio_pin_configure_dt(&ppk_signal, GPIO_OUTPUT_ACTIVE);
	if (ret != 0) {
		printk("Failed to configure PPK signaling GPIO\n");
		return -2;
	}

	configure_test_timer(tst_timer_dev, TEST_TIMER_COUNT_TIME_LIMIT_MS);

	if (device_is_ready(sdmmc_dev) != 1) {
		printk("SDCARD/eMMC device not ready\n");
		return -3;
	}

#if !defined(CONFIG_MMC_STACK)
	/* This step depends on the device (SDCARD/eMMC) */
	if (sd_is_card_present(sdmmc_dev) != 1) {
		printk("SDCARD not present in slot\n");
		return -4;
	};
#endif

	ret = sd_init(sdmmc_dev, &card);
	if (ret != 0) {
		printk("SDCARD/eMMC init failed: %d\n", ret);
		return -5;
	}

#if defined(CONFIG_MMC_STACK)
	ret = mmc_ioctl(&card, DISK_IOCTL_GET_SECTOR_COUNT, &block_count);
#else
	ret = sdmmc_ioctl(&card, DISK_IOCTL_GET_SECTOR_COUNT, &block_count);
#endif
	if (ret != 0) {
		printk("IOCTL block count read failed: %d\n", ret);
		return -6;
	}
	printk("SDCARD/eMMC block count: %u\n", block_count);

#if defined(CONFIG_MMC_STACK)
	ret = mmc_ioctl(&card, DISK_IOCTL_GET_SECTOR_SIZE, &block_size);
#else
	ret = sdmmc_ioctl(&card, DISK_IOCTL_GET_SECTOR_SIZE, &block_size);
#endif
	if (ret != 0) {
		printk("IOCTL block size read failed: %d\n", ret);
		return -7;
	}
	printk("SDCARD/eMMC block size: %u\n", block_size);

	k_msleep(DEAD_TIME_MS);
	return 0;
}

static void test_sdmmc_operation(uint32_t operation_size_in_blocks, sdmmc_operation_fn operation,
				 const char *operation_name)
{
	int ret;
	uint32_t tst_timer_value = 0;
	uint64_t timer_value_us = 0;
	uint32_t required_operation_repeats = 1;
	uint32_t operation_block_count = operation_size_in_blocks;
	int32_t cpu_load;

	if (operation_size_in_blocks > MAX_BLOCK_COUNT) {
		/* Cannot be done in one shot due to CPU RAM limitation */
		required_operation_repeats = operation_size_in_blocks / MAX_BLOCK_COUNT;
		operation_block_count = MAX_BLOCK_COUNT;
	}

	printk("SDCARD/eMMC %s test [size: %u blocks]\n", operation_name, operation_size_in_blocks);
	printk("Operation repeats: %u\n", required_operation_repeats);
	printk("Single operation block count: %u\n", operation_block_count);
	memset(test_buffer, 0xAB, MAX_TEST_BUFFER_SIZE);

	cpu_load_get(true);
	gpio_pin_set_dt(&ppk_signal, 1);
	counter_reset(tst_timer_dev);
	counter_start(tst_timer_dev);
	for (uint32_t i = 0; i < required_operation_repeats; i++) {
		ret = operation(&card, test_buffer, START_OFFSET, operation_block_count);
	}
	counter_get_value(tst_timer_dev, &tst_timer_value);
	counter_stop(tst_timer_dev);
	gpio_pin_set_dt(&ppk_signal, 0);
	cpu_load = cpu_load_get(true);

	if (ret != 0) {
		printk("!!!! SDCARD/eMMC %s error: %d !!!!\n", operation_name, ret);
	}

	timer_value_us = counter_ticks_to_us(tst_timer_dev, tst_timer_value);

	printk("### Summary ###\n");
	printk("SDCARD/eMMC %s [size: %u blocks] took: %llu us\n", operation_name,
	       operation_size_in_blocks, timer_value_us);

	if ((ret == 0) && (timer_value_us > 0)) {
		uint64_t rate =
			(uint64_t)operation_size_in_blocks * BLOCK_SIZE * 1000ULL / timer_value_us;

		printk("SDCARD/eMMC %s rate: %llu.%03llu MB/s\n", operation_name, rate / 1000,
		       rate % 1000);
	}

	if (cpu_load > 0) {
		printk("CPU load %d,%d%%\n", cpu_load / 10, cpu_load % 10);
	} else {
		printk("CPU load get failed: %d\n", cpu_load);
	}

	k_msleep(DEAD_TIME_MS);
}

int main(void)
{
	int ret;
	uint32_t test_operation_size[] = {1, 4, 128, 512, 2048, 16384};

	printk("SDCARD/eMMC performance benchmark %s\n", CONFIG_BOARD_TARGET);

	ret = test_setup();
	if (ret) {
		printk("Test setup failed: %d\n", ret);
		return -1;
	}

	for (int i = 0; i < ARRAY_SIZE(test_operation_size); i++) {
		printk("*********************************************\n");
		printk("**** [Step %u] SDCARD/eMMC operation size: %u blocks ****\n", i + 1,
		       test_operation_size[i]);
#if !defined(CONFIG_MMC_STACK)
		test_sdmmc_operation(test_operation_size[i], sdmmc_erase_operation, "erase");
#endif
		test_sdmmc_operation(test_operation_size[i], sdmmc_write_operation, "write");
		test_sdmmc_operation(test_operation_size[i], sdmmc_read_operation, "read");
	}

	printk("Done\n");

	return 0;
}

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
static uint32_t cpu_loads[MAX_CPU_LOAD_VALUES_HELD];
static uint32_t average_cpu_load;
static uint32_t peak_cpu_load;

typedef enum {
	WAIT_FOR_TRIGGER = 0,
	MEASURE_CPU_LOAD = 1,
	CHECK_TERM_SIGNAL = 2
} monitor_state;

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

/*
 * Instead of listing individual values
 * calculate peak and average CPU load
 */
static void calculate_peak_and_average_cpu_load(uint32_t loads_counter, uint32_t *peak_load,
						uint32_t *average_load)
{

	uint64_t average_buffer = 0;
	*peak_load = 0;

	for (int i = 0; i < loads_counter; i++) {
		average_buffer += cpu_loads[i];
		if (cpu_loads[i] > *peak_load) {
			*peak_load = cpu_loads[i];
		}
	}

	*average_load = (uint32_t)(average_buffer / loads_counter);
}

/*
 * Background CPU load minitoring task
 * start - when 'cpu_load_start_sem' is released
 * stop - when 'cpu_load_stop_sem' is released
 * after stop it performs load calculations
 * terminates when 'cpu_load_thread_terminate_sem' is given
 * they are done when 'cpu_load_stop_sem' is released
 */
static void cpu_load_monitor(void *param1, void *param2, void *param3)
{
	int32_t cpu_load;
	static uint32_t cpu_loads_counter;

	static monitor_state cpu_load_monitor_state = WAIT_FOR_TRIGGER;

	while (1) {
		switch (cpu_load_monitor_state) {
		case WAIT_FOR_TRIGGER:
			if (k_sem_take(&cpu_load_start_sem, K_NO_WAIT) == 0) {
				cpu_load_monitor_state = MEASURE_CPU_LOAD;
				peak_cpu_load = 0;
				average_cpu_load = 0;
				cpu_loads_counter = 0;
			}
			if (k_sem_take(&cpu_load_thread_terminate_sem, K_NO_WAIT) == 0) {
				k_sleep(K_FOREVER);
			}
			k_msleep(1);
			break;

		case MEASURE_CPU_LOAD:
			cpu_load = cpu_load_get(true);
			if (cpu_load < 0) {
				/* error */
				cpu_load = 0;
			}
			cpu_loads[cpu_loads_counter] = (uint32_t)cpu_load;
			cpu_loads_counter = (cpu_loads_counter + 1) % MAX_CPU_LOAD_VALUES_HELD;
			k_msleep(CPU_LOAD_MONITOR_PERIOD_MS);
			cpu_load_monitor_state = CHECK_TERM_SIGNAL;

		case CHECK_TERM_SIGNAL:
			if (k_sem_take(&cpu_load_stop_sem, K_NO_WAIT) == 0) {
				cpu_load_monitor_state = WAIT_FOR_TRIGGER;
				calculate_peak_and_average_cpu_load(
					cpu_loads_counter, &peak_cpu_load, &average_cpu_load);
				k_sem_give(&cpu_load_calc_done_sem);
			} else {
				cpu_load_monitor_state = MEASURE_CPU_LOAD;
			}

		default:
			break;
		}
	}
}

/*
 * CPU load monitor thread
 */
K_THREAD_DEFINE(thread_a, CPU_LOAD_MONITOR_THREAD_STACK_SIZE, cpu_load_monitor, NULL, NULL, NULL, 5,
		0, 0);

/*
 * Display peak and average CPU load
 * in mili-percent [m%]
 */
static void show_measured_cpu_loads(void)
{
	k_sem_take(&cpu_load_calc_done_sem, K_FOREVER);
	printk("Meeasured CPU load:\n");
	printk("Peak CPU load: %u [m%%]\n", peak_cpu_load);
	printk("Average CPU load: %u [m%%]\n", average_cpu_load);
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

	if (operation_size_in_blocks > MAX_BLOCK_COUNT) {
		/* Cannot be done in one shot due to CPU RAM limitation */
		required_operation_repeats = operation_size_in_blocks / MAX_BLOCK_COUNT;
		operation_block_count = MAX_BLOCK_COUNT;
	}

	printk("SDCARD/eMMC %s test [size: %u blocks]\n", operation_name, operation_size_in_blocks);
	printk("Operation repeats: %u\n", required_operation_repeats);
	printk("Single operation block count: %u\n", operation_block_count);
	memset(test_buffer, 0xAB, MAX_TEST_BUFFER_SIZE);

	k_sem_give(&cpu_load_start_sem);
	gpio_pin_set_dt(&ppk_signal, 1);
	counter_reset(tst_timer_dev);
	counter_start(tst_timer_dev);
	for (uint32_t i = 0; i < required_operation_repeats; i++) {
		ret = operation(&card, test_buffer, START_OFFSET, operation_block_count);
	}
	counter_get_value(tst_timer_dev, &tst_timer_value);
	counter_stop(tst_timer_dev);
	gpio_pin_set_dt(&ppk_signal, 0);
	k_sem_give(&cpu_load_stop_sem);

	if (ret != 0) {
		printk("!!!! SDCARD/eMMC %s error: %d !!!!\n", operation_name, ret);
	}

	timer_value_us = counter_ticks_to_us(tst_timer_dev, tst_timer_value);

	printk("### Summary ###\n");
	printk("SDCARD/eMMC %s [size: %u blocks] took: %llu us\n", operation_name,
	       operation_size_in_blocks, timer_value_us);
	show_measured_cpu_loads();
	k_msleep(DEAD_TIME_MS);
}

int main(void)
{
	int ret;

	printk("SDCARD/eMMC performance benchmark %s\n", CONFIG_BOARD_TARGET);

	ret = test_setup();
	if (ret) {
		printk("Test setup failed: %d\n", ret);
		return -1;
	}

	/* DRY RUN */
	uint32_t test_operation_size[] = {1, 4, 8, 16, 64, 128, 256, 512};

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

	k_sem_give(&cpu_load_thread_terminate_sem);
	printk("Done\n");

	return 0;
}

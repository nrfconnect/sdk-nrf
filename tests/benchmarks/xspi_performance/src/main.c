/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/debug/cpu_load.h>
#include <dk_buttons_and_leds.h>

#define CPU_LOAD_MONITOR_THREAD_STACK_SIZE 4096
#define CPU_LOAD_MONITOR_PERIOD_MS	   25
#define FLASH_TEST_DATA_OFFSET		   0x0
#define MAX_TEST_BUFFER_SIZE		   80 * 1024
#define MAX_CPU_LOAD_VALUES_HELD	   32
#define TEST_TIMER_COUNT_TIME_LIMIT_MS	   10000
#define DEAD_TIME_MS			   1000

static const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(dut_flash));
const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_NODELABEL(tst_timer));

static uint64_t flash_size;
static size_t pages_count;
static size_t write_block_size;
static size_t page_size;
static uint8_t test_buffer[MAX_TEST_BUFFER_SIZE];
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

/*
 * Flash operation function pointer
 * and auxiliary functions
 * which re passed to the 'test_flash_operation'
 * main test function
 */
typedef int (*flash_operation_fn)(const struct device *dev, off_t offset, void *data, size_t len);

static int flash_read_operation(const struct device *dev, off_t offset, void *data, size_t len)
{
	return flash_read(dev, offset, data, len);
}

static int flash_write_operation(const struct device *dev, off_t offset, void *data, size_t len)
{
	return flash_write(dev, offset, (const void *)data, len);
}

static int flash_erase_operation(const struct device *dev, off_t offset, void *data, size_t len)
{
	ARG_UNUSED(data);

	return flash_erase(dev, offset, len);
}

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
 * CPU load mintor thread
 */
K_THREAD_DEFINE(thread_a, CPU_LOAD_MONITOR_THREAD_STACK_SIZE, cpu_load_monitor, NULL, NULL, NULL, 3,
		0, 0);

/*
 * Check flash meory readiness
 * read memory parameters to be used
 * in the upcoming tests
 */
static int test_setup(void)
{
	int is_flash_ready = 0;

	dk_leds_init();
	configure_test_timer(tst_timer_dev, TEST_TIMER_COUNT_TIME_LIMIT_MS);

	for (int i = 0; i < 3; i++) {
		is_flash_ready = device_is_ready(flash_dev);
		if (is_flash_ready) {
			break;
		}
		k_msleep(DEAD_TIME_MS);
	}

	if (!is_flash_ready) {
		printk("Flash device not ready\n");
		return 1;
	}

	flash_get_size(flash_dev, &flash_size);
	pages_count = flash_get_page_count(flash_dev);
	write_block_size = flash_get_write_block_size(flash_dev);
	page_size = (size_t)(flash_size / pages_count);

	printk("Flash size: %llu\n", flash_size);
	printk("Pages: %u\n", pages_count);
	printk("Minimal write block size: %u\n", write_block_size);
	printk("Page size: %u\n", page_size);

	k_msleep(DEAD_TIME_MS);
	return 0;
}

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

/*
 * General flash operations test function
 * set DK_LED1 to ON state
 * start CPU load monitor
 * start timer
 * perform flahs operation(s) (read, write, erase)
 * get timer value
 * stop timer
 * set DK_LED1 to OFF state
 * stop CPU load monitor
 * calculate operation duration in [us]
 * show measured timing
 * wait for CPU loads caluclations to finish
 * show measured CPU loads
 * sleep for 'DEAD_TIME_MS'
 */
static void test_flash_operation(size_t flash_operation_size, flash_operation_fn flash_operation,
				 const char *operation_name)
{

	int err = 0;
	uint32_t tst_timer_value = 0;
	uint64_t timer_value_us = 0;
	uint32_t required_repetitions = flash_operation_size / page_size;

	printk("Flash %s test [size: %u bytes]\n", operation_name, flash_operation_size);
	memset(test_buffer, 0xAB, MAX_TEST_BUFFER_SIZE);

	k_sem_give(&cpu_load_start_sem);
	dk_set_led_on(DK_LED1);
	counter_reset(tst_timer_dev);
	counter_start(tst_timer_dev);
	if (required_repetitions > 0) {
		/* Cannot be done in one shot due to CPU RAM limitation */
		for (int i = 0; i < required_repetitions; i++) {
			err = flash_operation(flash_dev, FLASH_TEST_DATA_OFFSET + i * page_size,
					       test_buffer, page_size);
		}
	} else {
		err = flash_operation(flash_dev, FLASH_TEST_DATA_OFFSET, test_buffer,
				      flash_operation_size);
	}
	counter_get_value(tst_timer_dev, &tst_timer_value);
	counter_stop(tst_timer_dev);
	dk_set_led_off(DK_LED1);
	k_sem_give(&cpu_load_stop_sem);

	if (err != 0) {
		printk("!!!! Flash operation error: %d !!!!\n", err);
	}

	timer_value_us = counter_ticks_to_us(tst_timer_dev, tst_timer_value);

	printk("### Summary ###\n");
	printk("Flash %s [size: %u bytes] took: %llu us\n", operation_name, flash_operation_size,
	       timer_value_us);
	show_measured_cpu_loads();
	k_msleep(DEAD_TIME_MS);
}

/*
 * Test flash operations with increasing
 * operation byte size
 */
int main(void)
{
	int err;

	printk("xSPI performance benchmark %s\n", CONFIG_BOARD_TARGET);
	k_msleep(DEAD_TIME_MS);

	if (test_setup()) {
		printk("Test setup failed\n");
		return 0;
	}

	if (write_block_size == 0) {
		printk("Flash driver returned minimal sector size equal to 0, setting to 1\n");
		write_block_size = 1;
	}

#if defined(CONFIG_TEST_FIXED_OPERATION_SIZE)
	uint32_t test_operation_size[] = { (size_t)CONFIG_TEST_FLASH_OPERATION_SIZE };
#else
	uint32_t test_operation_size[] = {
		write_block_size, 16, 256, 16384, page_size, page_size * 8, page_size * 32};
#endif

	for (int i = 0; i < ARRAY_SIZE(test_operation_size); i++) {
		printk("*********************************************\n");
		printk("**** [Step %u] flash operation size: %uB ****\n", i + 1,
		       test_operation_size[i]);
		if (test_operation_size[i] >= page_size) {
			test_flash_operation(test_operation_size[i], flash_erase_operation,
					     "erase");
		} else {
			err = flash_erase(flash_dev, FLASH_TEST_DATA_OFFSET, page_size);
			k_msleep(DEAD_TIME_MS);
			if (err != 0) {
				printk("!!!! Flash erase error: %d !!!!\n", err);
			}
		}
		test_flash_operation(test_operation_size[i], flash_write_operation, "write");
		test_flash_operation(test_operation_size[i], flash_read_operation, "read");
	}

	/*
	 * After the measurement are done
	 * CPU shold enter idle state
	 * with low current consumption
	 * Terminate the CPU load monitor thread
	 * to reduce current consumption
	 */
	k_sem_give(&cpu_load_thread_terminate_sem);
	printk("Done\n");

	return 0;
}

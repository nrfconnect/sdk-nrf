/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/counter.h>
#include <dk_buttons_and_leds.h>
#include "cpu_load_monitor.h"

#define FLASH_TEST_DATA_OFFSET		   0x0
#define MAX_TEST_BUFFER_SIZE		   80 * 1024
#define MAX_CPU_LOAD_VALUES_HELD	   32
#define TEST_TIMER_COUNT_TIME_LIMIT_MS	   10000
#define DEAD_TIME_MS			   1000

/* Pattern written by the write test and expected to still be present in
 * flash when the read test runs (write always precedes read for a given
 * operation size). Erased flash is expected to read back as erase_value.
 */
#define TEST_DATA_PATTERN		   0xAB
#define VERIFY_CHUNK_SIZE		   4096

static const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(dut_flash));
const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_NODELABEL(tst_timer));

static uint64_t flash_size;
static size_t pages_count;
static size_t write_block_size;
static size_t page_size;
static uint8_t flash_erase_value;
static uint8_t test_buffer[MAX_TEST_BUFFER_SIZE];
static uint8_t verify_buffer[VERIFY_CHUNK_SIZE];

/*
 * Flash operation kind, used to select the data pattern that is
 * expected to be found in flash once the operation has completed.
 */
typedef enum {
	FLASH_OP_ERASE,
	FLASH_OP_WRITE,
	FLASH_OP_READ,
} flash_op_type;

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
 * Check flash meory readiness
 * read memory parameters to be used
 * in the upcoming tests
 */
static int test_setup(void)
{
	int is_flash_ready = 0;

	dk_leds_init();
	configure_test_timer(tst_timer_dev, TEST_TIMER_COUNT_TIME_LIMIT_MS);

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_monitor_init();
	}

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
	flash_erase_value = flash_get_parameters(flash_dev)->erase_value;

	printk("Flash size: %llu\n", flash_size);
	printk("Pages: %u\n", pages_count);
	printk("Minimal write block size: %u\n", write_block_size);
	printk("Page size: %u\n", page_size);
	printk("Erase value: 0x%02x\n", flash_erase_value);

	k_msleep(DEAD_TIME_MS);
	return 0;
}

/*
 * Check that a buffer matches the expected pattern byte-for-byte.
 * Used to validate data returned by a measured read operation.
 */
static int verify_test_buffer(const uint8_t *buf, size_t len, uint8_t expected_byte,
			      const char *operation_name)
{
	size_t mismatches = 0;
	size_t first_mismatch_index = SIZE_MAX;
	uint8_t first_mismatch_value = 0;

	for (size_t i = 0; i < len; i++) {
		if (buf[i] != expected_byte) {
			if (first_mismatch_index == SIZE_MAX) {
				first_mismatch_index = i;
				first_mismatch_value = buf[i];
			}
			mismatches++;
		}
	}

	if (mismatches > 0) {
		printk("!!!! Data verification FAILED for %s [size: %u bytes]: "
		       "%u mismatching byte(s), first at buffer index %u "
		       "(expected 0x%02x, got 0x%02x) !!!!\n",
		       operation_name, (unsigned)len, (unsigned)mismatches,
		       (unsigned)first_mismatch_index, expected_byte, first_mismatch_value);
		return -EIO;
	}

	printk("Data verification PASSED for %s [size: %u bytes]\n", operation_name,
	       (unsigned)len);
	return 0;
}

/*
 * Read back a flash region in bounded chunks and check that every byte
 * matches the expected pattern. Runs outside of any timed section, so it
 * has no impact on the measured operation rate.
 * Returns 0 if the whole region matches, -EIO otherwise.
 */
static int verify_flash_region(off_t offset, size_t len, uint8_t expected_byte,
				const char *operation_name)
{
	size_t remaining = len;
	off_t cur_offset = offset;
	size_t mismatches = 0;
	off_t first_mismatch_offset = -1;
	uint8_t first_mismatch_value = 0;
	int err;

	while (remaining > 0) {
		size_t chunk = remaining < VERIFY_CHUNK_SIZE ? remaining : VERIFY_CHUNK_SIZE;

		err = flash_read(flash_dev, cur_offset, verify_buffer, chunk);
		if (err != 0) {
			printk("!!!! Verify %s: flash_read error %d at offset 0x%lx !!!!\n",
			       operation_name, err, (long)cur_offset);
			return err;
		}

		for (size_t i = 0; i < chunk; i++) {
			if (verify_buffer[i] != expected_byte) {
				if (first_mismatch_offset < 0) {
					first_mismatch_offset = cur_offset + (off_t)i;
					first_mismatch_value = verify_buffer[i];
				}
				mismatches++;
			}
		}

		cur_offset += (off_t)chunk;
		remaining -= chunk;
	}

	if (mismatches > 0) {
		printk("!!!! Data verification FAILED for %s [size: %u bytes]: "
		       "%u mismatching byte(s), first at offset 0x%llx "
		       "(expected 0x%02x, got 0x%02x) !!!!\n",
		       operation_name, (unsigned)len, (unsigned)mismatches,
		       (unsigned long long)first_mismatch_offset, expected_byte, first_mismatch_value);
		return -EIO;
	}

	printk("Data verification PASSED for %s [size: %u bytes]\n", operation_name,
	       (unsigned)len);
	return 0;
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
 * show measured timing and the rate it gives
 * wait for CPU loads caluclations to finish
 * show measured CPU loads
 * verify flash content against the expected pattern (untimed)
 * sleep for 'DEAD_TIME_MS'
 */
static void test_flash_operation(size_t flash_operation_size, flash_operation_fn flash_operation,
				 const char *operation_name, flash_op_type op_type)
{

	int err = 0;
	uint32_t tst_timer_value = 0;
	uint64_t timer_value_us = 0;
	uint32_t required_repetitions = flash_operation_size / page_size;

	printk("Flash %s test [size: %u bytes]\n", operation_name, flash_operation_size);
	memset(test_buffer, TEST_DATA_PATTERN, MAX_TEST_BUFFER_SIZE);

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_monitor_start();
	}
	dk_set_led_on(DK_LED1);
	counter_reset(tst_timer_dev);
	counter_start(tst_timer_dev);
	if (required_repetitions > 0) {
		/* Cannot be done in one shot due to CPU RAM limitation */
		for (int i = 0; i < required_repetitions; i++) {
			off_t flash_offset = FLASH_TEST_DATA_OFFSET + (off_t)i * page_size;
			void *data_buf = test_buffer;

			/* Reads reuse test_buffer per page; advance the destination so the
			 * full measured result can be verified afterwards.
			 */
			if (op_type == FLASH_OP_READ) {
				data_buf = test_buffer + i * page_size;
			}

			err = flash_operation(flash_dev, flash_offset, data_buf, page_size);
			if (err != 0) {
				break;
			}
		}
	} else {
		err = flash_operation(flash_dev, FLASH_TEST_DATA_OFFSET, test_buffer,
				      flash_operation_size);
	}
	counter_get_value(tst_timer_dev, &tst_timer_value);
	counter_stop(tst_timer_dev);
	dk_set_led_off(DK_LED1);
	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_monitor_stop();
	}

	if (err != 0) {
		printk("!!!! Flash operation error: %d !!!!\n", err);
	}

	timer_value_us = counter_ticks_to_us(tst_timer_dev, tst_timer_value);

	printk("### Summary ###\n");
	printk("Flash %s [size: %u bytes] took: %llu us\n", operation_name, flash_operation_size,
	       timer_value_us);
	if ((err == 0) && (timer_value_us > 0)) {
		uint64_t rate = (uint64_t)flash_operation_size * 1000ULL / timer_value_us;

		printk("Flash %s rate: %llu.%03llu MB/s\n", operation_name, rate / 1000,
		       rate % 1000);
	}
	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_monitor_show();
	}

	if (IS_ENABLED(CONFIG_TEST_DATA_VERIFICATION) && (err == 0)) {
		uint8_t expected_pattern =
			(op_type == FLASH_OP_ERASE) ? flash_erase_value : TEST_DATA_PATTERN;

		if (op_type == FLASH_OP_READ) {
			verify_test_buffer(test_buffer, flash_operation_size, expected_pattern,
					   operation_name);
		} else {
			verify_flash_region(FLASH_TEST_DATA_OFFSET, flash_operation_size,
					    expected_pattern, operation_name);
		}
	}

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
					     "erase", FLASH_OP_ERASE);
		} else {
			err = flash_erase(flash_dev, FLASH_TEST_DATA_OFFSET, page_size);
			k_msleep(DEAD_TIME_MS);
			if (err != 0) {
				printk("!!!! Flash erase error: %d !!!!\n", err);
			} else if (IS_ENABLED(CONFIG_TEST_DATA_VERIFICATION)) {
				verify_flash_region(FLASH_TEST_DATA_OFFSET, page_size,
						     flash_erase_value, "erase");
			}
		}
		test_flash_operation(test_operation_size[i], flash_write_operation, "write",
				     FLASH_OP_WRITE);
		test_flash_operation(test_operation_size[i], flash_read_operation, "read",
				     FLASH_OP_READ);
	}

	/*
	 * After the measurement are done
	 * CPU shold enter idle state
	 * with low current consumption
	 * Terminate the CPU load monitor thread
	 * to reduce current consumption
	 */
	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_monitor_terminate();
	}
	printk("Done\n");

	return 0;
}

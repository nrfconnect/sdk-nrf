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
#define TEST_DATA_PATTERN		   0xAB
#define VERIFY_CHUNK_SIZE		   4096

static const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(dut_flash));
const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_NODELABEL(tst_timer));

static uint64_t flash_size;
static size_t pages_count;
static size_t write_block_size;
static size_t page_size;
static uint8_t flash_erase_value;
/* Word alignment keeps driver that pass buffers by reference (e.g. the HPF
 * MSPI zero-copy path) on its fast path instead of falling back to a copy path.
 */
static uint8_t test_buffer[MAX_TEST_BUFFER_SIZE] __aligned(sizeof(uint32_t));
static uint8_t verify_buffer[VERIFY_CHUNK_SIZE] __aligned(sizeof(uint32_t));

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

static void measure_start(void)
{
	dk_set_led_on(DK_LED1);
	counter_reset(tst_timer_dev);
	counter_start(tst_timer_dev);
}

static uint32_t measure_finish(void)
{
	uint32_t tst_timer_value;

	counter_get_value(tst_timer_dev, &tst_timer_value);
	counter_stop(tst_timer_dev);
	dk_set_led_off(DK_LED1);

	return tst_timer_value;
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
 * Data verification state, accumulated over the chunks of one operation.
 */
struct verify_result {
	size_t checked;
	size_t mismatches;
	size_t first_mismatch;
	uint8_t first_mismatch_value;
};

static void verify_chunk(struct verify_result *res, const uint8_t *buf, size_t len,
			 uint8_t expected_byte)
{
	for (size_t i = 0; i < len; i++) {
		if (buf[i] != expected_byte) {
			if (res->mismatches == 0) {
				res->first_mismatch = res->checked + i;
				res->first_mismatch_value = buf[i];
			}
			res->mismatches++;
		}
	}
	res->checked += len;
}

static int verify_report(const struct verify_result *res, uint8_t expected_byte,
			 const char *operation_name)
{
	if (res->mismatches > 0) {
		printk("!!!! Data verification FAILED for %s [size: %u bytes]: "
		       "%u mismatching byte(s), first at byte %u "
		       "(expected 0x%02x, got 0x%02x) !!!!\n",
		       operation_name, (unsigned int)res->checked,
		       (unsigned int)res->mismatches, (unsigned int)res->first_mismatch,
		       expected_byte, res->first_mismatch_value);
		return -EIO;
	}

	printk("Data verification PASSED for %s [size: %u bytes]\n", operation_name,
	       (unsigned int)res->checked);
	return 0;
}

/*
 * Read back a flash region in bounded chunks and check that region content
 * matches the expected pattern.
 * Returns 0 if the whole region matches, a negative error code otherwise.
 */
static int verify_flash_region(off_t offset, size_t len, uint8_t expected_byte,
				const char *operation_name)
{
	struct verify_result res = {0};
	int err;

	while (res.checked < len) {
		size_t chunk = MIN(len - res.checked, VERIFY_CHUNK_SIZE);
		off_t cur_offset = offset + (off_t)res.checked;

		err = flash_read(flash_dev, cur_offset, verify_buffer, chunk);
		if (err != 0) {
			printk("!!!! Verify %s: flash_read error %d at offset 0x%llx !!!!\n",
			       operation_name, err, (unsigned long long)cur_offset);
			return err;
		}

		verify_chunk(&res, verify_buffer, chunk, expected_byte);
	}

	return verify_report(&res, expected_byte, operation_name);
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
	uint64_t timer_value_us = 0;
	size_t remaining = flash_operation_size;
	off_t flash_offset = FLASH_TEST_DATA_OFFSET;
	struct verify_result read_res = {0};

	printk("Flash %s test [size: %u bytes]\n", operation_name, flash_operation_size);
	memset(test_buffer, TEST_DATA_PATTERN, MAX_TEST_BUFFER_SIZE);

	if (page_size > MAX_TEST_BUFFER_SIZE) {
		printk("!!!! Page size %u exceeds test buffer %u !!!!\n", (unsigned int)page_size,
		       MAX_TEST_BUFFER_SIZE);
		return;
	}

	/* Partial-page erase is not supported, so erase operation ends in the region aligned
	 * to the page boundary.
	 */
	if (op_type == FLASH_OP_ERASE) {
		if ((FLASH_TEST_DATA_OFFSET % page_size) != 0) {
			printk("!!!! Erase offset 0x%lx is not page aligned [page size: %u] !!!!\n",
			       (unsigned long)FLASH_TEST_DATA_OFFSET, (unsigned int)page_size);
			return;
		}

		if ((flash_operation_size % page_size) != 0) {
			printk("!!!! Erase size %u is not a multiple of page size %u !!!!\n",
			       (unsigned int)flash_operation_size, (unsigned int)page_size);
			return;
		}
	}

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_monitor_start();
	}

	/* Chunked by page: the whole region cannot be held in RAM at once. */
	while (remaining > 0) {
		size_t chunk = MIN(remaining, page_size);

		measure_start();
		err = flash_operation(flash_dev, flash_offset, test_buffer, chunk);
		timer_value_us += counter_ticks_to_us(tst_timer_dev, measure_finish());

		if (err != 0) {
			break;
		}
		if (IS_ENABLED(CONFIG_TEST_DATA_VERIFICATION) && (op_type == FLASH_OP_READ)) {
			verify_chunk(&read_res, test_buffer, chunk, TEST_DATA_PATTERN);
		}
		flash_offset += (off_t)chunk;
		remaining -= chunk;
	}

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_monitor_stop();
	}

	if (IS_ENABLED(CONFIG_TEST_DATA_VERIFICATION) && (op_type == FLASH_OP_READ) &&
	    (err == 0)) {
		err = verify_report(&read_res, TEST_DATA_PATTERN, operation_name);
	}

	if (err != 0) {
		printk("!!!! Flash operation error: %d !!!!\n", err);
	}

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

		if (op_type != FLASH_OP_READ) {
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

/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#define TEST_DATA_PARTITION	   storage_partition
#define TEST_DATA_PARTITION_OFFSET FIXED_PARTITION_OFFSET(TEST_DATA_PARTITION)
#define TEST_DATA_PARTITION_DEVICE FIXED_PARTITION_DEVICE(TEST_DATA_PARTITION)

#define FLASH_PAGE_SIZE 4096
#define FLASH_WORD_SIZE 16
#define FLASH_PAGE_ID	0

#define SLEEP_TIME_MS	50
#define MAX_REPETITIONS 10

const struct device *const soc_flash_dev = TEST_DATA_PARTITION_DEVICE;

#if defined(CONFIG_SOC_NRF54H20_CPUAPP)
uint8_t test_data_pattern[FLASH_WORD_SIZE] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
					      0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10};
#else
uint8_t test_data_pattern[FLASH_WORD_SIZE] = {0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
					      0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0x20};
#endif

static int soc_flash_setup(const struct device *flash_dev)
{
	if (!device_is_ready(flash_dev)) {
		printk("Internal storage device not ready\n");
		return -1;
	}

	return 0;
}

static int write_soc_flash(const struct device *flash_dev, uint32_t offset, uint8_t *buffer)
{
	printk("Write SoC flash, base address: %x, offset: %x\n", TEST_DATA_PARTITION_OFFSET,
	       offset * FLASH_WORD_SIZE);
	return flash_write(flash_dev, TEST_DATA_PARTITION_OFFSET + offset * FLASH_WORD_SIZE, buffer,
			   FLASH_WORD_SIZE);
}

static int read_soc_flash(const struct device *flash_dev, uint32_t offset, uint8_t *buffer)
{
	printk("Read SoC flash, base address: %x, offset: %x\n", TEST_DATA_PARTITION_OFFSET,
	       offset * FLASH_WORD_SIZE);
	return flash_read(flash_dev, TEST_DATA_PARTITION_OFFSET + offset * FLASH_WORD_SIZE, buffer,
			  FLASH_WORD_SIZE);
}

int main(void)
{
	int err;
	int repetition_counter = MAX_REPETITIONS;
#if defined(CONFIG_SOC_NRF54H20_CPUAPP)
	uint32_t offset = 0;
#else
	uint32_t offset = FLASH_WORD_SIZE * MAX_REPETITIONS;
#endif
	uint8_t test_data_buffer[FLASH_WORD_SIZE];

	printk("Hello World! %s\n", CONFIG_BOARD_TARGET);

	if (soc_flash_setup(soc_flash_dev) != 0) {
		return -1;
	}

	while (repetition_counter--) {
		err = write_soc_flash(soc_flash_dev, offset, test_data_pattern);
		if (err != 0) {
			printk("Writing to SoC flash failed: %d\n", err);
			return -1;
		}

		err = read_soc_flash(soc_flash_dev, offset, test_data_buffer);
		if (err != 0) {
			printk("Reading from SoC flash failed: %d\n", err);
			return -1;
		}

		if (memcmp(test_data_buffer, test_data_pattern, FLASH_WORD_SIZE)) {
			printk("Data read does not match data written\n");
			for (size_t i = 0; i < FLASH_WORD_SIZE; ++i) {
				printk("[%d] got: 0x%x != 0x%x\n", i, test_data_buffer[i],
				       test_data_pattern[i]);
			}
			return -1;
		}

		printk("Data read matches data written\n");

		k_msleep(SLEEP_TIME_MS);
		offset += FLASH_WORD_SIZE;
	}

	printk("Tests status: PASS\n");

	return 0;
}

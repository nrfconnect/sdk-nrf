/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>

#define TEST_AREA storage_partition

/* TEST_AREA is only defined for configurations that rely on
 * fixed-partition nodes.
 */
#if DT_FIXED_PARTITION_EXISTS(DT_NODELABEL(TEST_AREA))
#define TEST_AREA_OFFSET DT_REG_ADDR(DT_NODELABEL(TEST_AREA))
#define TEST_AREA_SIZE   DT_REG_SIZE(DT_NODELABEL(TEST_AREA))
#define TEST_AREA_END    (TEST_AREA_OFFSET + TEST_AREA_SIZE)
#define TEST_AREA_DEVICE DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(DT_NODELABEL(TEST_AREA)))
#else
#error "Unsupported configuration"
#endif

#define BUF_SIZE        512

#if !defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE) && !defined(CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE)
#error There is no flash device enabled or it is missing Kconfig options
#endif

static const struct device *const flash_dev = TEST_AREA_DEVICE;
static const uint8_t WEAR_LIMIT = 20;

static uint8_t __aligned(4) data[BUF_SIZE];
uint8_t read_buf[BUF_SIZE];

/* seed is used to generate pseudo random data. */
static uint8_t seed;

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* Each time data[] is populated, it has different contents */
void generate_data(void)
{
	for (int i = 0; i < BUF_SIZE; i++) {
		data[i] = i + seed;
	}
	seed++;
}

/* Write TEST_AREA in BUF_SIZE chunks. */
void write_to_test_area(void)
{
	int rc;
	int64_t start, delta;

	gpio_pin_set_dt(&led, 1);
	start = k_uptime_get();
	for (uint32_t write_offset = TEST_AREA_OFFSET;
		write_offset + BUF_SIZE < TEST_AREA_END;
		write_offset += BUF_SIZE) {

		rc = flash_write(flash_dev, write_offset, data, BUF_SIZE);
		__ASSERT(rc == 0, "flash_write() failed (%d)", rc);
	}
	delta = k_uptime_delta(&start);
	gpio_pin_set_dt(&led, 0);
	printk("Write took %llu ms\n", delta);
}

/* Read TEST_AREA and verify data. */
void read_and_verify_test_area(void)
{
	int rc;

	for (uint32_t read_offset = TEST_AREA_OFFSET;
		read_offset + BUF_SIZE < TEST_AREA_END;
		read_offset += BUF_SIZE) {

		rc = flash_read(flash_dev, read_offset, read_buf, BUF_SIZE);
		__ASSERT(rc == 0, "flash_read() failed (%d)", rc);

		for (uint32_t i = 0; i < BUF_SIZE; i++) {
			__ASSERT(data[i] == read_buf[i],
				"%u: data = %u while read_buf = %u",
				i, data[i], read_buf[i]);
		}
	}
}

int main(void)
{
	int rc;

	printk("RRAM write test on %s\n", CONFIG_BOARD_TARGET);
	printk("Tested device %s\n", flash_dev->name);
	printk("TEST_AREA_OFFSET = 0x%lx\n", (unsigned long)TEST_AREA_OFFSET);
	printk("TEST_AREA_SIZE = 0x%lx\n", (unsigned long)TEST_AREA_SIZE);
	printk("BUF_SIZE = %u\n", BUF_SIZE);
#if defined(CONFIG_SOC_FLASH_NRF_THROTTLING)
	printk("Throttling is enabled\n");
#else
	printk("Throttling is disabled\n");
#endif

	__ASSERT(device_is_ready(flash_dev), "Flash device is not ready");

	rc = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	__ASSERT(rc == 0, "gpio_pin_configure_dt() failed (%d)", rc);

	/* Sleep for CI synchronization puropse. */
	k_msleep(200);

	for (uint32_t iteration = 0; iteration < WEAR_LIMIT; iteration++) {

		k_msleep(200);
		generate_data();

		k_msleep(500);
		write_to_test_area();

		k_msleep(300);
		read_and_verify_test_area();
	}

	printk("Test completed\n");
}

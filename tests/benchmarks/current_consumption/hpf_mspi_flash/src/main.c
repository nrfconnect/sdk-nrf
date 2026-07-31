/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mspi.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hpf_mspi_flash, LOG_LEVEL_INF);

#define MSPI_CONTROLLER    DT_NODELABEL(hpf_mspi)
#define TEST_REGION_START  0x0
#define FLASH_SECTOR_SIZE  4096
#define BUFFER_SIZE        1024
#define WRITE_WEAR_LIMIT   8

#define SLEEP_TIME_MS 1000
#define ACTIVE_TIME_MS 1000

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

static const struct device *mspi_devices[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_CONTROLLER, DEVICE_DT_GET, (,))
};
BUILD_ASSERT(ARRAY_SIZE(mspi_devices) > 0, "No MSPI devices found");

#if CONFIG_DCACHE
static uint8_t write_buf[BUFFER_SIZE]__aligned(CONFIG_DCACHE_LINE_SIZE);
static uint8_t read_buf[BUFFER_SIZE]__aligned(CONFIG_DCACHE_LINE_SIZE);
#else
static uint8_t write_buf[BUFFER_SIZE];
static uint8_t read_buf[BUFFER_SIZE];
#endif

/* Count how many times CPU is active for ACTIVE_TIME_MS.
 * Erase/write momory only if value is lower than WRITE_WEAR_LIMIT.
 */
static uint32_t active_state_count;

K_SEM_DEFINE(timer_expired_sem, 0, 1);

static void timer_handler(struct k_timer *dummy)
{
	(void)dummy;

	active_state_count++;
	k_sem_give(&timer_expired_sem);
}

K_TIMER_DEFINE(timer, timer_handler, NULL);

static void fill_write_buf(uint8_t *buff, uint32_t len, uint8_t data_offset)
{
	for (uint32_t i = 0; i < len; i++) {
		buff[i] = (uint8_t) i + data_offset;
	}
}

int main(void)
{
	int ret;
	const struct device *flash_dev = mspi_devices[0];

	LOG_INF("hpf_mspi_flash test on %s", CONFIG_BOARD_TARGET);
	LOG_INF("Testing device %s", flash_dev->name);

	ret = device_is_ready(flash_dev);
	__ASSERT(ret, "%s is not ready", flash_dev->name);

	ret = gpio_is_ready_dt(&led);
	__ASSERT(ret == 1, "GPIO device not ready\n");

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	__ASSERT(ret == 0, "gpio_pin_configure_dt return code: %d\n", ret);

	active_state_count = 1;

	while (1) {
		k_timer_start(&timer, K_MSEC(ACTIVE_TIME_MS), K_NO_WAIT);
		gpio_pin_set_dt(&led, 1);

		if (active_state_count <= WRITE_WEAR_LIMIT) {
			fill_write_buf(write_buf, BUFFER_SIZE, active_state_count);

			/* Erase memory and Verify. */
			ret = flash_erase(flash_dev, TEST_REGION_START, FLASH_SECTOR_SIZE);
			__ASSERT(ret == 0, "Flash erase failed! %d", ret);

			ret = flash_read(flash_dev, TEST_REGION_START, read_buf, BUFFER_SIZE);
			__ASSERT(ret == 0, "Flash read failed! %d", ret);

			for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
				__ASSERT(read_buf[i] == 0xff, "Unexpected value %x at %x",
					read_buf[i], i);
			}

			/* Write memory. */
			ret = flash_write(flash_dev, TEST_REGION_START, write_buf, BUFFER_SIZE);
			__ASSERT(ret == 0, "Flash write failed! %d", ret);
			LOG_INF("Memory written %u times", active_state_count);
		}

		/* For the rest of active time do memory read and validation. */
		while (k_sem_take(&timer_expired_sem, K_NO_WAIT) != 0) {
			ret = flash_read(flash_dev, TEST_REGION_START, read_buf, BUFFER_SIZE);
			__ASSERT(ret == 0, "Flash read failed! %d", ret);

			for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
				__ASSERT(read_buf[i] == write_buf[i], "%x: written %x, read %x",
					i, write_buf[i], read_buf[i]);
			}
		}

		gpio_pin_set_dt(&led, 0);
		k_msleep(SLEEP_TIME_MS);
	}
}

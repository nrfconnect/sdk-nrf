/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <bootutil/bootutil_public.h>
#include <bootutil/boot_request.h>

#include <dk_buttons_and_leds.h>

LOG_MODULE_DECLARE(ab_sample);

#define ACTIVE_IMAGE 0

#define CODE_PARTITION DT_CHOSEN(zephyr_code_partition)
#define CODE_PARTITION_OFFSET FIXED_PARTITION_NODE_OFFSET(CODE_PARTITION)

#define SLOT_A_PARTITION cpuapp_slot0_partition
#define SLOT_B_PARTITION cpuapp_slot1_partition
#define CPURAD_SLOT_A_PARTITION cpurad_slot0_partition
#define CPURAD_SLOT_B_PARTITION cpurad_slot1_partition

#define SLOT_A_OFFSET FIXED_PARTITION_OFFSET(SLOT_A_PARTITION)
#define SLOT_B_OFFSET FIXED_PARTITION_OFFSET(SLOT_B_PARTITION)

#define SLOT_A_FLASH_AREA_ID FIXED_PARTITION_ID(SLOT_A_PARTITION)
#define SLOT_B_FLASH_AREA_ID FIXED_PARTITION_ID(SLOT_B_PARTITION)
#define CPURAD_SLOT_A_FLASH_AREA_ID FIXED_PARTITION_ID(CPURAD_SLOT_A_PARTITION)
#define CPURAD_SLOT_B_FLASH_AREA_ID FIXED_PARTITION_ID(CPURAD_SLOT_B_PARTITION)

#define IS_SLOT_A (CODE_PARTITION_OFFSET == SLOT_A_OFFSET)
#define IS_SLOT_B (CODE_PARTITION_OFFSET == SLOT_B_OFFSET)

#define STATUS_LEDS_THREAD_STACK_SIZE 512
#define STATUS_LEDS_THREAD_PRIORITY (CONFIG_NUM_PREEMPT_PRIORITIES - 1)
K_THREAD_STACK_DEFINE(status_leds_thread_stack_area, STATUS_LEDS_THREAD_STACK_SIZE);

enum ab_boot_slot {
	SLOT_A = 0,
	SLOT_B = 1,
	SLOT_INVALID,
};

/** @brief Radio firmware self test
 *
 * @details
 * End-device specific self test should be implemented here.
 */
static bool radio_domain_healthy(void)
{
	return bt_is_ready();
}

/** @brief Application firmware self test
 *
 * @details
 * End-device specific self test should be implemented here. Enabling
 * CONFIG_EMULATE_APP_HEALTH_CHECK_FAILURE allows to emulate a faulty
 * firmware, unable to confirm its health, and ultimately to test
 * a rollback to previous firmware after the update.
 */
static bool app_domain_healthy(void)
{
	if (IS_ENABLED(CONFIG_EMULATE_APP_HEALTH_CHECK_FAILURE)) {
		return false;
	}

	return true;
}

static enum ab_boot_slot active_boot_slot_get(void)
{
	enum ab_boot_slot active_slot = SLOT_INVALID;

	if (IS_SLOT_A) {
		active_slot = SLOT_A;
	} else if (IS_SLOT_B) {
		active_slot = SLOT_B;
	} else {
		LOG_ERR("Cannot determine current slot");
	}

	return active_slot;
}

static void device_healthcheck(void)
{
	int err;
	char *img_set = NULL;
	const struct flash_area *fa;
	int area_id = -1;
	int cpurad_area_id = -1;
	enum ab_boot_slot active_slot = active_boot_slot_get();

	if (active_slot == SLOT_INVALID) {
		return;
	}

	/* Confirming only in non-degraded boot states
	 */
	if (active_slot == SLOT_A) {
		img_set = "A";
		area_id = SLOT_A_FLASH_AREA_ID;
		cpurad_area_id = CPURAD_SLOT_A_FLASH_AREA_ID;
	} else if (active_slot == SLOT_B) {
		img_set = "B";
		area_id = SLOT_B_FLASH_AREA_ID;
		cpurad_area_id = CPURAD_SLOT_B_FLASH_AREA_ID;
	}

	LOG_INF("Testing image set %s...", img_set);

	bool healthy = true;

	if (!radio_domain_healthy()) {
		LOG_ERR("Radio domain is NOT healthy");
		healthy = false;
	}

	if (!app_domain_healthy()) {
		LOG_ERR("App domain is NOT healthy");
		healthy = false;
	}

	if (!healthy) {
		LOG_ERR("Reboot the device to try to boot from previous firmware");
		return;
	}

	LOG_INF("Confirming...");

	if (flash_area_open(area_id, &fa) != 0) {
		LOG_ERR("Cannot open flash area for application slot %s", img_set);
		return;
	}

	err = boot_set_next(fa, true, true);

	flash_area_close(fa);
	if (err == 0) {
		LOG_INF("Application confirmed\n");
	} else {
		LOG_ERR("Failed to confirm application, err: %d", err);
	}

	if (flash_area_open(cpurad_area_id, &fa) != 0) {
		LOG_ERR("Cannot open flash area for radio slot %s", img_set);
		return;
	}

	err = boot_set_next(fa, true, true);

	flash_area_close(fa);
	if (err == 0) {
		LOG_INF("Radio confirmed\n");
	} else {
		LOG_ERR("Failed to confirm radio, err: %d", err);
	}
}

static void toggle_slot_for_single_boot(void)
{
	int err = 0;
	enum ab_boot_slot active_slot = active_boot_slot_get();
	enum boot_slot new_slot = BOOT_SLOT_NONE;

	if (active_slot == SLOT_A) {
		LOG_INF("Temporarily switching slots (A -> B)");
		new_slot = BOOT_SLOT_SECONDARY;
	} else if (active_slot == SLOT_B) {
		LOG_INF("Temporarily switching slots (B -> A)");
		new_slot = BOOT_SLOT_PRIMARY;
	} else {
		LOG_ERR("Cannot determine active slot, cannot toggle");
		return;
	}

	err = boot_request_set_preferred_slot(ACTIVE_IMAGE, new_slot);

	if (err == 0) {
		LOG_INF("Slot toggled, restart the device to enforce");
	} else {
		LOG_ERR("Failed to toggle slots, err: %d", err);
	}
}

static void boot_state_report(void)
{
	enum ab_boot_slot active_slot = active_boot_slot_get();

	if (active_slot == SLOT_A) {
		LOG_INF("Booted from slot A");
	} else if (active_slot == SLOT_B) {
		LOG_INF("Booted from slot B");
	} else {
		LOG_INF("Cannot determine active slot");
	}
}

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	if ((has_changed & DK_BTN1_MSK) && (button_state & DK_BTN1_MSK)) {
		toggle_slot_for_single_boot();
	}
}

struct k_thread status_leds_thread_data;

static void status_leds_thread_entry_point(void *p1, void *p2, void *p3)
{
	int blinking_led = DK_LED1;
	enum ab_boot_slot active_slot = active_boot_slot_get();

	if (active_slot == SLOT_A) {
		blinking_led = DK_LED1;
	} else if (active_slot == SLOT_B) {
		blinking_led = DK_LED2;
	} else {
		return;
	}

	while (1) {
		for (int i = 0; i < CONFIG_N_BLINKS; i++) {
			dk_set_led_off(blinking_led);
			k_msleep(250);
			dk_set_led_on(blinking_led);
			k_msleep(250);
		}

		k_msleep(5000);
	}
}

void ab_actions_perform(void)
{
	int ret;

	boot_state_report();

	ret = dk_leds_init();
	if (ret) {
		LOG_ERR("Cannot init LEDs (err: %d)", ret);
	}

	ret = dk_buttons_init(button_handler);
	if (ret) {
		LOG_ERR("Cannot init buttons (err: %d)", ret);
	}

	k_thread_create(&status_leds_thread_data, status_leds_thread_stack_area,
			K_THREAD_STACK_SIZEOF(status_leds_thread_stack_area),
			status_leds_thread_entry_point,
			NULL, NULL, NULL,
			STATUS_LEDS_THREAD_PRIORITY, 0, K_NO_WAIT);

	device_healthcheck();
}

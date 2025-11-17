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

#ifdef CONFIG_PARTITION_MANAGER_ENABLED

#define SLOT_A_FLASH_AREA_ID PM_MCUBOOT_PRIMARY_ID
#define SLOT_B_FLASH_AREA_ID PM_MCUBOOT_SECONDARY_ID

#ifdef CONFIG_NCS_IS_VARIANT_IMAGE
#define IS_SLOT_A 0
#define IS_SLOT_B 1
#else
#define IS_SLOT_A 1
#define IS_SLOT_B 0
#endif

#else /* CONFIG_PARTITION_MANAGER_ENABLED */

#define CODE_PARTITION DT_CHOSEN(zephyr_code_partition)
#define CODE_PARTITION_OFFSET FIXED_PARTITION_NODE_OFFSET(CODE_PARTITION)

#define SLOT_A_PARTITION slot0_partition
#define SLOT_B_PARTITION slot1_partition

#define SLOT_A_OFFSET FIXED_PARTITION_OFFSET(SLOT_A_PARTITION)
#define SLOT_B_OFFSET FIXED_PARTITION_OFFSET(SLOT_B_PARTITION)
#define SLOT_A_SIZE   FIXED_PARTITION_SIZE(SLOT_A_PARTITION)
#define SLOT_B_SIZE   FIXED_PARTITION_SIZE(SLOT_B_PARTITION)

#define SLOT_A_FLASH_AREA_ID FIXED_PARTITION_ID(SLOT_A_PARTITION)
#define SLOT_B_FLASH_AREA_ID FIXED_PARTITION_ID(SLOT_B_PARTITION)

#define IS_SLOT_A                                                                                  \
	(CODE_PARTITION_OFFSET >= SLOT_A_OFFSET &&                                                 \
	 CODE_PARTITION_OFFSET < SLOT_A_OFFSET + SLOT_A_SIZE)
#define IS_SLOT_B                                                                                  \
	(CODE_PARTITION_OFFSET >= SLOT_B_OFFSET &&                                                 \
	 CODE_PARTITION_OFFSET < SLOT_B_OFFSET + SLOT_B_SIZE)

#endif /* CONFIG_PARTITION_MANAGER_ENABLED */

#define STATUS_LEDS_THREAD_STACK_SIZE 512
#define STATUS_LEDS_THREAD_PRIORITY (CONFIG_NUM_PREEMPT_PRIORITIES - 1)
K_THREAD_STACK_DEFINE(status_leds_thread_stack_area, STATUS_LEDS_THREAD_STACK_SIZE);

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

static enum boot_slot active_boot_slot_get(void)
{
	enum boot_slot active_slot = BOOT_SLOT_NONE;

	if (IS_SLOT_A) {
		active_slot = BOOT_SLOT_PRIMARY;
	} else if (IS_SLOT_B) {
		active_slot = BOOT_SLOT_SECONDARY;
	} else {
		LOG_ERR("Cannot determine current slot");
	}

	return active_slot;
}

static bool slot_confirmed(enum boot_slot slot)
{
	struct boot_swap_state state;
	const struct flash_area *fa;
	int area_id = -1;
	char *img_set = NULL;
	bool confirmed = false;
	int ret;

	if (slot == BOOT_SLOT_PRIMARY) {
		img_set = "A";
		area_id = SLOT_A_FLASH_AREA_ID;
	} else if (slot == BOOT_SLOT_SECONDARY) {
		img_set = "B";
		area_id = SLOT_B_FLASH_AREA_ID;
	} else {
		LOG_ERR("Cannot determine slot to check for confirmation");
		return false;
	}

	if (flash_area_open(area_id, &fa) != 0) {
		LOG_ERR("Cannot open flash area for slot %s", img_set);
		return false;
	}

	ret = boot_read_swap_state(fa, &state);
	if (ret != 0) {
		LOG_ERR("Cannot read swap state for slot %s", img_set);
	} else if (state.image_ok == BOOT_FLAG_SET) {
		confirmed = true;
	} else {
		confirmed = false;
	}

	flash_area_close(fa);

	return confirmed;
}

static void device_healthcheck(void)
{
	int err;
	char *img_set = NULL;
	enum boot_slot active_slot = active_boot_slot_get();

	/* Confirming only in non-degraded boot states */
	if (active_slot == BOOT_SLOT_PRIMARY) {
		img_set = "A";
	} else if (active_slot == BOOT_SLOT_SECONDARY) {
		img_set = "B";
	} else {
		LOG_ERR("Cannot determine active slot for health check");
		return;
	}

	if (slot_confirmed(active_slot)) {
		LOG_INF("Slot %s already confirmed, no action needed", img_set);
		return;
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

	err = boot_request_confirm_slot(ACTIVE_IMAGE, active_slot);
	if (err == 0) {
		LOG_INF("Confirmed\n");
	} else {
		LOG_ERR("Failed to confirm, err: %d", err);
	}
}

static void select_slot_for_single_boot(enum boot_slot slot)
{
	int err = 0;
	char active_slot = (active_boot_slot_get() == BOOT_SLOT_PRIMARY) ? 'A' : 'B';

#ifdef CONFIG_NRF_MCUBOOT_BOOT_REQUEST_PREFERENCE_KEEP
	if (slot == BOOT_SLOT_PRIMARY) {
		LOG_INF("Switching slots (%c -> A)", active_slot);
	} else if (slot == BOOT_SLOT_SECONDARY) {
		LOG_INF("Switching slots (%c -> B)", active_slot);
	} else {
		LOG_ERR("Cannot determine active slot, cannot toggle");
		return;
	}
#else
	if (slot == BOOT_SLOT_PRIMARY) {
		LOG_INF("Temporarily switching slots (%c -> A)", active_slot);
	} else if (slot == BOOT_SLOT_SECONDARY) {
		LOG_INF("Temporarily switching slots (%c -> B)", active_slot);
	} else {
		LOG_ERR("Cannot determine active slot, cannot toggle");
		return;
	}
#endif

	err = boot_request_set_preferred_slot(ACTIVE_IMAGE, slot);
	if (err == 0) {
		LOG_INF("Slot toggled, restart the device to enforce");
	} else {
		LOG_ERR("Failed to toggle slots, err: %d", err);
	}
}

static void boot_state_report(void)
{
	enum boot_slot active_slot = active_boot_slot_get();

	if (active_slot == BOOT_SLOT_PRIMARY) {
		LOG_INF("Booted from slot A");
	} else if (active_slot == BOOT_SLOT_SECONDARY) {
		LOG_INF("Booted from slot B");
	} else {
		LOG_INF("Cannot determine active slot");
	}
}

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	if ((has_changed & DK_BTN1_MSK) && (button_state & DK_BTN1_MSK)) {
		select_slot_for_single_boot(BOOT_SLOT_PRIMARY);
	} else if ((has_changed & DK_BTN2_MSK) && (button_state & DK_BTN2_MSK)) {
		select_slot_for_single_boot(BOOT_SLOT_SECONDARY);
	}
}

struct k_thread status_leds_thread_data;

static void status_leds_thread_entry_point(void *p1, void *p2, void *p3)
{
	int blinking_led = DK_LED1;
	enum boot_slot active_slot = active_boot_slot_get();

	if (active_slot == BOOT_SLOT_PRIMARY) {
		blinking_led = DK_LED1;
	} else if (active_slot == BOOT_SLOT_SECONDARY) {
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

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_manifests_state.h>
#include <suit_components_state.h>
#include <device_management.h>

#include "common.h"
#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>


#define STATUS_LEDS_THREAD_STACK_SIZE 512
#define STATUS_LEDS_THREAD_PRIORITY (CONFIG_NUM_PREEMPT_PRIORITIES - 1)
K_THREAD_STACK_DEFINE(status_leds_thread_stack_area, STATUS_LEDS_THREAD_STACK_SIZE);

LOG_MODULE_REGISTER(AB, CONFIG_SUIT_LOG_LEVEL);

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	if ((has_changed & DK_BTN4_MSK) && (button_state & DK_BTN4_MSK)) {
		toggle_preferred_boot_set();
	}
}

/**
 * Meaning of LED states on LED-s 3 and 4:
 *
 * 1) Both LEDs are on all the time: Normal mode, not degraded.
 * 2) Both leds blinking at the same time: Degraded mode.
 * 3) Only one led blinking:  Degraded mode, no radio.
 * 4) LEDs blinking one after the other:  Device in recovery mode.
 * 5) Both LEDs off:  Unrecognized state.
 */
void status_leds_thread_entry_point(void *p1, void *p2, void *p3)
{
	uint8_t sequence_led3 = 0b00;
	uint8_t sequence_led4 = 0b00;
	uint32_t boot_status = device_boot_status_get();
	uint8_t cnt = 0;

	if (device_is_in_recovery_mode()) {
		sequence_led3 = 0b01;
		sequence_led4 = 0b10;
	} else if (boot_status == BOOT_STATUS_A || boot_status == BOOT_STATUS_B) {
		sequence_led3 = 0b11;
		sequence_led4 = 0b11;
	} else if (boot_status == BOOT_STATUS_A_DEGRADED || boot_status == BOOT_STATUS_B_DEGRADED) {
		sequence_led3 = 0b10;
		sequence_led4 = 0b10;
	} else if (boot_status == BOOT_STATUS_A_NO_RADIO || boot_status == BOOT_STATUS_B_NO_RADIO) {
		sequence_led3 = 0b01;
		sequence_led4 = 0b00;
	}

	while (1) {
		if (sequence_led3 & (1 << cnt)) {
			dk_set_led_on(DK_LED3);
		} else {
			dk_set_led_off(DK_LED3);
		}

		if (sequence_led4 & (1 << cnt)) {
			dk_set_led_on(DK_LED4);
		} else {
			dk_set_led_off(DK_LED4);
		}

		k_msleep(250);

		cnt = (cnt + 1) % 2;
	}
}

int main(void)
{
	char *app_img_name = NULL;
	int ret = 0;

	ret = dk_leds_init();
	if (ret) {
		LOG_ERR("Cannot init LEDs (err: %d)\r\n", ret);
	}

	ret = dk_buttons_init(button_handler);
	if (ret) {
		LOG_ERR("Cannot init buttons (err: %d)", ret);
	}

	dk_set_led_on(DK_LED1);
	dk_set_led_on(DK_LED2);

	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_BT)) {
		start_smp_bluetooth_adverts();
		k_msleep(250);
	}
	printk("\n");

	suit_mainfests_state_report();
	printk("\n");

	int blinking_led = DK_LED1;

	if (DT_SAME_NODE(DT_ALIAS(suit_active_code_partition),
			 DT_NODELABEL(cpuapp_slot_a_partition))) {
		app_img_name = "A";
	} else {
		app_img_name = "B";
		blinking_led = DK_LED2;
	}

	printk("---------------------------------------------------\n");
	printk("A/B Hello world from %s, blinks: %d, img: %s\n", CONFIG_BOARD, CONFIG_N_BLINKS,
	       app_img_name);
	printk("BUILD: %s %s\n", __DATE__, __TIME__);
	printk("---------------------------------------------------\n\n");

	suit_components_state_report();
	printk("\n");

	device_boot_state_report();
	printk("\n");

	device_healthcheck();

	struct k_thread status_leds_thread_data;

	k_thread_create(&status_leds_thread_data, status_leds_thread_stack_area,
			K_THREAD_STACK_SIZEOF(status_leds_thread_stack_area),
			status_leds_thread_entry_point,
			NULL, NULL, NULL,
			STATUS_LEDS_THREAD_PRIORITY, 0, K_NO_WAIT);

	while (1) {
		for (int i = 0; i < CONFIG_N_BLINKS; i++) {
			dk_set_led_off(blinking_led);
			k_msleep(250);
			dk_set_led_on(blinking_led);
			k_msleep(250);
		}

		k_msleep(5000);
	}

	return 0;
}

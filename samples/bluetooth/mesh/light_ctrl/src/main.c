/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic mesh light fixture sample
 */
#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <bluetooth/mesh/dk_prov.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"
#include "lc_pwm_led.h"

#ifdef CONFIG_EMDS
#include <emds/emds.h>

#if defined(CONFIG_BT_CTLR)
#include <mpsl/mpsl_lib.h>
#endif

#define EMDS_DEV_IRQ 26
#define EMDS_DEV_PRIO 0
#define EMDS_ISR_ARG 0
#define EMDS_IRQ_FLAGS 0

static void button_handler_cb(uint32_t pressed, uint32_t changed)
{
	if (!bt_mesh_is_provisioned()) {
		return;
	}

	if (pressed & changed & BIT(3)) {
		NVIC_SetPendingIRQ(EMDS_DEV_IRQ);
	}
}

static void app_emds_cb(void)
{
	printk("SAMPLE HALTED!!!\n");
	dk_set_leds(DK_LED2_MSK | DK_LED3_MSK | DK_LED4_MSK);
	k_fatal_halt(K_ERR_CPU_EXCEPTION);
}

static void isr_emds_cb(void *arg)
{
	ARG_UNUSED(arg);

#if defined(CONFIG_BT_CTLR)
	/* Stop mpsl to reduce power usage. */
	mpsl_lib_uninit();
#endif

	emds_store();
}
#endif

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	dk_leds_init();
	dk_buttons_init(NULL);

#ifdef CONFIG_EMDS
	static struct button_handler button_handler = {
		.cb = button_handler_cb,
	};

	dk_button_handler_add(&button_handler);

	err = emds_init(&app_emds_cb);
	if (err) {
		printk("Initializing emds failed (err %d)\n", err);
		return;
	}
#endif

	err = bt_mesh_init(bt_mesh_dk_prov_init(), model_handler_init());
	if (err) {
		printk("Initializing mesh failed (err %d)\n", err);
		return;
	}

#ifdef CONFIG_EMDS
	err = emds_load();
	if (err) {
		printk("Restore of emds data failed (err %d)\n", err);
		return;
	}

	err = emds_prepare();
	if (err) {
		printk("Preparation emds failed (err %d)\n", err);
		return;
	}

	IRQ_CONNECT(EMDS_DEV_IRQ, EMDS_DEV_PRIO, isr_emds_cb,
		    EMDS_ISR_ARG, EMDS_IRQ_FLAGS);
	irq_enable(EMDS_DEV_IRQ);
#endif

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	/* This will be a no-op if settings_load() loaded provisioning info */
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);

	printk("Mesh initialized\n");

	model_handler_start();
}

int main(void)
{
	int err;

	printk("Initializing...\n");
	lc_pwm_led_init();
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	return 0;
}

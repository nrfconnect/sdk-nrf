/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/mesh.h>
#include <dk_buttons_and_leds.h>
#include <drivers/hwinfo.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(dk_bt_mesh_prov, CONFIG_BT_MESH_DK_PROV_LOG_LEVEL);

static u32_t oob_toggles;
static u32_t oob_toggle_count;
static struct k_delayed_work oob_work;

static u32_t button_press_count;
static struct button_handler button_handler;

static void oob_blink_toggle(struct k_work *work)
{
	dk_set_leds(((oob_toggle_count++) & 0x01) ? DK_NO_LEDS_MSK :
						    DK_ALL_LEDS_MSK);
	if (oob_toggle_count == oob_toggles) {
		oob_toggle_count = 0;
		k_delayed_work_submit(
			CONTAINER_OF(work, struct k_delayed_work, work), 3000);
	} else {
		k_delayed_work_submit(
			CONTAINER_OF(work, struct k_delayed_work, work), 250);
	}
}

static int output_number(bt_mesh_output_action_t action, u32_t number)
{
	if (IS_ENABLED(CONFIG_BT_MESH_DK_PROV_OOB_LOG) &&
	    action == BT_MESH_DISPLAY_NUMBER) {
		printk("OOB Number: %u\n", number);
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_DK_PROV_OOB_BLINK) &&
	    action == BT_MESH_BLINK) {
		LOG_DBG("Blinking %u times", number);
		oob_toggles = number * 2;
		oob_toggle_count = 0;
		k_delayed_work_init(&oob_work, oob_blink_toggle);
		k_delayed_work_submit(&oob_work, K_NO_WAIT);
		return 0;
	}

	return -ENOTSUP;
}

static int output_string(const char *string)
{
	printk("OOB String: %s\n", string);
	return 0;
}

static void oob_button_timeout(struct k_work *work)
{
	if (!IS_ENABLED(CONFIG_BT_MESH_DK_PROV_OOB_BUTTON)) {
		return;
	}

	dk_button_handler_remove(&button_handler);
	dk_set_leds(DK_NO_LEDS_MSK);

	bt_mesh_input_number(button_press_count);
}

static void oob_button_handler(u32_t button_state, u32_t has_changed)
{
	if (!(button_state & has_changed)) {
		return;
	}

	u32_t led = button_press_count++ & BIT_MASK(3);

	dk_set_leds_state(BIT(led) & DK_ALL_LEDS_MSK,
			  BIT(led - 4) & DK_ALL_LEDS_MSK);

	k_delayed_work_submit(&oob_work, 3000);
}

static int input(bt_mesh_input_action_t act, u8_t size)
{
	if (!IS_ENABLED(CONFIG_BT_MESH_DK_PROV_OOB_BUTTON)) {
		return -ENOTSUP;
	}

	LOG_DBG("Press a button to set the right number.");
	k_delayed_work_init(&oob_work, oob_button_timeout);
	button_handler.cb = oob_button_handler;
	dk_button_handler_add(&button_handler);
	button_press_count = 0;

	return 0;
}

static void input_complete(void)
{
	if (IS_ENABLED(CONFIG_BT_MESH_DK_PROV_OOB_BUTTON)) {
		k_delayed_work_cancel(&oob_work);
	}
}

static void oob_stop(void)
{
	if (IS_ENABLED(CONFIG_BT_MESH_DK_PROV_OOB_BUTTON) ||
	    IS_ENABLED(CONFIG_BT_MESH_DK_PROV_OOB_BLINK)) {
		k_delayed_work_cancel(&oob_work);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_DK_PROV_OOB_BUTTON)) {
		dk_button_handler_remove(&button_handler);
	}
}

static void prov_complete(u16_t net_idx, u16_t src)
{
	oob_stop();
	LOG_DBG("Prov complete! Addr: 0x%04x\n", src);
}

static void prov_reset(void)
{
	oob_stop();
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

static u8_t dev_uuid[16];

static const struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.output_size = 1,
	.output_actions = (0
#ifdef CONFIG_BT_MESH_DK_PROV_OOB_LOG
		| BT_MESH_DISPLAY_NUMBER
		| BT_MESH_DISPLAY_STRING
#endif
#ifdef CONFIG_BT_MESH_DK_PROV_OOB_BLINK
		| BT_MESH_BLINK
#endif
		),
	.output_number = output_number,
	.output_string = output_string,
	.input_size = 1,
	.input = input,
#ifdef CONFIG_BT_MESH_DK_PROV_OOB_BUTTON
	.input_actions = BT_MESH_PUSH,
#endif
	.complete = prov_complete,
	.input_complete = input_complete,
	.reset = prov_reset,
};

const struct bt_mesh_prov *bt_mesh_dk_prov_init(void)
{
	hwinfo_get_device_id(dev_uuid, sizeof(dev_uuid));
	return &prov;
}

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/mesh.h>
#include <dk_buttons_and_leds.h>
#include <drivers/hwinfo.h>

static u32_t oob_toggles;
static u32_t oob_toggle_count;
static struct k_delayed_work oob_work;
static u32_t button_press_count;

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
	if (action == BT_MESH_DISPLAY_NUMBER) {
		printk("OOB Number: %u\n", number);
	} else if (action == BT_MESH_BLINK) {
		printk("Blinking %u times\n", number);
		oob_toggles = number * 2;
		oob_toggle_count = 0;
		k_delayed_work_init(&oob_work, oob_blink_toggle);
		k_delayed_work_submit(&oob_work, 0);
	}
	return 0;
}


static void oob_button_timeout(struct k_work *work)
{
	dk_buttons_init(NULL);
	dk_set_leds(DK_NO_LEDS_MSK);

	bt_mesh_input_number(button_press_count);
}

static void oob_button_handler(u32_t button_state, u32_t has_changed)
{
	if (button_state & has_changed) {
		button_press_count++;
		dk_set_leds((1 << button_press_count) - 1);
		k_delayed_work_submit(&oob_work, 3000);
	}
}

static int input(bt_mesh_input_action_t act, u8_t size)
{
	printk("Press a button to set the right number.\n");
	k_delayed_work_init(&oob_work, oob_button_timeout);
	dk_buttons_init(oob_button_handler);
	button_press_count = true;
	return true;
}

static void input_complete(void)
{
	k_delayed_work_cancel(&oob_work);
}

static void prov_complete(u16_t net_idx, u16_t src)
{
	k_delayed_work_cancel(&oob_work);
	dk_buttons_init(NULL);
	printk("Prov complete! Addr: 0x%04x\n", src);
}

static void prov_reset(void)
{
	k_delayed_work_cancel(&oob_work);
	dk_buttons_init(NULL);
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

static u8_t dev_uuid[16];

static const struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.output_size = 1,
	.output_actions = BT_MESH_DISPLAY_NUMBER | BT_MESH_BLINK,
	.output_number = output_number,
	.input_size = 1,
	.input = input,
	.input_actions = BT_MESH_PUSH,
	.complete = prov_complete,
	.input_complete = input_complete,
	.reset = prov_reset,
};

const struct bt_mesh_prov *prov_handler_init(void)
{
	hwinfo_get_device_id(dev_uuid, sizeof(dev_uuid));
	return &prov;
}

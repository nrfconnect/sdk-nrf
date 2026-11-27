/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/bluetooth/mesh.h>
#include <dk_buttons_and_leds.h>
#include <bluetooth/mesh/node_reset_button.h>


static struct button_handler button_handler;

static void node_reset_work_handler(struct k_work *work)
{
	printk("Node reset by button press\n");
	bt_mesh_reset();
}

static K_WORK_DELAYABLE_DEFINE(node_reset_work, node_reset_work_handler);

static void button_handler_cb(uint32_t pressed, uint32_t changed)
{
	if (!(changed & BT_MESH_NODE_RESET_BUTTON_MSK))	{
		return;
	}
	if (pressed & BT_MESH_NODE_RESET_BUTTON_MSK) {
		k_work_schedule(&node_reset_work,
				 K_MSEC(CONFIG_BT_MESH_NODE_RESET_BUTTON_HOLD_MS));
	} else {
		k_work_cancel_delayable(&node_reset_work);
	}
}

int bt_mesh_node_reset_button_init(void)
{
	button_handler.cb = button_handler_cb;
	dk_button_handler_add(&button_handler);

	return 0;
}

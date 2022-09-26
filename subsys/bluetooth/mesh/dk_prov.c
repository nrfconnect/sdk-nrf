/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dk_bt_mesh_prov, CONFIG_BT_MESH_DK_PROV_LOG_LEVEL);

static uint32_t oob_toggles;
static uint32_t oob_toggle_count;
static struct k_work_delayable oob_work;

static uint32_t button_press_count;
static struct button_handler button_handler;

static enum {
	OOB_IDLE,
	OOB_BLINK,
	OOB_BUTTON,
} oob_state;

static void oob_blink_toggle(void)
{
	dk_set_leds(((oob_toggle_count++) & 0x01) ? DK_NO_LEDS_MSK :
						    DK_ALL_LEDS_MSK);
	if (oob_toggle_count == oob_toggles) {
		oob_toggle_count = 0;
		k_work_reschedule(&oob_work, K_SECONDS(3));
	} else {
		k_work_reschedule(&oob_work, K_MSEC(250));
	}
}

static void oob_button_timeout(void)
{
	if (!IS_ENABLED(CONFIG_BT_MESH_DK_PROV_OOB_BUTTON)) {
		return;
	}

	dk_button_handler_remove(&button_handler);
	dk_set_leds(DK_NO_LEDS_MSK);

	bt_mesh_input_number(button_press_count);
	oob_state = OOB_IDLE;
}

static void oob_timer_handler(struct k_work *work)
{
	switch (oob_state) {
	case OOB_BLINK:
		oob_blink_toggle();
		break;
	case OOB_BUTTON:
		oob_button_timeout();
		break;
	case OOB_IDLE:
		dk_set_leds(DK_NO_LEDS_MSK);
		break;
	}
}

static int output_number(bt_mesh_output_action_t action, uint32_t number)
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

		oob_state = OOB_BLINK;
		k_work_reschedule(&oob_work, K_NO_WAIT);
		return 0;
	}

	return -ENOTSUP;
}

static int output_string(const char *string)
{
	printk("OOB String: %s\n", string);
	return 0;
}

static void oob_button_handler(uint32_t button_state, uint32_t has_changed)
{
	if (!(button_state & has_changed)) {
		return;
	}

	uint32_t led = button_press_count++ & BIT_MASK(3);

	dk_set_leds_state(BIT(led) & DK_ALL_LEDS_MSK,
			  BIT(led - 4) & DK_ALL_LEDS_MSK);

	k_work_reschedule(&oob_work, K_SECONDS(3));
}

static int input(bt_mesh_input_action_t act, uint8_t size)
{
	if (!IS_ENABLED(CONFIG_BT_MESH_DK_PROV_OOB_BUTTON)) {
		return -ENOTSUP;
	}

	LOG_DBG("Press a button to set the right number.");
	button_handler.cb = oob_button_handler;
	dk_button_handler_add(&button_handler);
	button_press_count = 0;
	oob_state = OOB_BUTTON;

	return 0;
}

static void oob_stop(void)
{
	if (IS_ENABLED(CONFIG_BT_MESH_DK_PROV_OOB_BUTTON) ||
	    IS_ENABLED(CONFIG_BT_MESH_DK_PROV_OOB_BLINK)) {
		oob_state = OOB_IDLE;
		k_work_cancel_delayable(&oob_work);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_DK_PROV_OOB_BUTTON)) {
		dk_button_handler_remove(&button_handler);
	}
}

static void prov_complete(uint16_t net_idx, uint16_t src)
{
	oob_stop();
	LOG_DBG("Prov complete! Addr: 0x%04x\n", src);
}

static void prov_reset(void)
{
	oob_stop();
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

static uint8_t dev_uuid[16];

static const struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
#if defined(CONFIG_BT_MESH_DK_PROV_OOB_LOG) || defined(CONFIG_BT_MESH_DK_PROV_OOB_BLINK)
	.output_size = 1,
#endif
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
	.input = input,
#ifdef CONFIG_BT_MESH_DK_PROV_OOB_BUTTON
	.input_size = 1,
	.input_actions = BT_MESH_PUSH,
#endif
	.complete = prov_complete,
	.input_complete = oob_stop,
	.reset = prov_reset,
};

const struct bt_mesh_prov *bt_mesh_dk_prov_init(void)
{
	/* Generate an RFC-4122 version 4 compliant UUID.
	 * Format:
	 *
	 * 0                   1                   2                   3
	 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * |                          time_low                             |
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * |       time_mid                |         time_hi_and_version   |
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * |clk_seq_hi_res |  clk_seq_low  |         node (0-1)            |
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * |                         node (2-5)                            |
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *
	 * Where the 4 most significant bits of time_hi_and_version shall be
	 * 0b0010 and the 2 most significant bits of clk_seq_hi_res shall be
	 * 0b10. The remaining fields have no required values, and are fetched
	 * from the HW info device ID. The fields are encoded in big endian
	 * format.
	 *
	 * https://tools.ietf.org/html/rfc4122
	 */
	size_t id_len = hwinfo_get_device_id(dev_uuid, sizeof(dev_uuid));

	if (!IS_ENABLED(CONFIG_BT_MESH_DK_LEGACY_UUID_GEN)) {
		/* If device ID is shorter than UUID size, fill rest of buffer with
		 * inverted device ID.
		 */
		for (size_t i = id_len; i < sizeof(dev_uuid); i++) {
			dev_uuid[i] = dev_uuid[i % id_len] ^ 0xff;
		}
	}

	dev_uuid[6] = (dev_uuid[6] & BIT_MASK(4)) | BIT(6);
	dev_uuid[8] = (dev_uuid[8] & BIT_MASK(6)) | BIT(7);

	k_work_init_delayable(&oob_work, oob_timer_handler);

	return &prov;
}

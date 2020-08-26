/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <random/rand32.h>
#include <drivers/gpio.h>
#include <drivers/gpio/gpio_sx1509b.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"
#include "orientation_handler.h"
#include "thingy52_mod.h"

#define DEFAULT_RBG_TTL 100
#define DEFAULT_RBG_MIN_SPEED_DELAY 200
#define DEFAULT_RBG_MAX_SPEED_DELAY 2000

#define ORIENTATION_SPEAKER_ON THINGY_ORIENT_Y_UP
#define ORIENTATION_START_AUTO_SEND THINGY_ORIENT_Y_DOWN
#define ORIENTATION_CHOOSE_COLOR THINGY_ORIENT_X_DOWN
#define ORIENTATION_SEND_LIGHT THINGY_ORIENT_Z_DOWN

static const struct device *io_expander_dev;

struct thingy52_ctrl {
	struct k_delayed_work rgb_pick_work;
	struct k_delayed_work delay_pick_work;
	struct k_delayed_work blink_work;
	struct k_delayed_work auto_send_work;
	struct bt_mesh_thingy52_rgb msg_rgb_color;
	uint16_t msg_relay_delay;
	bool msg_speaker_on;
};

static struct thingy52_ctrl cli_ctrl = { .msg_relay_delay =
						 DEFAULT_RBG_MIN_SPEED_DELAY };

static struct bt_mesh_thingy52_mod thingy52_mod =
	BT_MESH_THINGY52_MOD_INIT(NULL);

static void set_large_rgb_led(struct bt_mesh_thingy52_rgb rgb)
{
	int err = 0;

	err |= sx1509b_led_intensity_pin_set(io_expander_dev, RED_LED, rgb.red);
	err |= sx1509b_led_intensity_pin_set(io_expander_dev, GREEN_LED,
					     rgb.green);
	err |= sx1509b_led_intensity_pin_set(io_expander_dev, BLUE_LED,
					     rgb.blue);

	if (err) {
		printk("Error setting large LED\n");
	}
}

static void set_small_rgb_led(bool onoff)
{
	int err = 0;
	uint8_t color_val = onoff ? 255 : 0;

	for (int i = SMALL_RED_LED; i <= SMALL_BLUE_LED; i++) {
		err |= sx1509b_led_intensity_pin_set(io_expander_dev, i,
						     color_val);
	}

	if (err) {
		printk("Error setting small LED\n");
	}
}

static uint8_t color_value(uint8_t index)
{
	if (index < 256 / 6) {
		return index * 6;
	}

	if (index < (3 * 256) / 6) {
		return 255;
	}

	if (index < (4 * 256) / 6) {
		return (3 * 255) - index * 6 + 1;
	}

	return 0;
}

static void pick_rgb(uint8_t input_val, struct bt_mesh_thingy52_rgb *rgb)
{
	uint16_t raw_val = input_val % (6 * 256);

	if (!raw_val) {
		rgb->red = 0;
		rgb->green = 0;
		rgb->blue = 0;
		return;
	}

	rgb->red = color_value(input_val);
	rgb->green = color_value(input_val + (256 / 3));
	rgb->blue = color_value(input_val + (2 * 256 / 3));
}

static void button_handler(uint32_t pressed, uint32_t changed)
{
	switch (orientation_get()) {
	case ORIENTATION_SPEAKER_ON:
		if (pressed & BIT(0)) {
			cli_ctrl.msg_speaker_on = !cli_ctrl.msg_speaker_on;

			/* Set small RGB LED to indicate if the
			 * speaker_on flag (used by the servers) is set
			 */
			set_small_rgb_led(cli_ctrl.msg_speaker_on);
		}

		break;
	case ORIENTATION_START_AUTO_SEND:
		if (pressed & BIT(0)) {
			static bool onoff = true;

			if (onoff) {
				k_delayed_work_submit(&cli_ctrl.auto_send_work,
						      K_NO_WAIT);
			} else {
				k_delayed_work_cancel(&cli_ctrl.auto_send_work);
			}

			onoff = !onoff;
		}

		break;
	case ORIENTATION_CHOOSE_COLOR:
		if (pressed & BIT(0)) {
			k_delayed_work_submit(&cli_ctrl.rgb_pick_work,
					      K_NO_WAIT);
		} else if (!pressed & BIT(0)) {
			k_delayed_work_cancel(&cli_ctrl.rgb_pick_work);
		}

		break;
	case ORIENTATION_SEND_LIGHT:
		if (pressed & BIT(0)) {
			cli_ctrl.msg_relay_delay = DEFAULT_RBG_MIN_SPEED_DELAY;

			k_delayed_work_submit(&cli_ctrl.blink_work, K_NO_WAIT);
			k_delayed_work_submit(&cli_ctrl.delay_pick_work,
					      K_NO_WAIT);
		} else if (!pressed & BIT(0)) {
			k_delayed_work_cancel(&cli_ctrl.blink_work);
			k_delayed_work_cancel(&cli_ctrl.delay_pick_work);

			set_large_rgb_led(cli_ctrl.msg_rgb_color);

			struct bt_mesh_thingy52_rgb_msg msg;

			msg.ttl = DEFAULT_RBG_TTL;
			msg.duration = cli_ctrl.msg_relay_delay;
			memcpy(&msg.color, &cli_ctrl.msg_rgb_color,
			       sizeof(msg.color));
			msg.speaker_on = cli_ctrl.msg_speaker_on;

			bt_mesh_thingy52_mod_rgb_set(&thingy52_mod, NULL, &msg);
		}

		break;
	default:
		return;
	}
}

static void button_and_led_init(void)
{
	int err = 0;

	err |= dk_buttons_init(button_handler);

	for (int i = GREEN_LED; i <= RED_LED; i++) {
		err |= sx1509b_led_intensity_pin_configure(io_expander_dev, i);
	}

	for (int i = SMALL_RED_LED; i <= SMALL_BLUE_LED; i++) {
		err |= sx1509b_led_intensity_pin_configure(io_expander_dev, i);
	}

	if (err) {
		printk("Initializing buttons and LEDs failed\n");
	}
}

static void auto_send_work_handler(struct k_work *work)
{
	struct bt_mesh_thingy52_rgb rand_rgb;
	struct bt_mesh_thingy52_rgb_msg msg;

	pick_rgb((uint8_t)sys_rand32_get(), &rand_rgb);

	msg.ttl = DEFAULT_RBG_TTL;
	msg.duration = DEFAULT_RBG_MIN_SPEED_DELAY +
		       (sys_rand32_get() % (DEFAULT_RBG_MAX_SPEED_DELAY -
					    DEFAULT_RBG_MIN_SPEED_DELAY));
	memcpy(&msg.color, &rand_rgb, sizeof(msg.color));
	msg.speaker_on = cli_ctrl.msg_speaker_on;

	bt_mesh_thingy52_mod_rgb_set(&thingy52_mod, NULL, &msg);

	k_delayed_work_submit(&cli_ctrl.auto_send_work,
			      K_MSEC(DEFAULT_RBG_MAX_SPEED_DELAY));
}

static void rgb_pick_work_handler(struct k_work *work)
{
	struct thingy52_ctrl *rgb =
		CONTAINER_OF(work, struct thingy52_ctrl, rgb_pick_work.work);
	static uint8_t val;

	pick_rgb(val, &rgb->msg_rgb_color);
	set_large_rgb_led(rgb->msg_rgb_color);
	val++;

	if (!val) {
		memset(&rgb->msg_rgb_color, 0,
		       sizeof(struct bt_mesh_thingy52_rgb));
		set_large_rgb_led(rgb->msg_rgb_color);
		return;
	}

	k_delayed_work_submit(&cli_ctrl.rgb_pick_work, K_MSEC(30));
}

static void delay_pick_work_handler(struct k_work *work)
{
	struct thingy52_ctrl *rgb =
		CONTAINER_OF(work, struct thingy52_ctrl, delay_pick_work.work);

	if (rgb->msg_relay_delay < DEFAULT_RBG_MAX_SPEED_DELAY) {
		rgb->msg_relay_delay += 50;
	}

	k_delayed_work_submit(&cli_ctrl.delay_pick_work,
			      K_MSEC(DEFAULT_RBG_MIN_SPEED_DELAY));
}

static void blink_work_handler(struct k_work *work)
{
	struct thingy52_ctrl *rgb =
		CONTAINER_OF(work, struct thingy52_ctrl, blink_work.work);
	static bool onoff;

	if (onoff) {
		set_large_rgb_led(rgb->msg_rgb_color);
	} else {
		struct bt_mesh_thingy52_rgb rgb_off = { 0, 0, 0 };

		set_large_rgb_led(rgb_off);
	}

	onoff = !onoff;

	k_delayed_work_submit(&cli_ctrl.blink_work,
			      K_MSEC(rgb->msg_relay_delay / 2));
}

static void attention_on(struct bt_mesh_model *mod)
{
	struct bt_mesh_thingy52_rgb rgb_on = { 255, 255, 255 };

	set_large_rgb_led(rgb_on);
}

static void attention_off(struct bt_mesh_model *mod)
{
	struct bt_mesh_thingy52_rgb rgb_off = { 0, 0, 0 };

	set_large_rgb_led(rgb_off);
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(
		1,
		BT_MESH_MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
				   BT_MESH_MODEL_HEALTH_SRV(&health_srv,
							    &health_pub)),
		BT_MESH_MODEL_LIST(BT_MESH_MODEL_THINGY52_MOD(&thingy52_mod))),
};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(void)
{
	io_expander_dev = device_get_binding(DT_LABEL(DT_NODELABEL(sx1509b)));

	if (io_expander_dev == NULL) {
		printk("Failure occurred while binding I/O expander device\n");
		return &comp;
	}

	button_and_led_init();
	orientation_init();

	k_delayed_work_init(&cli_ctrl.rgb_pick_work, rgb_pick_work_handler);
	k_delayed_work_init(&cli_ctrl.delay_pick_work, delay_pick_work_handler);
	k_delayed_work_init(&cli_ctrl.blink_work, blink_work_handler);
	k_delayed_work_init(&cli_ctrl.auto_send_work, auto_send_work_handler);

	return &comp;
}

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"

#if !DT_NODE_EXISTS(DT_ALIAS(led0))
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

#define MESH_LED DK_LED1

static void led_set(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    const struct bt_mesh_onoff_set *set, struct bt_mesh_onoff_status *rsp);

static void led_get(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    struct bt_mesh_onoff_status *rsp);

static const struct bt_mesh_onoff_srv_handlers onoff_handlers = {
	.set = led_set,
	.get = led_get,
};

struct led_ctx {
	struct bt_mesh_onoff_srv srv;
	struct k_work_delayable work;
	uint32_t remaining;
	bool value;
};

static struct led_ctx led_ctx = {
	.srv = BT_MESH_ONOFF_SRV_INIT(&onoff_handlers),
};

static void led_transition_start(void)
{
	/* As long as the transition is in progress, the onoff
	 * state is "on":
	 */
	dk_set_led(MESH_LED, true);
	k_work_reschedule(&led_ctx.work, K_MSEC(led_ctx.remaining));
	led_ctx.remaining = 0;
}

static void led_status(struct bt_mesh_onoff_status *status)
{
	status->remaining_time =
		k_ticks_to_ms_ceil32(k_work_delayable_remaining_get(&led_ctx.work)) +
		led_ctx.remaining;
	status->target_on_off = led_ctx.value;
	/* As long as the transition is in progress, the onoff state is "on": */
	status->present_on_off = led_ctx.value || status->remaining_time;
}

static void led_set(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    const struct bt_mesh_onoff_set *set, struct bt_mesh_onoff_status *rsp)
{
	if (set->on_off == led_ctx.value) {
		goto respond;
	}

	led_ctx.value = set->on_off;
	if (!bt_mesh_model_transition_time(set->transition)) {
		led_ctx.remaining = 0;
		dk_set_led(MESH_LED, set->on_off);
		goto respond;
	}

	led_ctx.remaining = set->transition->time;

	if (set->transition->delay) {
		k_work_reschedule(&led_ctx.work, K_MSEC(set->transition->delay));
	} else {
		led_transition_start();
	}

respond:
	if (rsp) {
		led_status(rsp);
	}
}

static void led_get(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    struct bt_mesh_onoff_status *rsp)
{
	led_status(rsp);
}

static void led_work(struct k_work *work)
{
	if (led_ctx.remaining) {
		led_transition_start();
	} else {
		dk_set_led(MESH_LED, led_ctx.value);

		/* Publish the new value at the end of the transition */
		struct bt_mesh_onoff_status status;

		led_status(&status);
		bt_mesh_onoff_srv_pub(&led_ctx.srv, NULL, &status);
	}
}

/* Set up a repeating delayed work to blink the DK's LEDs when attention is
 * requested.
 */
static struct k_work_delayable attention_blink_work;
static bool attention;

static void attention_blink(struct k_work *work)
{
	static int idx;

	if (attention) {
		dk_set_leds(BIT((idx++) % 2));
		k_work_reschedule(&attention_blink_work, K_MSEC(30));
	} else {
		dk_set_leds(DK_NO_LEDS_MSK);
	}
}

static void attention_on(const struct bt_mesh_model *mod)
{
	attention = true;
	k_work_reschedule(&attention_blink_work, K_NO_WAIT);
}

static void attention_off(const struct bt_mesh_model *mod)
{
	/* Will stop rescheduling blink timer */
	attention = false;
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
	BT_MESH_ELEM(1,
		     BT_MESH_MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
					BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
					BT_MESH_MODEL_ONOFF_SRV(&led_ctx.srv)),
		     BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(void)
{
	k_work_init_delayable(&attention_blink_work, attention_blink);
	k_work_init_delayable(&led_ctx.work, led_work);

	return &comp;
}

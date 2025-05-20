/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"
#include "lc_pwm_led.h"

#define PWM_SIZE_STEP 512

struct lightness_ctx {
	struct bt_mesh_lightness_srv lightness_srv;
	struct k_work_delayable per_work;
	uint16_t target_lvl;
	uint16_t current_lvl;
	uint32_t time_per;
	uint32_t rem_time;
};

#if IS_ENABLED(CONFIG_BT_MESH_NLC_PERF_CONF)
static const uint8_t cmp2_elem_offset1[2] = { 0, 1 };
static const uint8_t cmp2_elem_offset2[1] = { 0 };

static const struct bt_mesh_comp2_record comp_rec[2] = {
	{/* Basic Lightness Controller NLC Profile 1.0.1 */
	.id = BT_MESH_NLC_PROFILE_ID_BASIC_LIGHTNESS_CONTROLLER,
	.version.x = 1,
	.version.y = 0,
	.version.z = 1,
	.elem_offset_cnt = 2,
	.elem_offset = cmp2_elem_offset1,
	.data_len = 0
	},
	{ /* Energy Monitor NLC Profile 1.0.1 */
	.id = BT_MESH_NLC_PROFILE_ID_ENERGY_MONITOR,
	.version.x = 1,
	.version.y = 0,
	.version.z = 1,
	.elem_offset_cnt = 1,
	.elem_offset = cmp2_elem_offset2,
	.data_len = 0
	}
};

static const struct bt_mesh_comp2 comp_p2 = {
	.record_cnt = 2,
	.record = comp_rec
};
#endif

/* Set up a repeating delayed work to blink the DK's LEDs when attention is
 * requested.
 */
static struct k_work_delayable attention_blink_work;
static bool attention;

static void attention_blink(struct k_work *work)
{
	static int idx;
	const uint8_t pattern[] = {
#if DT_NODE_EXISTS(DT_ALIAS(led0))
		BIT(0),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led1))
		BIT(1),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led2))
		BIT(2),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led3))
		BIT(3),
#endif
	};

	if (attention) {
		dk_set_leds(pattern[idx++ % ARRAY_SIZE(pattern)]);
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

static void start_new_light_trans(const struct bt_mesh_lightness_set *set,
				  struct lightness_ctx *ctx)
{
	uint32_t step_cnt = abs(set->lvl - ctx->current_lvl) / PWM_SIZE_STEP;
	uint32_t time = set->transition ? set->transition->time : 0;
	uint32_t delay = set->transition ? set->transition->delay : 0;

	ctx->target_lvl = set->lvl;
	ctx->time_per = (step_cnt ? time / step_cnt : 0);
	ctx->rem_time = time;
	k_work_reschedule(&ctx->per_work, K_MSEC(delay));

	printk("New light transition-> Lvl: %d, Time: %d, Delay: %d\n",
	       set->lvl, time, delay);
}

static void periodic_led_work(struct k_work *work)
{
	uint16_t clamped_lvl;
	struct lightness_ctx *l_ctx =
		CONTAINER_OF(work, struct lightness_ctx, per_work.work);
	l_ctx->rem_time -= l_ctx->time_per;

	if ((l_ctx->rem_time <= l_ctx->time_per) ||
	    (abs(l_ctx->target_lvl - l_ctx->current_lvl) <= PWM_SIZE_STEP)) {
		struct bt_mesh_lightness_status status = {
			.current = l_ctx->target_lvl,
			.target = l_ctx->target_lvl,
		};

		l_ctx->current_lvl = l_ctx->target_lvl;
		l_ctx->rem_time = 0;

		bt_mesh_lightness_srv_pub(&l_ctx->lightness_srv, NULL, &status);

		goto apply_and_print;
	} else if (l_ctx->target_lvl > l_ctx->current_lvl) {
		l_ctx->current_lvl += PWM_SIZE_STEP;
	} else {
		l_ctx->current_lvl -= PWM_SIZE_STEP;
	}

	k_work_reschedule(&l_ctx->per_work, K_MSEC(l_ctx->time_per));
apply_and_print:
	clamped_lvl = bt_mesh_lightness_clamp(&l_ctx->lightness_srv, l_ctx->current_lvl);
	lc_pwm_led_set(clamped_lvl);
	printk("Current light lvl: %u/65535\n", clamped_lvl);
}

static void light_set(struct bt_mesh_lightness_srv *srv,
		      struct bt_mesh_msg_ctx *ctx,
		      const struct bt_mesh_lightness_set *set,
		      struct bt_mesh_lightness_status *rsp)
{
	struct lightness_ctx *l_ctx =
		CONTAINER_OF(srv, struct lightness_ctx, lightness_srv);

	start_new_light_trans(set, l_ctx);
	rsp->current = l_ctx->rem_time ? l_ctx->current_lvl : l_ctx->target_lvl;
	rsp->target = l_ctx->target_lvl;
	rsp->remaining_time = set->transition ? set->transition->time : 0;
}

static void light_get(struct bt_mesh_lightness_srv *srv,
		      struct bt_mesh_msg_ctx *ctx,
		      struct bt_mesh_lightness_status *rsp)
{
	struct lightness_ctx *l_ctx =
		CONTAINER_OF(srv, struct lightness_ctx, lightness_srv);

	rsp->current = bt_mesh_lightness_clamp(&l_ctx->lightness_srv, l_ctx->current_lvl);
	rsp->target = l_ctx->target_lvl;
	rsp->remaining_time = l_ctx->rem_time;
}

static const struct bt_mesh_lightness_srv_handlers lightness_srv_handlers = {
	.light_set = light_set,
	.light_get = light_get,
};

static struct lightness_ctx my_ctx = {
	.lightness_srv = BT_MESH_LIGHTNESS_SRV_INIT(&lightness_srv_handlers),

};

static int dummy_energy_use;

static int energy_use_get(struct bt_mesh_sensor_srv *srv,
			 struct bt_mesh_sensor *sensor,
			 struct bt_mesh_msg_ctx *ctx,
			 struct bt_mesh_sensor_value *rsp)
{
	/* Report energy usage as dummy value, and increase it by one every time
	 * a get callback is triggered. The logic and hardware for measuring
	 * the actual energy usage of the device should be implemented here.
	 */
	(void)bt_mesh_sensor_value_from_micro(sensor->type->channels[0].format,
					      dummy_energy_use * 1000000LL, rsp);
	dummy_energy_use++;

	return 0;
}

static const struct bt_mesh_sensor_descriptor energy_use_desc = {
	.tolerance = {
		.negative = 0,
		.positive = 0,
	},
	.sampling_type = BT_MESH_SENSOR_SAMPLING_UNSPECIFIED,
	.period = 0,
	.update_interval = 0,
};

static struct bt_mesh_sensor energy_use = {
	.type = &bt_mesh_sensor_precise_tot_dev_energy_use,
	.get = energy_use_get,
	.descriptor = &energy_use_desc,
};

static struct bt_mesh_sensor *const sensors[] = {
	&energy_use,
};

static struct bt_mesh_sensor_srv sensor_srv =
	BT_MESH_SENSOR_SRV_INIT(sensors, ARRAY_SIZE(sensors));

static struct bt_mesh_scene_srv scene_srv;

static struct bt_mesh_light_ctrl_srv light_ctrl_srv =
	BT_MESH_LIGHT_CTRL_SRV_INIT(&my_ctx.lightness_srv);

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(1,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_CFG_SRV,
			     BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
			     BT_MESH_MODEL_LIGHTNESS_SRV(
					 &my_ctx.lightness_srv),
			     BT_MESH_MODEL_SCENE_SRV(&scene_srv),
			     BT_MESH_MODEL_SENSOR_SRV(&sensor_srv)),
		     BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(2,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_LIGHT_CTRL_SRV(&light_ctrl_srv)),
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
	k_work_init_delayable(&my_ctx.per_work, periodic_led_work);

	return &comp;
}

void model_handler_start(void)
{
	int err;

#if IS_ENABLED(CONFIG_BT_MESH_NLC_PERF_CONF)
	if (bt_mesh_comp2_register(&comp_p2)) {
		printk("Failed to register comp2\n");
	}
#endif

	if (bt_mesh_is_provisioned()) {
		return;
	}

	bt_mesh_ponoff_srv_set(&light_ctrl_srv.lightness->ponoff,
			       BT_MESH_ON_POWER_UP_RESTORE);

	err = bt_mesh_light_ctrl_srv_enable(&light_ctrl_srv);
	if (!err) {
		printk("Successfully enabled LC server\n");
	}
}

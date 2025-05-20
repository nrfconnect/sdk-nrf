/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Model handler for the Light Dimmer Controller.
 *
 */
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh/proxy.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"
#include <time.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dimmer_sample, LOG_LEVEL_INF);

#define SHORT_PRESS_THRESHOLD_MS 300
#define LONG_PRESS_TIMEOUT_SEC K_SECONDS(20)
#define DELTA_MOVE_STEP 1000
#define SCENE_COUNT 4

enum dim_direction { DIM_DOWN, DIM_UP };

/** Context for a single Light Dimmer instance. */
struct dimmer_ctx {
	/** Generic OnOff client instance for this switch */
	struct bt_mesh_onoff_cli onoff_client;
	/** Generic Level client instance for this switch */
	struct bt_mesh_lvl_cli lvl_client;
	/* Relative start time of the latest press of the button */
	int64_t press_start_time;
	/* Dimmer direction */
	enum dim_direction direction;
	/* Long press work (Dimmer mode) */
	struct k_work_delayable long_press_start_work;
	struct k_work_delayable long_press_stop_work;
};

/** Context for a single scene button instance */
struct scene_btn_ctx {
	/** Scene client instance for this button */
	struct bt_mesh_scene_cli client;
	/** The currently recalled scene*/
	uint16_t current_scene;
	/** Time when button was pressed */
	int64_t press_start_time;
};

#if IS_ENABLED(CONFIG_BT_MESH_NLC_PERF_CONF)
static const uint8_t cmp2_elem_offset[1] = { 0 };

static const struct bt_mesh_comp2_record comp_rec[2] = {
	{/* Basic Scene Selector NLC Profile 1.0.1*/
	.id = BT_MESH_NLC_PROFILE_ID_BASIC_SCENE_SELECTOR,
	.version.x = 1,
	.version.y = 0,
	.version.z = 1,
	.elem_offset_cnt = 1,
	.elem_offset = cmp2_elem_offset,
	.data_len = 0
	},
	{/* Dimming Control NLC Profile 1.0.1 */
	.id = BT_MESH_NLC_PROFILE_ID_DIMMING_CONTROL,
	.version.x = 1,
	.version.y = 0,
	.version.z = 1,
	.elem_offset_cnt = 1,
	.elem_offset = cmp2_elem_offset,
	.data_len = 0
	}
};

static const struct bt_mesh_comp2 comp_p2 = {
	.record_cnt = 2,
	.record = comp_rec
};
#endif

static void dimmer_start(struct k_work *work)
{
	struct dimmer_ctx *ctx = CONTAINER_OF(k_work_delayable_from_work(work), struct dimmer_ctx,
					      long_press_start_work);

	struct bt_mesh_lvl_move_set move_set = {
		.delta = (ctx->direction == DIM_UP) ? DELTA_MOVE_STEP :
						      (int16_t)(-1 * DELTA_MOVE_STEP),
		.transition = &(struct bt_mesh_model_transition){ .time = 500 },
	};

	LOG_INF("Start dimming %s...", (ctx->direction == DIM_UP ? "UP" : "DOWN"));

	/* As we can't know how many nodes are in a group, it doesn't
	 * make sense to send acknowledged messages to group addresses.
	 */
	if (bt_mesh_model_pub_is_unicast(ctx->lvl_client.model)) {
		bt_mesh_lvl_cli_move_set(&ctx->lvl_client, NULL, &move_set, NULL);
	} else {
		bt_mesh_lvl_cli_move_set_unack(&ctx->lvl_client, NULL, &move_set);
	}

	k_work_reschedule(&ctx->long_press_stop_work, LONG_PRESS_TIMEOUT_SEC);
}

static void dimmer_stop(struct k_work *work)
{
	struct dimmer_ctx *ctx = CONTAINER_OF(k_work_delayable_from_work(work), struct dimmer_ctx,
					      long_press_stop_work);

	struct bt_mesh_lvl_move_set move_set = {
		.delta = 0,
		.transition = NULL,
	};

	if (bt_mesh_model_pub_is_unicast(ctx->lvl_client.model)) {
		bt_mesh_lvl_cli_move_set(&ctx->lvl_client, NULL, &move_set, NULL);
	} else {
		bt_mesh_lvl_cli_move_set_unack(&ctx->lvl_client, NULL, &move_set);
	}
}

static struct dimmer_ctx dimmer = {
	.lvl_client = BT_MESH_LVL_CLI_INIT(NULL),
	.onoff_client = BT_MESH_ONOFF_CLI_INIT(NULL),
	.press_start_time = 0,
	.direction = DIM_DOWN
};

static void dimmer_button_handler(struct dimmer_ctx *ctx, bool pressed)
{
	int err;

	if (pressed) {
		ctx->direction = (ctx->direction == DIM_UP) ? DIM_DOWN : DIM_UP;

		ctx->press_start_time = k_uptime_get();
		err = k_work_reschedule(&ctx->long_press_start_work,
					K_MSEC(SHORT_PRESS_THRESHOLD_MS));

		if (err < 0) {
			LOG_ERR("Scheduling dimming to start failed: %d", err);
		}
	} else {
		if ((k_uptime_get() - ctx->press_start_time) >= SHORT_PRESS_THRESHOLD_MS) {
			/** Button was released after a long time.
			 * Stop dimming immediately.
			 */
			err = k_work_reschedule(&ctx->long_press_stop_work, K_NO_WAIT);

			if (err < 0) {
				LOG_INF("Scheduling dimming to stop failed: %d", err);
			}

		} else {
			/** Button was released after a short time.
			 * Cancel dimming operation and toggle light on/off.
			 */
			LOG_INF("Toggle light %s", (ctx->direction == DIM_UP ? "ON" : "OFF"));
			k_work_cancel_delayable(&ctx->long_press_start_work);

			struct bt_mesh_onoff_set set = {
				.on_off = ctx->direction,
			};

			/* As we can't know how many nodes are in a group, it doesn't
			 * make sense to send acknowledged messages to group addresses.
			 */
			if (bt_mesh_model_pub_is_unicast(ctx->onoff_client.model)) {
				err = bt_mesh_onoff_cli_set(&ctx->onoff_client, NULL, &set, NULL);
			} else {
				err = bt_mesh_onoff_cli_set_unack(&ctx->onoff_client, NULL, &set);
			}

			if (err) {
				LOG_ERR("Failed to send OnOff set message: %d", err);
			}
		}
	}
}

static struct scene_btn_ctx scene_btn = {
	.current_scene = 1,
	.press_start_time = 0
};

static void scene_button_handler(struct scene_btn_ctx *ctx, bool pressed)
{
	if (pressed) {
		ctx->press_start_time = k_uptime_get();
	} else {
		int err;

		if (k_uptime_get() - ctx->press_start_time > SHORT_PRESS_THRESHOLD_MS) {
			/* Long press, store scene */
			LOG_INF("Storing scene %d", ctx->current_scene);
			err = bt_mesh_scene_cli_store_unack(&ctx->client, NULL,
							    ctx->current_scene);

			if (err) {
				LOG_ERR("Failed to send Scene Store message: %d", err);
			}
		} else {
			/* Short press, recall next scene */
			ctx->current_scene = (ctx->current_scene >= SCENE_COUNT) ?
					     1 : ctx->current_scene + 1;
			LOG_INF("Recalling scene %d", ctx->current_scene);
			err = bt_mesh_scene_cli_recall_unack(&ctx->client, NULL,
							     ctx->current_scene, NULL);

			if (err) {
				LOG_ERR("Failed to send Scene Recall message: %d", err);
			}
		}
	}
}

static void button_handler_cb(uint32_t pressed, uint32_t changed)
{
	if (!bt_mesh_is_provisioned()) {
		LOG_ERR("Node is not provisioned");
		return;
	}

	if (changed & BIT(0)) {
		dimmer_button_handler(&dimmer, pressed & BIT(0));
	}
	if (changed & BIT(1)) {
		scene_button_handler(&scene_btn, pressed & BIT(1));
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
	const uint8_t pattern[] = {
#if DT_NODE_EXISTS(DT_ALIAS(sw0))
		BIT(0),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw1))
		BIT(1),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw2))
		BIT(2),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw3))
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

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(1,
		     BT_MESH_MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
					BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
					BT_MESH_MODEL_ONOFF_CLI(&dimmer.onoff_client),
					BT_MESH_MODEL_LVL_CLI(&dimmer.lvl_client),
					BT_MESH_MODEL_SCENE_CLI(&scene_btn.client)),
		     BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(void)
{
#if IS_ENABLED(CONFIG_BT_MESH_NLC_PERF_CONF)
	if (bt_mesh_comp2_register(&comp_p2)) {
		printf("Failed to register comp2\n");
	}
#endif

	static struct button_handler button_handler = {
		.cb = button_handler_cb,
	};

	dk_button_handler_add(&button_handler);
	k_work_init_delayable(&attention_blink_work, attention_blink);
	k_work_init_delayable(&dimmer.long_press_start_work, dimmer_start);
	k_work_init_delayable(&dimmer.long_press_stop_work, dimmer_stop);

	return &comp;
}

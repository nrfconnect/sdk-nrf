/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"

#define GET_DATA_INTERVAL 3000

static void sensor_cli_data_cb(struct bt_mesh_sensor_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_sensor_type *sensor,
			       const struct sensor_value *value)
{
	if (sensor->id == bt_mesh_sensor_present_dev_op_temp.id) {
		printk("Chip temperature: %s\n",
		       bt_mesh_sensor_ch_str(value));
	} else if (sensor->id == bt_mesh_sensor_presence_detected.id) {
		if (value->val1) {
			printk("Precense detected\n");
		} else {
			printk("End of presence\n");
		}
	} else if (sensor->id ==
		   bt_mesh_sensor_time_since_presence_detected.id) {
		if (value->val1) {
			printk("%s second(s) since last presence detected\n",
			       bt_mesh_sensor_ch_str(value));
		} else {
			printk("No presence detected, or under 1 second since presence detected\n");
		}
	}
}

static void sensor_cli_series_entry_cb(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_sensor_type *sensor, uint8_t index, uint8_t count,
	const struct bt_mesh_sensor_series_entry *entry)
{
	printk("Relative runtime in %d to %d degrees: %s percent\n",
	       entry->value[1].val1, entry->value[2].val1,
	       bt_mesh_sensor_ch_str(&entry->value[0]));
}

static const struct bt_mesh_sensor_cli_handlers bt_mesh_sensor_cli_handlers = {
	.data = sensor_cli_data_cb,
	.series_entry = sensor_cli_series_entry_cb,
};

static struct bt_mesh_sensor_cli sensor_cli =
	BT_MESH_SENSOR_CLI_INIT(&bt_mesh_sensor_cli_handlers);

static struct k_work_delayable get_data_work;

static void get_data(struct k_work *work)
{
	static bool req_temp_range;
	int err;

	/* Only one message can be published at a time. Swap sensor after each timeout. */
	if (req_temp_range) {
		err = bt_mesh_sensor_cli_series_entries_get(
			&sensor_cli, NULL,
			&bt_mesh_sensor_rel_runtime_in_a_dev_op_temp_range, NULL, NULL,
			NULL);
		if (err) {
			printk("Error getting relative chip temperature data (%d)\n",
			       err);
		}
	} else {
		err = bt_mesh_sensor_cli_get(
			&sensor_cli, NULL, &bt_mesh_sensor_time_since_presence_detected,
			NULL);
		if (err) {
			printk("Error getting time since presence detected (%d)\n",
			       err);
		}
	}

	req_temp_range = !req_temp_range;

	k_work_schedule(&get_data_work, K_MSEC(GET_DATA_INTERVAL));
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
		BIT(0) | BIT(1),
		BIT(1) | BIT(2),
		BIT(2) | BIT(3),
		BIT(3) | BIT(0),
	};

	if (attention) {
		dk_set_leds(pattern[idx++ % ARRAY_SIZE(pattern)]);
		k_work_reschedule(&attention_blink_work, K_MSEC(30));
	} else {
		dk_set_leds(DK_NO_LEDS_MSK);
	}
}

static void attention_on(struct bt_mesh_model *mod)
{
	attention = true;
	k_work_reschedule(&attention_blink_work, K_NO_WAIT);
}

static void attention_off(struct bt_mesh_model *mod)
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
					BT_MESH_MODEL_HEALTH_SRV(&health_srv,
								 &health_pub),
					BT_MESH_MODEL_SENSOR_CLI(&sensor_cli)),
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
	k_work_init_delayable(&get_data_work, get_data);

	k_work_schedule(&get_data_work, K_MSEC(GET_DATA_INTERVAL));

	return &comp;
}

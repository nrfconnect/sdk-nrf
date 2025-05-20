/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"

#define GET_DATA_INTERVAL 60000
#define GET_DATA_INTERVAL_QUICK 3000
#define MOTION_TIMEOUT K_SECONDS(60)

static bool is_occupied;
static struct k_work_delayable motion_timeout_work;

static void motion_timeout(struct k_work *work)
{
	is_occupied = false;
	printk("Area is now vacant.\n");
}

static void sensor_cli_data_cb(struct bt_mesh_sensor_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_sensor_type *sensor,
			       const struct bt_mesh_sensor_value *value)
{
	enum bt_mesh_sensor_value_status status;

	if (sensor->id == bt_mesh_sensor_present_dev_op_temp.id) {
		printk("Chip temperature: %s\n", bt_mesh_sensor_ch_str(value));
	} else if (sensor->id == bt_mesh_sensor_presence_detected.id) {
		int64_t presence_detected = 0;

		status = bt_mesh_sensor_value_to_micro(value, &presence_detected);
		if (!bt_mesh_sensor_value_status_is_numeric(status)) {
			printk("Warning: unexpected behaviour during conversion of presence "
			       "detected value (%d)\n",
			       status);
			return;
		}
		if (!!presence_detected) {
			printk("Presence detected\n");
		} else {
			printk("No presence detected\n");
		}
	} else if (sensor->id ==
		   bt_mesh_sensor_time_since_presence_detected.id) {
		int64_t time_since_presence_detected = 0;

		status = bt_mesh_sensor_value_to_micro(value, &time_since_presence_detected);
		if (status == BT_MESH_SENSOR_VALUE_UNKNOWN) {
			printk("Unknown last presence detected\n");
		} else if (!time_since_presence_detected) {
			printk("Presence detected, or under 1 second since presence detected\n");
		} else {
			printk("%s second(s) since last presence detected\n",
			       bt_mesh_sensor_ch_str(value));
		}
	} else if (sensor->id == bt_mesh_sensor_present_amb_light_level.id) {
		printk("Ambient light level: %s\n", bt_mesh_sensor_ch_str(value));
	} else if (sensor->id == bt_mesh_sensor_motion_sensed.id) {
		int64_t motion_sensed = 0;

		status = bt_mesh_sensor_value_to_micro(value, &motion_sensed);
		if (!bt_mesh_sensor_value_status_is_numeric(status)) {
			printk("Warning: unexpected behaviour during conversion of motion sensed "
			       "value (%d)\n",
			       status);
			return;
		}
		if (!!motion_sensed) {
			is_occupied = true;
			printk("Motion detected (%s %%). Area is occupied.\n",
			       bt_mesh_sensor_ch_str(value));
			k_work_cancel_delayable(&motion_timeout_work);
		} else {
			if (is_occupied) {
				printk("No current motion detected. Area is still occupied.\n");
				k_work_reschedule(&motion_timeout_work, MOTION_TIMEOUT);
			} else {
				printk("No motion detected. Area is vacant.\n");
			}
		}
	} else if (sensor->id == bt_mesh_sensor_time_since_motion_sensed.id) {
		int64_t time_since_motion_sensed = 0;

		status = bt_mesh_sensor_value_to_micro(value, &time_since_motion_sensed);
		if (status == BT_MESH_SENSOR_VALUE_UNKNOWN) {
			printk("Unknown last motion sensed\n");
		} else if (!time_since_motion_sensed) {
			printk("Motion sensed, or under 1 second since motion sensed\n");
		} else {
			printk("%s second(s) since last motion sensed\n",
			       bt_mesh_sensor_ch_str(value));
		}
	} else if (sensor->id == bt_mesh_sensor_people_count.id) {
		printk("People count is %s\n", bt_mesh_sensor_ch_str(value));
	}
}

static void sensor_cli_series_entry_cb(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_sensor_type *sensor, uint8_t index, uint8_t count,
	const struct bt_mesh_sensor_series_entry *entry)
{
	printk("Relative runtime in %s", bt_mesh_sensor_ch_str(&entry->value[1]));
	printk(" to %s degrees: ", bt_mesh_sensor_ch_str(&entry->value[2]));
	printk("%s percent\n", bt_mesh_sensor_ch_str(&entry->value[0]));
}

static void sensor_cli_setting_status_cb(struct bt_mesh_sensor_cli *cli,
					 struct bt_mesh_msg_ctx *ctx,
					 const struct bt_mesh_sensor_type *sensor,
					 const struct bt_mesh_sensor_setting_status *setting)
{
	printk("Sensor ID: 0x%04x, Setting ID: 0x%04x\n", sensor->id, setting->type->id);
	for (int chan = 0; chan < setting->type->channel_count; chan++) {
		printk("\tChannel %d value: %s\n", chan,
		       bt_mesh_sensor_ch_str(&(setting->value[chan])));
	}
}

static void sensor_cli_desc_cb(struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_sensor_info *sensor)
{
	printk("Descriptor of sensor with ID 0x%04x:\n", sensor->id);
	printk("\ttolerance: { positive: %d negative: %d }\n",
	       sensor->descriptor.tolerance.positive, sensor->descriptor.tolerance.negative);
	printk("\tsampling type: %d\n", sensor->descriptor.sampling_type);
}

static const struct bt_mesh_sensor_cli_handlers bt_mesh_sensor_cli_handlers = {
	.data = sensor_cli_data_cb,
	.series_entry = sensor_cli_series_entry_cb,
	.setting_status = sensor_cli_setting_status_cb,
	.sensor = sensor_cli_desc_cb,
};

static struct bt_mesh_sensor_cli sensor_cli =
	BT_MESH_SENSOR_CLI_INIT(&bt_mesh_sensor_cli_handlers);

static struct k_work_delayable get_data_work;

static void get_data(struct k_work *work)
{
	if (!bt_mesh_is_provisioned()) {
		k_work_schedule(&get_data_work, K_MSEC(GET_DATA_INTERVAL));
		return;
	}

	static uint32_t sensor_idx;
	int err;

	/* Only one message can be published at a time. Swap sensor after each timeout. */
	switch (sensor_idx++) {
	case (0): {
		err = bt_mesh_sensor_cli_get(
			&sensor_cli, NULL, &bt_mesh_sensor_present_dev_op_temp,
			NULL);
		if (err) {
			printk("Error getting chip temperature (%d)\n", err);
		}
		break;
	}
	case (1): {
		err = bt_mesh_sensor_cli_series_entries_get(
			&sensor_cli, NULL,
			&bt_mesh_sensor_rel_runtime_in_a_dev_op_temp_range, NULL, NULL,
			NULL, NULL);
		if (err) {
			printk("Error getting relative chip temperature data (%d)\n", err);
		}
		break;
	}
	case (2): {
		err = bt_mesh_sensor_cli_get(
			&sensor_cli, NULL, &bt_mesh_sensor_time_since_presence_detected,
			NULL);
		if (err) {
			printk("Error getting time since presence detected (%d)\n", err);
		}
		break;
	}
	case (3): {
		err = bt_mesh_sensor_cli_get(
			&sensor_cli, NULL, &bt_mesh_sensor_present_amb_light_level, NULL);
		if (err) {
			printk("Error getting ambient light level (%d)\n", err);
		}
		break;
	}
	case (4): {
		err = bt_mesh_sensor_cli_get(&sensor_cli, NULL,
					     &bt_mesh_sensor_time_since_motion_sensed, NULL);
		if (err) {
			printk("Error getting time since motion detected (%d)\n", err);
		}
		break;
	}
	case (5): {
		err = bt_mesh_sensor_cli_get(&sensor_cli, NULL, &bt_mesh_sensor_people_count, NULL);
		if (err) {
			printk("Error getting people count (%d)\n", err);
		}
		break;
	}
	}

	if (sensor_idx % 6) {
		k_work_schedule(&get_data_work, K_MSEC(GET_DATA_INTERVAL_QUICK));
	} else {
		k_work_schedule(&get_data_work, K_MSEC(GET_DATA_INTERVAL));
		sensor_idx = 0;
	}
}

static const int temp_ranges[][2] = {
	{ 0, 100 },
	{ 10, 20 },
	{ 22, 30 },
	{ 40, 50 },
};

static const int presence_motion_threshold[] = { 0, 25, 50, 75, 100 };

static int setting_set_int(const struct bt_mesh_sensor_type *sensor_type,
			   const struct bt_mesh_sensor_type *setting_type,
			   const int *values)
{
	struct bt_mesh_sensor_value sensor_vals[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
	int err;

	for (int i = 0; i < setting_type->channel_count; i++) {
		err = bt_mesh_sensor_value_from_micro(setting_type->channels[i].format,
						      values[i] * 1000000LL, &sensor_vals[i]);
		if (err) {
			return err;
		}
	}

	return bt_mesh_sensor_cli_setting_set(&sensor_cli, NULL, sensor_type,
					      setting_type, sensor_vals, NULL);
}

static void button_handler_cb(uint32_t pressed, uint32_t changed)
{
	if (!bt_mesh_is_provisioned()) {
		return;
	}

	static uint32_t temp_idx;
	static uint32_t motion_threshold_idx;
	int err;

	if (pressed & changed & BIT(0)) {
		err = bt_mesh_sensor_cli_setting_get(&sensor_cli, NULL,
						     &bt_mesh_sensor_present_dev_op_temp,
						     &bt_mesh_sensor_dev_op_temp_range_spec, NULL);
		if (err) {
			printk("Error getting range setting (%d)\n", err);
		}
	}
	if (pressed & changed & BIT(1)) {
		err = setting_set_int(&bt_mesh_sensor_present_dev_op_temp,
				      &bt_mesh_sensor_dev_op_temp_range_spec,
				      temp_ranges[temp_idx++]);
		if (err) {
			printk("Error setting range setting (%d)\n", err);
		}
		temp_idx = temp_idx % ARRAY_SIZE(temp_ranges);
	}
	if (pressed & changed & BIT(2)) {
		err = bt_mesh_sensor_cli_desc_get(&sensor_cli, NULL,
						  &bt_mesh_sensor_present_dev_op_temp, NULL);
		if (err) {
			printk("Error getting sensor descriptor (%d)\n", err);
		}
	}
	if (pressed & changed & BIT(3)) {
		err = setting_set_int(&bt_mesh_sensor_presence_detected,
				      &bt_mesh_sensor_motion_threshold,
				      &presence_motion_threshold[motion_threshold_idx++]);
		if (err) {
			printk("Error setting motion threshold setting (%d)\n", err);
		}
		motion_threshold_idx = motion_threshold_idx % ARRAY_SIZE(presence_motion_threshold);
	}
}

static struct button_handler button_handler = {
	.cb = button_handler_cb,
};

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
	k_work_init_delayable(&motion_timeout_work, motion_timeout);

	dk_button_handler_add(&button_handler);
	k_work_schedule(&get_data_work, K_MSEC(GET_DATA_INTERVAL));

	return &comp;
}

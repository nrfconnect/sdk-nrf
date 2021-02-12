/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"

/* The columns (temperature ranges) for relative
 * runtime in a chip temperature
 */
static const struct bt_mesh_sensor_column columns[] = {
	{ { 0 }, { 20 } },
	{ { 20 }, { 25 } },
	{ { 25 }, { 30 } },
	{ { 30 }, { 100 } },
};

static const struct device *dev;
static uint32_t tot_temp_samps;
static uint32_t col_samps[ARRAY_SIZE(columns)];

static int32_t prev_pres;

static int chip_temp_get(struct bt_mesh_sensor *sensor,
			 struct bt_mesh_msg_ctx *ctx, struct sensor_value *rsp)
{
	int err;

	sensor_sample_fetch(dev);

	err = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, rsp);

	if (err) {
		printk("Error getting temperature sensor data (%d)\n", err);
	}

	for (int i = 0; i < ARRAY_SIZE(columns); ++i) {
		if (bt_mesh_sensor_value_in_column(rsp, &columns[i])) {
			col_samps[i]++;
			break;
		}
	}

	tot_temp_samps++;

	return err;
}

static struct bt_mesh_sensor chip_temp = {
	.type = &bt_mesh_sensor_present_dev_op_temp,
	.get = chip_temp_get,
};

static int relative_runtime_in_chip_temp_get(
	struct bt_mesh_sensor *sensor, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_sensor_column *column, struct sensor_value *value)
{
	if (tot_temp_samps) {
		int32_t index = column - &columns[0];
		uint8_t percent_steps =
			(200 * col_samps[index]) / tot_temp_samps;

		value[0].val1 = percent_steps / 2;
		value[0].val2 = (percent_steps % 2) * 500000;
	} else {
		value[0].val1 = 0;
		value[0].val2 = 0;
	}

	value[1] = column->start;
	value[2] = column->end;

	return 0;
}

static struct bt_mesh_sensor rel_chip_temp_runtime = {
	.type = &bt_mesh_sensor_rel_runtime_in_a_dev_op_temp_range,
	.series = {
		columns,
		ARRAY_SIZE(columns),
		relative_runtime_in_chip_temp_get,
		},
};

static struct bt_mesh_sensor presence_sensor = {
	.type = &bt_mesh_sensor_presence_detected,
};

static int time_since_presence_detected_get(struct bt_mesh_sensor *sensor,
					    struct bt_mesh_msg_ctx *ctx,
					    struct sensor_value *rsp)
{
	if (prev_pres) {
		rsp->val1 = (k_uptime_get_32() - prev_pres) / MSEC_PER_SEC;
	} else {
		rsp->val1 = 0;
	}

	return 0;
}

static struct bt_mesh_sensor time_since_presence_detected = {
	.type = &bt_mesh_sensor_time_since_presence_detected,
	.get = time_since_presence_detected_get,
};

static struct bt_mesh_sensor *const sensors[] = {
	&chip_temp,
	&rel_chip_temp_runtime,
	&presence_sensor,
	&time_since_presence_detected,
};

static struct bt_mesh_sensor_srv sensor_srv =
	BT_MESH_SENSOR_SRV_INIT(sensors, ARRAY_SIZE(sensors));

static struct k_delayed_work end_of_presence_work;

static void end_of_presence(struct k_work *work)
{
	int err;

	/* This sensor value must be boolean -
	 * .val1 can only be '0' or '1'
	 */
	struct sensor_value val = {
		.val1 = 0,
	};

	err = bt_mesh_sensor_srv_pub(&sensor_srv, NULL, &presence_sensor, &val);

	if (err) {
		printk("Error publishing end of presence (%d)\n", err);
	}
}

static void button_handler_cb(uint32_t pressed, uint32_t changed)
{
	if ((pressed & BIT(0))) {
		int err;

		/* This sensor value must be boolean -
		 * .val1 can only be '0' or '1'
		 */
		struct sensor_value val = {
			.val1 = 1,
		};

		err = bt_mesh_sensor_srv_pub(&sensor_srv, NULL,
					     &presence_sensor, &val);

		if (err) {
			printk("Error publishing presence (%d)\n", err);
		}

		prev_pres = k_uptime_get_32();

		k_delayed_work_submit(&end_of_presence_work, K_MSEC(2000));
	}
}

static struct button_handler button_handler = {
	.cb = button_handler_cb,
};

/* Set up a repeating delayed work to blink the DK's LEDs when attention is
 * requested.
 */
static struct k_delayed_work attention_blink_work;

static void attention_blink(struct k_work *work)
{
	static int idx;
	const uint8_t pattern[] = {
		BIT(0) | BIT(1),
		BIT(1) | BIT(2),
		BIT(2) | BIT(3),
		BIT(3) | BIT(0),
	};
	dk_set_leds(pattern[idx++ % ARRAY_SIZE(pattern)]);
	k_delayed_work_submit(&attention_blink_work, K_MSEC(30));
}

static void attention_on(struct bt_mesh_model *mod)
{
	k_delayed_work_submit(&attention_blink_work, K_NO_WAIT);
}

static void attention_off(struct bt_mesh_model *mod)
{
	k_delayed_work_cancel(&attention_blink_work);
	dk_set_leds(DK_NO_LEDS_MSK);
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
					BT_MESH_MODEL_SENSOR_SRV(&sensor_srv)),
		     BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(void)
{
	k_delayed_work_init(&attention_blink_work, attention_blink);
	k_delayed_work_init(&end_of_presence_work, end_of_presence);

	dev = device_get_binding(DT_PROP(DT_NODELABEL(temp), label));

	if (dev == NULL) {
		printk("Could not initiate temperature sensor\n");
	} else {
		printk("Temperature sensor (%s) initiated\n", dev->name);
	}

	dk_button_handler_add(&button_handler);

	return &comp;
}

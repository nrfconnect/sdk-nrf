/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <bluetooth/mesh/sensor_types.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"

#if DT_NODE_HAS_STATUS(DT_NODELABEL(bme680), okay)
/** Thingy53 */
#define SENSOR_NODE DT_NODELABEL(bme680)
#define SENSOR_DATA_TYPE SENSOR_CHAN_AMBIENT_TEMP
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(temp), okay)
/** nRF52 DK */
#define SENSOR_NODE DT_NODELABEL(temp)
#define SENSOR_DATA_TYPE SENSOR_CHAN_DIE_TEMP
#else
#error "Unsupported board!"
#endif

/* The columns (temperature ranges) for relative
 * runtime in a chip temperature
 */
static const struct bt_mesh_sensor_column columns[] = {
	{ { 0 }, { 20 } },
	{ { 20 }, { 25 } },
	{ { 25 }, { 30 } },
	{ { 30 }, { 100 } },
};

#define DEFAULT_TEMP_RANGE_LOW 0
#define DEFAULT_TEMP_RANGE_HIGH 100
/* Range limiting the reported values of the chip temperature */
static struct bt_mesh_sensor_column temp_range = {
	.start = { .val1 = DEFAULT_TEMP_RANGE_LOW, .val2 = 0},
	.end = { .val1 = DEFAULT_TEMP_RANGE_HIGH, .val2 = 0},
};

static const struct device *dev = DEVICE_DT_GET(SENSOR_NODE);
static uint32_t tot_temp_samps;
static uint32_t col_samps[ARRAY_SIZE(columns)];
static struct sensor_value pres_mot_thres = { .val1 = 25, .val2 = 0 };

static int32_t pres_detect;
static uint32_t prev_detect;

static int chip_temp_get(struct bt_mesh_sensor_srv *srv,
			 struct bt_mesh_sensor *sensor,
			 struct bt_mesh_msg_ctx *ctx,
			 struct sensor_value *rsp)
{
	int err;

	sensor_sample_fetch(dev);

	err = sensor_channel_get(dev, SENSOR_DATA_TYPE, rsp);

	if (err) {
		printk("Error getting temperature sensor data (%d)\n", err);
	}

	if (!bt_mesh_sensor_value_in_column(rsp, &temp_range)) {
		if (temp_range.start.val1 == temp_range.end.val1) {
			if (rsp->val2 <= temp_range.start.val2) {
				*rsp = temp_range.start;
			} else {
				*rsp = temp_range.end;
			}
		} else if (rsp->val1 <= temp_range.start.val1) {
			*rsp = temp_range.start;
		} else {
			*rsp = temp_range.end;
		}
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

/* Tolerance is based on the nRF52832's temperature sensor's accuracy and range (5/125 = 4%). */
static const struct bt_mesh_sensor_descriptor chip_temp_descriptor = {
	.tolerance = {
		.negative = {
			.val1 = 4,
		},
		.positive = {
			.val1 = 4,
		}
	},
	.sampling_type = BT_MESH_SENSOR_SAMPLING_INSTANTANEOUS,
};

static void chip_temp_range_get(struct bt_mesh_sensor_srv *srv, struct bt_mesh_sensor *sensor,
				const struct bt_mesh_sensor_setting *setting,
				struct bt_mesh_msg_ctx *ctx, struct sensor_value *rsp)
{
	rsp[0] = temp_range.start;
	rsp[1] = temp_range.end;
	printk("Temperature sensor lower limit: %u\n", rsp[0].val1);
	printk("Temperature sensor upper limit: %u\n", rsp[1].val1);
}

static int chip_temp_range_set(struct bt_mesh_sensor_srv *srv, struct bt_mesh_sensor *sensor,
			       const struct bt_mesh_sensor_setting *setting,
			       struct bt_mesh_msg_ctx *ctx, const struct sensor_value *value)
{
	temp_range.start = value[0];
	temp_range.end = value[1];
	printk("Temperature sensor lower limit: %u\n", value[0].val1);
	printk("Temperature sensor upper limit: %u\n", value[1].val1);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		int err;

		err = settings_save_one("temp/range", &temp_range, sizeof(temp_range));
		if (err) {
			printk("Error storing setting (%d)\n", err);
		} else {
			printk("Stored setting\n");
		}
	}
	return 0;
}

static struct bt_mesh_sensor_setting chip_temp_setting[] = { {
	.type = &bt_mesh_sensor_dev_op_temp_range_spec,
	.get = chip_temp_range_get,
	.set = chip_temp_range_set,
} };

static int chip_temp_range_settings_restore(const char *name, size_t len, settings_read_cb read_cb,
					    void *cb_arg)
{
	const char *next;
	int rc;

	if (!(settings_name_steq(name, "range", &next) && !next)) {
		return -ENOENT;
	}

	if (len != sizeof(temp_range)) {
		return -EINVAL;
	}

	rc = read_cb(cb_arg, &temp_range, sizeof(temp_range));
	if (rc < 0) {
		return rc;
	}

	printk("Restored temperature range setting\n");
	return 0;
}

struct settings_handler temp_range_conf = { .name = "temp",
					    .h_set = chip_temp_range_settings_restore };

static struct bt_mesh_sensor chip_temp = {
	.type = &bt_mesh_sensor_present_dev_op_temp,
	.get = chip_temp_get,
	.descriptor = &chip_temp_descriptor,
	.settings = {
		.list = (const struct bt_mesh_sensor_setting *)&chip_temp_setting,
		.count = ARRAY_SIZE(chip_temp_setting),
	},
};

static int relative_runtime_in_chip_temp_get(struct bt_mesh_sensor_srv *srv,
	struct bt_mesh_sensor *sensor,
	struct bt_mesh_msg_ctx *ctx,
	uint32_t column_index,
	struct sensor_value *value)
{
	if (tot_temp_samps) {
		uint8_t percent_steps =
			(200 * col_samps[column_index]) / tot_temp_samps;

		value[0].val1 = percent_steps / 2;
		value[0].val2 = (percent_steps % 2) * 500000;
	} else {
		value[0].val1 = 0;
		value[0].val2 = 0;
	}

	value[1] = columns[column_index].start;
	value[2] = columns[column_index].end;

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

static void presence_motion_threshold_get(struct bt_mesh_sensor_srv *srv,
	struct bt_mesh_sensor *sensor,
	const struct bt_mesh_sensor_setting *setting,
	struct bt_mesh_msg_ctx *ctx,
	struct sensor_value *rsp)
{
	rsp[0] = pres_mot_thres;
	printk("Presence motion threshold: %u [%d ms]\n", rsp[0].val1, 100 * rsp[0].val1);
}

static int presence_motion_threshold_set(struct bt_mesh_sensor_srv *srv,
	struct bt_mesh_sensor *sensor,
	const struct bt_mesh_sensor_setting *setting,
	struct bt_mesh_msg_ctx *ctx,
	const struct sensor_value *value)
{
	pres_mot_thres = value[0];
	printk("Presence motion threshold: %u [%d ms]\n", value[0].val1, 100 * value[0].val1);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		int err;

		err = settings_save_one("presence/motion_threshold",
					&pres_mot_thres, sizeof(pres_mot_thres));
		if (err) {
			printk("Error storing setting (%d)\n", err);
		} else {
			printk("Stored setting\n");
		}
	}
	return 0;
}

static struct bt_mesh_sensor_setting presence_motion_threshold_setting[] = { {
	.type = &bt_mesh_sensor_motion_threshold,
	.get = presence_motion_threshold_get,
	.set = presence_motion_threshold_set,
} };

static int presence_motion_threshold_settings_restore(const char *name,
	size_t len,
	settings_read_cb read_cb,
	void *cb_arg)
{
	const char *next;
	int rc;

	if (!(settings_name_steq(name, "motion_threshold", &next) && !next)) {
		return -ENOENT;
	}

	if (len != sizeof(pres_mot_thres)) {
		return -EINVAL;
	}

	rc = read_cb(cb_arg, &pres_mot_thres, sizeof(pres_mot_thres));
	if (rc < 0) {
		return rc;
	}

	printk("Restored presence motion threshold setting\n");
	return 0;
}

struct settings_handler presence_motion_threshold_conf = {
	.name = "presence",
	.h_set = presence_motion_threshold_settings_restore
};

static int presence_detected_get(struct bt_mesh_sensor_srv *srv,
				 struct bt_mesh_sensor *sensor,
				 struct bt_mesh_msg_ctx *ctx,
				 struct sensor_value *rsp)
{
	rsp->val1 = pres_detect;

	return 0;
};

static struct bt_mesh_sensor presence_sensor = {
	.type = &bt_mesh_sensor_presence_detected,
	.get = presence_detected_get,
	.settings = {
		.list = (const struct bt_mesh_sensor_setting *)&presence_motion_threshold_setting,
		.count = ARRAY_SIZE(presence_motion_threshold_setting),
	},
};

static int time_since_presence_detected_get(struct bt_mesh_sensor_srv *srv,
					struct bt_mesh_sensor *sensor,
					struct bt_mesh_msg_ctx *ctx,
					struct sensor_value *rsp)
{
	if (pres_detect) {
		rsp->val1 = 0;
	} else if (prev_detect) {
		rsp->val1 = (k_uptime_get_32() - prev_detect) / MSEC_PER_SEC;
	} else {
		/* Before first detection, the time since last detection is unknown. Returning
		 * unknown value until a detection is done. Value is defined in specification.
		 */
		rsp->val1 = 0xFFFF;
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

static struct k_work_delayable presence_detected_work;

static void presence_detected(struct k_work *work)
{
	int err;

	/* This sensor value must be boolean -
	 * .val1 can only be '0' or '1'
	 */
	struct sensor_value val = {
		.val1 = 1,
	};

	err = bt_mesh_sensor_srv_pub(&sensor_srv, NULL, &presence_sensor, &val);

	if (err) {
		printk("Error publishing end of presence (%d)\n", err);
	}

	pres_detect = 1;
}

static void button_handler_cb(uint32_t pressed, uint32_t changed)
{
	if (!bt_mesh_is_provisioned()) {
		return;
	}

	if (pressed & changed & BIT(0)) {
		k_work_reschedule(&presence_detected_work, K_MSEC(100 * pres_mot_thres.val1));
	}

	if ((!pressed) & changed & BIT(0)) {
		if (!pres_detect) {
			k_work_cancel_delayable(&presence_detected_work);
		} else {
			int err;

			/* This sensor value must be boolean -
			 * .val1 can only be '0' or '1'
			 */
			struct sensor_value val = {
				.val1 = 0,
			};

			err = bt_mesh_sensor_srv_pub(&sensor_srv, NULL,
						&presence_sensor, &val);

			if (err) {
				printk("Error publishing presence (%d)\n", err);
			}

			pres_detect = 0;
			prev_detect = k_uptime_get_32();
		}
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
	k_work_init_delayable(&attention_blink_work, attention_blink);
	k_work_init_delayable(&presence_detected_work, presence_detected);

	if (!device_is_ready(dev)) {
		printk("Temperature sensor not ready\n");
	} else {
		printk("Temperature sensor (%s) initiated\n", dev->name);
	}

	dk_button_handler_add(&button_handler);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		settings_subsys_init();
		settings_register(&temp_range_conf);
		settings_register(&presence_motion_threshold_conf);
	}

	return &comp;
}

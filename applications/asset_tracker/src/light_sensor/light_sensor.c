/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <spinlock.h>
#include "light_sensor.h"

enum ls_ch_type {
	LS_CH__BEGIN = 0,

	LS_CH_RED = LS_CH__BEGIN,
	LS_CH_GREEN,
	LS_CH_BLUE,
	LS_CH_IR,

	LS_CH__END
};

struct ls_ch_data {
	enum sensor_channel type;
	struct sensor_value data;
};

static struct ls_ch_data ls_ch_red = { .type = SENSOR_CHAN_RED };
static struct ls_ch_data ls_ch_green = { .type = SENSOR_CHAN_GREEN };
static struct ls_ch_data ls_ch_blue = { .type = SENSOR_CHAN_BLUE };
static struct ls_ch_data ls_ch_ir = { .type = SENSOR_CHAN_IR };
static char *ls_dev_name = CONFIG_LIGHT_SENSOR_DEV_NAME;
static struct device *ls_dev;
static struct k_spinlock ls_lock;
static struct ls_ch_data *ls_data[LS_CH__END] = { [LS_CH_RED] = &ls_ch_red,
						  [LS_CH_GREEN] = &ls_ch_green,
						  [LS_CH_BLUE] = &ls_ch_blue,
						  [LS_CH_IR] = &ls_ch_ir };
static light_sensor_data_ready_cb ls_cb;
static struct k_delayed_work ls_poller;

static void light_sensor_poll_fn(struct k_work *work);

int light_sensor_init_and_start(const light_sensor_data_ready_cb cb)
{
	if (!IS_ENABLED(CONFIG_LIGHT_SENSOR)) {
		return -ENXIO;
	}

	ls_dev = device_get_binding(ls_dev_name);

	if (ls_dev == NULL) {
		return -ENODEV;
	}

	ls_cb = cb;

	k_delayed_work_init(&ls_poller, light_sensor_poll_fn);

	return k_delayed_work_submit(&ls_poller, K_SECONDS(5));
}

int light_sensor_get_data(struct light_sensor_data *const data)
{
	if (data == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&ls_lock);

	data->red = ls_data[LS_CH_RED]->data.val1;
	data->green = ls_data[LS_CH_GREEN]->data.val1;
	data->blue = ls_data[LS_CH_BLUE]->data.val1;
	data->ir = ls_data[LS_CH_IR]->data.val1;

	k_spin_unlock(&ls_lock, key);

	return 0;
}

void light_sensor_poll_fn(struct k_work *work)
{
	k_spinlock_key_t key;

	int err = sensor_sample_fetch_chan(ls_dev, SENSOR_CHAN_ALL);

	if (err) {
		printk("Failed to fetch data from %s, error: %d\n", ls_dev_name,
		       err);
	}

	key = k_spin_lock(&ls_lock);

	for (enum ls_ch_type ch = LS_CH__BEGIN; ch < LS_CH__END; ++ch) {
		err = sensor_channel_get(ls_dev, ls_data[ch]->type,
					 &ls_data[ch]->data);
		if (err) {
			printk("Failed to get data from %s, sensor ch: %d , error: %d\n",
			       ls_dev_name, ls_data[ch]->type, err);
		}
	}

	k_spin_unlock(&ls_lock, key);

	if (ls_cb != NULL) {
		ls_cb();
	}

	k_delayed_work_submit(
		&ls_poller, K_SECONDS(CONFIG_LIGHT_SENSOR_DATA_SEND_INTERVAL));
}

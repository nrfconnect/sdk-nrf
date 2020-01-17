/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <spinlock.h>
#include "light_sensor.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(light_sensor, CONFIG_ASSET_TRACKER_LOG_LEVEL);

#define LS_INIT_DELAY_S (5) /* Polling delay upon initialization */
#define MAX_INTERVAL_S	(INT_MAX/MSEC_PER_SEC)

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
static u32_t data_send_interval_s = CONFIG_LIGHT_SENSOR_DATA_SEND_INTERVAL;
static bool initialized;

static void light_sensor_poll_fn(struct k_work *work);

static inline int submit_poll_work(const u32_t delay_s)
{
	return k_delayed_work_submit(&ls_poller, K_SECONDS((u32_t)delay_s));
}

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

	initialized = true;

	return (data_send_interval_s > 0) ?
		submit_poll_work(LS_INIT_DELAY_S) : 0;
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

	if (data_send_interval_s == 0) {
		return;
	}

	int err = sensor_sample_fetch_chan(ls_dev, SENSOR_CHAN_ALL);

	if (err) {
		LOG_ERR("Failed to fetch data from %s, error: %d",
			log_strdup(ls_dev_name), err);
	}

	key = k_spin_lock(&ls_lock);

	for (enum ls_ch_type ch = LS_CH__BEGIN; ch < LS_CH__END; ++ch) {
		err = sensor_channel_get(ls_dev, ls_data[ch]->type,
					 &ls_data[ch]->data);
		if (err) {
			LOG_ERR("Failed to get data from %s, sensor ch: %d , error: %d",
				log_strdup(ls_dev_name),
				ls_data[ch]->type, err);
		}
	}

	k_spin_unlock(&ls_lock, key);

	if (ls_cb != NULL) {
		ls_cb();
	}

	submit_poll_work(data_send_interval_s);
}

void light_sensor_set_send_interval(const u32_t interval_s)
{
	if (interval_s == data_send_interval_s) {
		return;
	}

	data_send_interval_s = MIN(interval_s, MAX_INTERVAL_S);

	if (!initialized) {
		return;
	}

	if (data_send_interval_s) {
		/* restart work for new interval to take effect */
		submit_poll_work(K_NO_WAIT);
	} else if (k_delayed_work_remaining_get(&ls_poller) > 0) {
		k_delayed_work_cancel(&ls_poller);
	}
}

u32_t light_sensor_get_send_interval(void)
{
	return data_send_interval_s;
}

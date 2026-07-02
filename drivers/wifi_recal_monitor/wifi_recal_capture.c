/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <drivers/wifi_recal_monitor/wifi_recal_monitor.h>

LOG_MODULE_REGISTER(wifi_recal_capture, CONFIG_WIFI_RECAL_LOG_LEVEL);

/* Default value for a channel with no registered provider, indexed by
 * enum wifi_recal_channel_id. Providers self-register from their own source
 * file via WIFI_RECAL_CHANNEL_DEFINE(); this file must not know about any
 * specific provider.
 */
static const union wifi_recal_sample_value wifi_recal_channel_defaults[WIFI_RECAL_CH_COUNT] = {
	[WIFI_RECAL_CH_BATTERY_VOLTAGE] = {.i32 = CONFIG_WIFI_RECAL_BATTERY_VOLTAGE_DEFAULT_VALUE},
	[WIFI_RECAL_CH_DIE_TEMP] = {.i32 = CONFIG_WIFI_RECAL_DIE_TEMP_DEFAULT_VALUE},
};

static union wifi_recal_sample_value wifi_recal_snapshots[WIFI_RECAL_CH_COUNT];
static bool ch_live_update[WIFI_RECAL_CH_COUNT];

int wifi_recal_monitor_sample_get(enum wifi_recal_channel_id id, union wifi_recal_sample_value *out)
{
	if (out == NULL || id >= WIFI_RECAL_CH_COUNT) {
		return -EINVAL;
	}

	*out = wifi_recal_snapshots[id];
	return 0;
}

static void wifi_recal_capture_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(wifi_recal_capture_work, wifi_recal_capture_work_handler);

static void wifi_recal_get_samples(void)
{
	STRUCT_SECTION_FOREACH(wifi_recal_channel, ch)
	{
		if (ch_live_update[ch->id]) {
			struct wifi_recal_sample sample;
			int err = ch->sample(&sample);

			if (err < 0 || sample.status != WIFI_RECAL_STATUS_OK) {
				LOG_ERR("ch=%d sample() failed: %d", ch->id, err);
			} else {
				wifi_recal_snapshots[ch->id] = sample.value;
			}
		}
		/* For non-live channels, default value is used. */
		LOG_DBG("Channel %d snapshot: %d", ch->id, wifi_recal_snapshots[ch->id].u32);
	}
}

static void wifi_recal_capture_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	wifi_recal_get_samples();
	k_work_schedule(&wifi_recal_capture_work, K_MSEC(CONFIG_WIFI_RECAL_SNAPSHOT_INTERVAL_MS));
}

static int wifi_recal_monitor_init(void)
{
	int err = 0;
	bool any_live_update = false;

	for (enum wifi_recal_channel_id id = 0; id < WIFI_RECAL_CH_COUNT; id++) {
		wifi_recal_snapshots[id] = wifi_recal_channel_defaults[id];
	}

	STRUCT_SECTION_FOREACH(wifi_recal_channel, ch)
	{
		LOG_DBG("Initializing channel %d", ch->id);
		wifi_recal_snapshots[ch->id] = ch->default_value;
		ch_live_update[ch->id] = true;
		if (ch->init == NULL || ch->sample == NULL) {
			LOG_WRN("ch=%d has no init() or no sample(), use default value instead",
				ch->id);
			ch_live_update[ch->id] = false;
			continue;
		}
		err = ch->init();
		if (err < 0) {
			LOG_ERR("ch=%d init() failed: %d, use default value instead", ch->id, err);
			ch_live_update[ch->id] = false;
			continue;
		}
		any_live_update = true;
	}

	if (any_live_update) {
		k_work_schedule(&wifi_recal_capture_work, K_NO_WAIT);
	} else {
		LOG_DBG("No live update channels enabled, snapshots will remain as defaults");
	}
	return 0;
}

SYS_INIT(wifi_recal_monitor_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

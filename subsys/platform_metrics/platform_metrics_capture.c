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
#include <platform_metrics.h>

LOG_MODULE_REGISTER(platform_metrics_capture, CONFIG_PLATFORM_METRICS_LOG_LEVEL);

/* Default value for a channel with no registered provider, indexed by
 * enum platform_metrics_channel_id. Providers self-register from their
 * own source file via PLATFORM_METRICS_CHANNEL_DEFINE(); this file must
 * not know about any specific provider.
 */
static const union platform_metrics_sample_value
	platform_metrics_channel_defaults[PLATFORM_METRICS_CH_COUNT] = {
		[PLATFORM_METRICS_CH_BATTERY_VOLTAGE] = {
			.i32 = CONFIG_PLATFORM_METRICS_BATTERY_VOLTAGE_DEFAULT_VALUE,
		},
		[PLATFORM_METRICS_CH_DIE_TEMP] = {
			.i32 = CONFIG_PLATFORM_METRICS_DIE_TEMP_DEFAULT_VALUE,
		},
};

static union platform_metrics_sample_value platform_metrics_snapshots[PLATFORM_METRICS_CH_COUNT];
static bool ch_live_update[PLATFORM_METRICS_CH_COUNT];

int platform_metrics_sample_get(enum platform_metrics_channel_id id,
				 union platform_metrics_sample_value *out)
{
	if (out == NULL || id >= PLATFORM_METRICS_CH_COUNT) {
		return -EINVAL;
	}

	*out = platform_metrics_snapshots[id];
	return 0;
}

static void platform_metrics_capture_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(platform_metrics_capture_work,
				platform_metrics_capture_work_handler);

static void platform_metrics_get_samples(void)
{
	STRUCT_SECTION_FOREACH(platform_metrics_channel, ch)
	{
		if (ch_live_update[ch->id]) {
			struct platform_metrics_sample sample;
			int err = ch->sample(&sample);

			if (err < 0 || sample.status != PLATFORM_METRICS_STATUS_OK) {
				LOG_ERR("ch=%d sample() failed: %d", ch->id, err);
			} else {
				platform_metrics_snapshots[ch->id] = sample.value;
			}
		}
		/* For non-live channels, default value is used. */
		LOG_DBG("Channel %d snapshot: %d", ch->id, platform_metrics_snapshots[ch->id].u32);
	}
}

static void platform_metrics_capture_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	platform_metrics_get_samples();
	k_work_schedule(&platform_metrics_capture_work,
			K_MSEC(CONFIG_PLATFORM_METRICS_SNAPSHOT_INTERVAL_MS));
}

static int platform_metrics_init(void)
{
	int err = 0;
	bool any_live_update = false;

	for (enum platform_metrics_channel_id id = 0; id < PLATFORM_METRICS_CH_COUNT; id++) {
		platform_metrics_snapshots[id] = platform_metrics_channel_defaults[id];
	}

	STRUCT_SECTION_FOREACH(platform_metrics_channel, ch)
	{
		LOG_DBG("Initializing channel %d", ch->id);
		platform_metrics_snapshots[ch->id] = ch->default_value;
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
		k_work_schedule(&platform_metrics_capture_work, K_NO_WAIT);
	} else {
		LOG_DBG("No live update channels enabled, snapshots will remain as defaults");
	}
	return 0;
}

SYS_INIT(platform_metrics_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

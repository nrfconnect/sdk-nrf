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
#include <drivers/vtf_monitoring/vtf_monitoring.h>

LOG_MODULE_REGISTER(vtf_capture, CONFIG_VTF_LOG_LEVEL);

/* Default value for a channel with no registered provider, indexed by
 * enum vtf_channel_id. Providers self-register from their own source
 * file via VTF_CHANNEL_DEFINE(); this file must not know about any
 * specific provider.
 */
static const union vtf_sample_value vtf_channel_defaults[VTF_CH_COUNT] = {
	[VTF_CH_BATTERY_VOLTAGE] = {.i32 = CONFIG_VTF_BATTERY_VOLTAGE_DEFAULT_VALUE},
	[VTF_CH_DIE_TEMP] = {.i32 = CONFIG_VTF_DIE_TEMP_DEFAULT_VALUE},
	[VTF_CH_FREQ_OFFSET] = {.i32 = CONFIG_VTF_FREQ_OFFSET_DEFAULT_VALUE},
};

static union vtf_sample_value vtf_snapshots[VTF_CH_COUNT];
static bool ch_live_update[VTF_CH_COUNT];

int vtf_monitoring_sample_get(enum vtf_channel_id id, union vtf_sample_value *out)
{
	if (out == NULL || id >= VTF_CH_COUNT) {
		return -EINVAL;
	}

	*out = vtf_snapshots[id];
	return 0;
}

static void vtf_capture_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(vtf_capture_work, vtf_capture_work_handler);

static void vtf_get_samples(void)
{
	STRUCT_SECTION_FOREACH(vtf_channel, ch)
	{
		if (ch_live_update[ch->id]) {
			struct vtf_sample sample;
			int err = ch->sample(&sample);

			if (err < 0 || sample.status != VTF_STATUS_OK) {
				LOG_ERR("ch=%d sample() failed: %d", ch->id, err);
			} else {
				vtf_snapshots[ch->id] = sample.value;
			}
		}
		/* For non-live channels, default value is used. */
		LOG_DBG("Channel %d snapshot: %d", ch->id, vtf_snapshots[ch->id].u32);
	}
}

static void vtf_capture_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	vtf_get_samples();
	k_work_schedule(&vtf_capture_work, K_MSEC(CONFIG_VTF_SNAPSHOT_INTERVAL_MS));
}

static int vtf_monitoring_init(void)
{
	int err = 0;
	bool any_live_update = false;

	for (enum vtf_channel_id id = 0; id < VTF_CH_COUNT; id++) {
		vtf_snapshots[id] = vtf_channel_defaults[id];
	}

	STRUCT_SECTION_FOREACH(vtf_channel, ch)
	{
		LOG_DBG("Initializing channel %d", ch->id);
		vtf_snapshots[ch->id] = ch->default_value;
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
		k_work_schedule(&vtf_capture_work, K_NO_WAIT);
	} else {
		LOG_DBG("No live update channels enabled, snapshots will remain as defaults");
	}
	return 0;
}

SYS_INIT(vtf_monitoring_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

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
#include "vtf_monitoring.h"

LOG_MODULE_REGISTER(vtf_capture, CONFIG_VTF_LOG_LEVEL);

VTF_CHANNEL_DEFINE(vtf_channel_die_temp, VTF_CH_DIE_TEMP, NULL, NULL, VTF_SAMPLE_TYPE_INT,
		   i32, CONFIG_VTF_DIE_TEMP_DEFAULT_VALUE);

VTF_CHANNEL_DEFINE(vtf_channel_battery_voltage, VTF_CH_BATTERY_VOLTAGE, NULL, NULL,
		   VTF_SAMPLE_TYPE_INT, i32, CONFIG_VTF_BATTERY_VOLTAGE_DEFAULT_VALUE);

VTF_CHANNEL_DEFINE(vtf_channel_freq_offset, VTF_CH_FREQ_OFFSET, NULL, NULL, VTF_SAMPLE_TYPE_INT,
		   i32, CONFIG_VTF_FREQ_OFFSET_DEFAULT_VALUE);

union vtf_sample_value vtf_snapshots[VTF_CH_COUNT];
static bool ch_live_update[VTF_CH_COUNT];

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

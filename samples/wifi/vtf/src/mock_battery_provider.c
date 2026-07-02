/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * Tier-2 provider example: no sensor API, no devicetree binding, just
 * a plain init()/sample() pair registered with VTF_CHANNEL_DEFINE().
 * Stands in for whatever a customer's own fuel-gauge library or raw
 * register read would do; simulates a battery discharging from 4200
 * to 3300 mV and back, to make the changing value visible in the log.
 */

#include <zephyr/kernel.h>

#include <drivers/vtf_monitoring/vtf_monitoring.h>

#define MOCK_BATTERY_MAX_MV 4200
#define MOCK_BATTERY_MIN_MV 3300
#define MOCK_BATTERY_STEP_MV 20

static int32_t mock_battery_mv = MOCK_BATTERY_MAX_MV;
static int32_t mock_battery_step_mv = -MOCK_BATTERY_STEP_MV;

static int mock_battery_init(void)
{
	return 0;
}

static int mock_battery_sample(struct vtf_sample *out)
{
	mock_battery_mv += mock_battery_step_mv;
	if (mock_battery_mv <= MOCK_BATTERY_MIN_MV || mock_battery_mv >= MOCK_BATTERY_MAX_MV) {
		mock_battery_step_mv = -mock_battery_step_mv;
	}

	out->type = VTF_SAMPLE_TYPE_INT;
	out->value.i32 = mock_battery_mv;
	out->timestamp_ms = k_uptime_get();
	out->status = VTF_STATUS_OK;
	return 0;
}

VTF_CHANNEL_DEFINE(vtf_channel_mock_battery, VTF_CH_BATTERY_VOLTAGE, mock_battery_sample,
		   mock_battery_init, VTF_SAMPLE_TYPE_INT, i32, MOCK_BATTERY_MAX_MV);

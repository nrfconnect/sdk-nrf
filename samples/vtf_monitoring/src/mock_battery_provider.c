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
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <drivers/vtf_monitoring/vtf_monitoring.h>

#define MOCK_BATTERY_MAX_MV  4200
#define MOCK_BATTERY_MIN_MV  3300
#define MOCK_BATTERY_STEP_MV 20

static int32_t mock_battery_mv = MOCK_BATTERY_MAX_MV;

#define MOCK_VTF_BATTERY_VOLTAGE_MONITOR_INTERVAL_MS 10000

LOG_MODULE_REGISTER(battery_voltage_monitor, LOG_LEVEL_INF);

static K_SEM_DEFINE(sensor_state_lock, 1, 1);

static struct vtf_sample sensor_state = {
	.type = VTF_SAMPLE_TYPE_INT,
	.value.i32 = 0,
	.timestamp_ms = 0,
	.status = VTF_STATUS_UNINITIALISED,
};

static void battery_voltage_work_handler(struct k_work *work);
static void reschedule_battery_voltage_work(void);

static K_WORK_DELAYABLE_DEFINE(battery_voltage_work, battery_voltage_work_handler);

static void reschedule_battery_voltage_work(void)
{
	k_work_schedule(&battery_voltage_work,
			K_MSEC(MOCK_VTF_BATTERY_VOLTAGE_MONITOR_INTERVAL_MS));
}

static void battery_voltage_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	mock_battery_mv -= MOCK_BATTERY_STEP_MV;
	if (mock_battery_mv <= MOCK_BATTERY_MIN_MV) {
		mock_battery_mv = MOCK_BATTERY_MAX_MV;
	}

	k_sem_take(&sensor_state_lock, K_FOREVER);
	sensor_state.value.i32 = mock_battery_mv;
	sensor_state.timestamp_ms = k_uptime_get();
	sensor_state.status = VTF_STATUS_OK;
	k_sem_give(&sensor_state_lock);

	reschedule_battery_voltage_work();
}

static int mock_battery_init(void)
{
	k_work_schedule(&battery_voltage_work, K_NO_WAIT);
	return 0;
}

static int mock_battery_sample(struct vtf_sample *out)
{
	if (out == NULL) {
		return -EINVAL;
	}

	k_sem_take(&sensor_state_lock, K_FOREVER);
	*out = sensor_state;
	k_sem_give(&sensor_state_lock);
	return 0;
}

VTF_CHANNEL_DEFINE(vtf_channel_battery_voltage, VTF_CH_BATTERY_VOLTAGE, mock_battery_sample,
		   mock_battery_init, VTF_SAMPLE_TYPE_INT, i32, MOCK_BATTERY_MAX_MV);

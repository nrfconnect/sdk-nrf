/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <vtf_monitoring/vtf_monitoring.h>

LOG_MODULE_REGISTER(battery_voltage_monitor, CONFIG_VTF_LOG_LEVEL);

#define VBAT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_vbat)

static const struct device *vbat_dev = DEVICE_DT_GET(VBAT_NODE);

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
			K_MSEC(CONFIG_VTF_BATTERY_VOLTAGE_MONITOR_INTERVAL_MS));
}

static void battery_voltage_work_handler(struct k_work *work)
{
	struct sensor_value val;
	int err;

	ARG_UNUSED(work);

	err = sensor_sample_fetch(vbat_dev);

	if (!err) {
		err = sensor_channel_get(vbat_dev, SENSOR_CHAN_VOLTAGE, &val);
	}

	if (err < 0) {
		LOG_ERR("Battery voltage read failed: %d", err);
	} else {
		LOG_DBG("BATTERY_VOLTAGE is %d.%02d", val.val1, abs(val.val2 / 10000));
	}

	k_sem_take(&sensor_state_lock, K_FOREVER);
	if (err < 0) {
		LOG_ERR("Failed to read BATTERY_VOLTAGE: %d", err);
		sensor_state.status = VTF_STATUS_ERROR;
	} else {
		sensor_state.value.i32 = (int32_t)sensor_value_to_milli(&val);
		sensor_state.timestamp_ms = k_uptime_get();
		sensor_state.status = VTF_STATUS_OK;
	}

	k_sem_give(&sensor_state_lock);

	reschedule_battery_voltage_work();
}

static int battery_voltage_init(void)
{
	if (!device_is_ready(vbat_dev)) {
		LOG_ERR("%s is not ready", vbat_dev->name);
		return -ENODEV;
	}

	k_work_schedule(&battery_voltage_work, K_NO_WAIT);
	return 0;
}

static int battery_voltage_sample(struct vtf_sample *out)
{
	if (out == NULL) {
		return -EINVAL;
	}

	k_sem_take(&sensor_state_lock, K_FOREVER);
	*out = sensor_state;
	k_sem_give(&sensor_state_lock);
	return 0;
}

VTF_CHANNEL_DEFINE(vtf_channel_battery_voltage, VTF_CH_BATTERY_VOLTAGE, battery_voltage_sample,
		   battery_voltage_init, VTF_SAMPLE_TYPE_INT, i32,
		   CONFIG_VTF_BATTERY_VOLTAGE_DEFAULT_VALUE);

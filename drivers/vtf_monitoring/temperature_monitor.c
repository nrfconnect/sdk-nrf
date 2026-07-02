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

#include <drivers/vtf_monitoring/vtf_monitoring.h>

LOG_MODULE_REGISTER(die_temp_monitor, CONFIG_VTF_LOG_LEVEL);

/* Board devicetree can override which sensor feeds VTF_CH_DIE_TEMP via
 * the `nordic,vtf-die-temp-sensor` chosen node (e.g. a customer's own
 * ambient temperature sensor). Falls back to the SoC's own die
 * temperature sensor when no override is present, so the DK keeps
 * working with zero devicetree changes.
 */
#if DT_HAS_CHOSEN(nordic_vtf_die_temp_sensor)
#define VTF_DIE_TEMP_SENSOR_NODE DT_CHOSEN(nordic_vtf_die_temp_sensor)
#else
#define VTF_DIE_TEMP_SENSOR_NODE DT_NODELABEL(temp)
#endif

static const struct device *const temp_dev = DEVICE_DT_GET(VTF_DIE_TEMP_SENSOR_NODE);

static K_SEM_DEFINE(sensor_state_lock, 1, 1);

static struct vtf_sample sensor_state = {
	.type = VTF_SAMPLE_TYPE_INT,
	.value.i32 = 0,
	.timestamp_ms = 0,
	.status = VTF_STATUS_UNINITIALISED,
};

static void die_temp_work_handler(struct k_work *work);
static void reschedule_die_temp_work(void);

static K_WORK_DELAYABLE_DEFINE(die_temp_work, die_temp_work_handler);

static void reschedule_die_temp_work(void)
{
	k_work_schedule(&die_temp_work, K_MSEC(CONFIG_VTF_DIE_TEMP_MONITOR_INTERVAL_MS));
}

static void die_temp_work_handler(struct k_work *work)
{
	struct sensor_value val;
	int err;

	ARG_UNUSED(work);

	err = sensor_sample_fetch(temp_dev);

	if (!err) {
		err = sensor_channel_get(temp_dev, SENSOR_CHAN_DIE_TEMP, &val);
	}

	if (err < 0) {
		LOG_ERR("Die temperature read failed: %d", err);
	} else {
		LOG_DBG("DIE_TEMP is %d.%02d", val.val1, abs(val.val2 / 10000));
	}

	k_sem_take(&sensor_state_lock, K_FOREVER);
	if (err < 0) {
		LOG_ERR("Failed to read DIE_TEMP: %d", err);
		sensor_state.status = VTF_STATUS_ERROR;
	} else {
		sensor_state.value.i32 = (int32_t)sensor_value_to_centi(&val);
		sensor_state.timestamp_ms = k_uptime_get();
		sensor_state.status = VTF_STATUS_OK;
	}

	k_sem_give(&sensor_state_lock);

	reschedule_die_temp_work();
}

static int die_temp_init(void)
{
	if (!device_is_ready(temp_dev)) {
		LOG_ERR("%s is not ready", temp_dev->name);
		return -ENODEV;
	}

	k_work_schedule(&die_temp_work, K_NO_WAIT);
	return 0;
}

static int die_temp_sample(struct vtf_sample *out)
{
	if (out == NULL) {
		return -EINVAL;
	}

	k_sem_take(&sensor_state_lock, K_FOREVER);
	*out = sensor_state;
	k_sem_give(&sensor_state_lock);
	return 0;
}

VTF_CHANNEL_DEFINE(vtf_channel_die_temp, VTF_CH_DIE_TEMP, die_temp_sample, die_temp_init,
		   VTF_SAMPLE_TYPE_INT, i32, CONFIG_VTF_DIE_TEMP_DEFAULT_VALUE);

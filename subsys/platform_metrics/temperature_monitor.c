/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/logging/log.h>

#include <platform_metrics.h>

LOG_MODULE_REGISTER(die_temp_monitor, CONFIG_PLATFORM_METRICS_LOG_LEVEL);

/* Board devicetree can override which sensor feeds PLATFORM_METRICS_CH_DIE_TEMP via
 * the `nordic,platform-metrics-die-temp-sensor` chosen node (e.g. a customer's own
 * ambient temperature sensor). Falls back to the SoC's own die
 * temperature sensor when no override is present, so the DK keeps
 * working with zero devicetree changes.
 */
#if DT_HAS_CHOSEN(nordic_platform_metrics_die_temp_sensor)
#define PLATFORM_METRICS_DIE_TEMP_SENSOR_NODE DT_CHOSEN(nordic_platform_metrics_die_temp_sensor)
#else
#define PLATFORM_METRICS_DIE_TEMP_SENSOR_NODE DT_NODELABEL(temp)
#endif

static const struct device *const temp_dev = DEVICE_DT_GET(PLATFORM_METRICS_DIE_TEMP_SENSOR_NODE);

/*
 * Read via the RTIO-based sensor API rather than sensor_sample_fetch()/
 * sensor_channel_get(): when the bound device has no native RTIO submit
 * (like temp_nrf5), Zephyr's sensor subsystem falls back to calling
 * fetch/get internally, so this keeps working on the DK. It also means
 * a board that overrides the chosen node to a sensor driver which *only*
 * implements the RTIO path (no fetch/get) still works, unlike a direct
 * fetch/get call would.
 */
SENSOR_DT_READ_IODEV(die_temp_iodev, PLATFORM_METRICS_DIE_TEMP_SENSOR_NODE,
		      {SENSOR_CHAN_DIE_TEMP, 0});
RTIO_DEFINE(die_temp_rtio_ctx, 1, 1);

static K_SEM_DEFINE(sensor_state_lock, 1, 1);

static struct platform_metrics_sample sensor_state = {
	.type = PLATFORM_METRICS_SAMPLE_TYPE_INT,
	.value.i32 = 0,
	.timestamp_ms = 0,
	.status = PLATFORM_METRICS_STATUS_UNINITIALISED,
};

static void die_temp_work_handler(struct k_work *work);
static void reschedule_die_temp_work(void);

static K_WORK_DELAYABLE_DEFINE(die_temp_work, die_temp_work_handler);

static void reschedule_die_temp_work(void)
{
	k_work_schedule(&die_temp_work,
			K_MSEC(CONFIG_PLATFORM_METRICS_DIE_TEMP_MONITOR_INTERVAL_MS));
}

/*
 * A decoded q31 sample represents value = q / 2^(31 - shift) in the
 * channel's SI unit (here, degC). Scale by 100 before the shift so the
 * result lands directly in centi-degC, the unit the rest of this driver
 * stores die temperature in.
 */
static int32_t q31_to_centi(q31_t q, int8_t shift)
{
	return (int32_t)(((int64_t)q * 100) >> (31 - shift));
}

static void die_temp_work_handler(struct k_work *work)
{
	uint8_t buf[128];
	int32_t temp_centi = 0;
	int err;

	ARG_UNUSED(work);

	err = sensor_read(&die_temp_iodev, &die_temp_rtio_ctx, buf, sizeof(buf));

	if (!err) {
		const struct sensor_decoder_api *decoder;

		err = sensor_get_decoder(temp_dev, &decoder);
		if (!err) {
			struct sensor_chan_spec chan = {SENSOR_CHAN_DIE_TEMP, 0};
			struct sensor_q31_data data = {0};
			uint32_t fit = 0;
			int decoded;

			decoded = decoder->decode(buf, chan, &fit, 1, &data);
			err = (decoded == 1) ? 0 : -EIO;
			if (!err) {
				temp_centi = q31_to_centi(data.readings[0].value, data.shift);
			}
		}
	}

	if (err < 0) {
		LOG_ERR("Die temperature read failed: %d", err);
	} else {
		LOG_DBG("DIE_TEMP is %d.%02d", temp_centi / 100, abs(temp_centi % 100));
	}

	k_sem_take(&sensor_state_lock, K_FOREVER);
	if (err < 0) {
		sensor_state.status = PLATFORM_METRICS_STATUS_ERROR;
	} else {
		sensor_state.value.i32 = temp_centi;
		sensor_state.timestamp_ms = k_uptime_get();
		sensor_state.status = PLATFORM_METRICS_STATUS_OK;
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

static int die_temp_sample(struct platform_metrics_sample *out)
{
	if (out == NULL) {
		return -EINVAL;
	}

	k_sem_take(&sensor_state_lock, K_FOREVER);
	*out = sensor_state;
	k_sem_give(&sensor_state_lock);
	return 0;
}

PLATFORM_METRICS_CHANNEL_DEFINE(platform_metrics_channel_die_temp, PLATFORM_METRICS_CH_DIE_TEMP,
			   die_temp_sample, die_temp_init, PLATFORM_METRICS_SAMPLE_TYPE_INT, i32,
			   CONFIG_PLATFORM_METRICS_DIE_TEMP_DEFAULT_VALUE);

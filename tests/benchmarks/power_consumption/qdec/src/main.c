/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device_runtime.h>


#define LOG_LEVEL LOG_LEVEL_DBG

#define QDEC DT_ALIAS(qdec0)

extern struct k_sem semaphore;

static const struct gpio_dt_spec phase_a = GPIO_DT_SPEC_GET(DT_ALIAS(qenca), gpios);
static const struct gpio_dt_spec phase_b = GPIO_DT_SPEC_GET(DT_ALIAS(qencb), gpios);
static const struct device *const qdec_dev = DEVICE_DT_GET(DT_ALIAS(qdec0));
static const uint32_t qdec_config_step = DT_PROP(DT_ALIAS(qdec0), steps);
static bool toggle_a = true;

static void qenc_emulate_work_handler(struct k_work *work)
{
	if (toggle_a) {
		gpio_pin_toggle_dt(&phase_a);
	} else {
		gpio_pin_toggle_dt(&phase_b);
	}
	toggle_a = !toggle_a;
}

static K_WORK_DEFINE(qenc_emulate_work, qenc_emulate_work_handler);

static void qenc_emulate_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&qenc_emulate_work);
}

static K_TIMER_DEFINE(qenc_emulate_timer, qenc_emulate_timer_handler, NULL);

static void qenc_emulate_start(k_timeout_t period, bool forward)
{
	gpio_pin_set_dt(&phase_a, 0);
	gpio_pin_set_dt(&phase_b, 0);

	toggle_a = !forward;

	k_timer_start(&qenc_emulate_timer, period, period);
}

static void qenc_emulate_stop(void)
{
	k_timer_stop(&qenc_emulate_timer);

	gpio_pin_set_dt(&phase_a, 0);
	gpio_pin_set_dt(&phase_b, 0);
}

static void qenc_emulate_verify_reading(int emulator_period_ms, int emulation_duration_ms,
					bool forward)
{
	int rc;
	struct sensor_value val = {0};
	int32_t expected_steps = emulation_duration_ms / emulator_period_ms;
	int32_t expected_reading = 360 * expected_steps / qdec_config_step;
	int32_t delta = expected_reading / 5;

	if (!forward) {
		expected_reading *= -1;
	}

	qenc_emulate_start(K_MSEC(emulator_period_ms), forward);
	k_msleep(emulation_duration_ms);
	qenc_emulate_stop();

	rc = sensor_sample_fetch(qdec_dev);
	__ASSERT_NO_MSG(rc == 0);

	rc = sensor_channel_get(qdec_dev, SENSOR_CHAN_ROTATION, &val);
	__ASSERT_NO_MSG(rc == 0);

	rc = ((val.val1 >= (expected_reading + delta)) || (val.val1 <= (expected_reading - delta)))
		     ? 1
		     : 0;
	__ASSERT_NO_MSG(rc == 0);

	/* wait and get readings to clear state */
	k_msleep(20);
	rc = sensor_sample_fetch(qdec_dev);
	__ASSERT_NO_MSG(rc == 0);

	rc = sensor_channel_get(qdec_dev, SENSOR_CHAN_ROTATION, &val);
	__ASSERT_NO_MSG(rc == 0);
}

void init_device(void)
{
	int rc;

	rc = device_is_ready(qdec_dev);
	__ASSERT_NO_MSG(rc);

	rc = gpio_is_ready_dt(&phase_a);
	__ASSERT_NO_MSG(rc);

	rc = gpio_pin_configure_dt(&phase_a, GPIO_OUTPUT);
	__ASSERT_NO_MSG(rc == 0);

	rc = gpio_is_ready_dt(&phase_b);
	__ASSERT_NO_MSG(rc);

	rc = gpio_pin_configure_dt(&phase_b, GPIO_OUTPUT);
	__ASSERT_NO_MSG(rc == 0);
}

int main(void)
{
	int rc;

	init_device();

	while (1) {
		if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
			rc = pm_device_runtime_get(qdec_dev);
			__ASSERT_NO_MSG(rc == 0);
		}
		qenc_emulate_verify_reading(20, 450, true);
		qenc_emulate_verify_reading(20, 450, false);
		if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
			rc = pm_device_runtime_put(qdec_dev);
			__ASSERT_NO_MSG(rc == 0);
		}
		k_msleep(1000);
	}
	return 0;
}

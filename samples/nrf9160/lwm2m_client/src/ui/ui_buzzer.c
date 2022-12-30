/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/devicetree.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_lwm2m_client, CONFIG_APP_LOG_LEVEL);

/* Transform frequency in Hz to period in microseconds */
#define PERIOD(freq) ((USEC_PER_SEC) / (freq))

#define FREQUENCY_MAX 10000
#define INTENSITY_MAX 100

/* Affects curvature of pulse width graph */
#define CURVE_CONST 50

static const struct pwm_dt_spec buzzer = PWM_DT_SPEC_GET(DT_NODELABEL(buzzer));

static uint32_t frequency;
static uint8_t intensity;
static bool state;

/**
 * @brief Calculate pulse width.
 *
 * Perceived sound level is logarithmic with respect to the pulse width.
 * This transformation is exponential to try to get a linear relationship
 * between intensity and perceived sound level.
 *
 * Designed to achieve pulse width = 0 with intensity = 0,
 * and pulse width = period/2 with intensity = 100.
 *
 * @param period Period in microseconds.
 * @param intensity Integer between [0, 100], describing
 * a percentage of the maximum buzzer volume intensity.
 * @return uint32_t Pulse width in microseconds.
 */
static uint32_t calculate_pulse_width(int32_t period, int32_t intensity)
{
	int32_t divisor = CURVE_CONST + ((2 - CURVE_CONST) * intensity) / INTENSITY_MAX;
	int32_t offset = (period * (INTENSITY_MAX - intensity)) / (CURVE_CONST * INTENSITY_MAX);

	return (uint32_t)(period / divisor - offset);
}

int ui_buzzer_on_off(bool new_state)
{
	int ret;

	state = new_state;

	if (frequency == 0) {
		/* Turn off buzzer when frequency = 0 */
		ret = pwm_set_pulse_dt(&buzzer, 0);
	} else {
		uint32_t period = PERIOD(frequency);
		uint32_t pulse_width = calculate_pulse_width(period, intensity);

		ret = pwm_set_dt(&buzzer, PWM_USEC(period), PWM_USEC(pulse_width * state));
	}

	if (ret) {
		LOG_ERR("Set pwm pin failed (%d)", ret);
		return ret;
	}

	return 0;
}

int ui_buzzer_set_frequency(uint32_t freq)
{
	int ret;

	if (freq > FREQUENCY_MAX) {
		LOG_ERR("Frequency too high (%d)", -EINVAL);
		return -EINVAL;
	}

	frequency = freq;

	if (state) {
		ret = ui_buzzer_on_off(state);
		if (ret) {
			LOG_ERR("Set buzzer on/off failed (%d)", ret);
			return ret;
		}
	}

	return 0;
}

int ui_buzzer_set_intensity(uint8_t new_intensity)
{
	int ret;

	if (new_intensity > INTENSITY_MAX) {
		LOG_ERR("Dutycycle too large (%d)", -EINVAL);
		return -EINVAL;
	}

	intensity = new_intensity;

	if (state) {
		ret = ui_buzzer_on_off(state);
		if (ret) {
			LOG_ERR("Set buzzer on/off failed (%d)", ret);
			return ret;
		}
	}

	return 0;
}

int ui_buzzer_init(void)
{
	if (!device_is_ready(buzzer.dev)) {
		LOG_ERR("Buzzer PWM device not ready");
		return -ENODEV;
	}

	return 0;
}

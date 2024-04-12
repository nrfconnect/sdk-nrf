/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_ui_speaker.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fmdn, LOG_LEVEL_DBG);

#define SPK_NODE	DT_ALIAS(pwm_spk)

static const struct pwm_dt_spec spk_pwm = PWM_DT_SPEC_GET(SPK_NODE);
static uint32_t spk_per_ns;

int app_ui_speaker_init(void)
{
	int err;

	if (!device_is_ready(spk_pwm.dev)) {
		LOG_ERR("PWM_SPK is not ready");
		return -EIO;
	}

	spk_per_ns = PWM_HZ(CONFIG_APP_UI_SPEAKER_FREQ);
	err = pwm_set_dt(&spk_pwm, spk_per_ns, 0);
	if (err) {
		LOG_ERR("Can't initiate PWM (err %d)", err);
		return err;
	}

	return 0;
}

int app_ui_speaker_on(void)
{
	int err;

	err = pwm_set_dt(&spk_pwm, spk_per_ns, (spk_per_ns / 2UL));
	if (err) {
		LOG_ERR("Can't set speaker frequency (err %d)", err);
		return err;
	}

	return err;
}

int app_ui_speaker_off(void)
{
	int err;

	err = pwm_set_dt(&spk_pwm, spk_per_ns, 0);
	if (err) {
		LOG_ERR("Can't set speaker frequency (err %d)", err);
		return err;
	}

	return err;
}

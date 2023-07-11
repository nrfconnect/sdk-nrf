/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>

#include "mpsl_fem_power_model_interface.h"

#include <mpsl_fem_nrf21540_power_model_builtin.h>
#include <mpsl_temp.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), supply_voltage_mv)
#define MPSL_FEM_NRF21540_SUPPLY_VOLTAGE_MV  DT_PROP(DT_NODELABEL(nrf_radio_fem), supply_voltage_mv)
#else
#error Cannot use nRF21540 built-in power model with unspecified supply-voltage-mv property
#endif /* !DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), supply_voltage_mv) */

static int32_t last_temperature = INT32_MIN;

static void model_update_work_handler(struct k_work *work)
{
	mpsl_fem_external_conditions_t external_conditions;
	int32_t current_temperature = mpsl_temperature_get();

	if (last_temperature != current_temperature) {
		external_conditions.temperature = mpsl_temperature_get() / 4;
		external_conditions.voltage = MPSL_FEM_NRF21540_SUPPLY_VOLTAGE_MV / 100;
		mpsl_fem_nrf21540_power_model_builtin_update(&external_conditions);
	}
}

K_WORK_DEFINE(model_update_work, model_update_work_handler);

static void model_update_timeout(struct k_timer *dummy)
{
	k_work_submit(&model_update_work);
}

K_TIMER_DEFINE(model_update_timer, model_update_timeout, NULL);

static int model_periodic_update_start(void)
{
	/* Perform initial model update */
	k_work_submit(&model_update_work);

	k_timer_start(&model_update_timer,
				K_MSEC(CONFIG_MPSL_FEM_BUILTIN_POWER_MODEL_UPDATE_PERIOD),
				K_MSEC(CONFIG_MPSL_FEM_BUILTIN_POWER_MODEL_UPDATE_PERIOD));

	return 0;
}

const mpsl_fem_power_model_t *mpsl_fem_power_model_to_use_get(void)
{
	return mpsl_fem_nrf21540_power_model_builtin_get();
}

SYS_INIT(model_periodic_update_start, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

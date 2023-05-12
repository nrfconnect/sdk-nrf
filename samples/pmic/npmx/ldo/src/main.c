/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <npmx_buck.h>
#include <npmx_driver.h>
#include <npmx_ldsw.h>

#define LOG_MODULE_NAME pmic_ldo
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/**
 * @brief Function for reading and logging selected voltage from the specified LDO instance.
 * This function does not read the actual output voltage of the LDO.
 *
 * @param[in] p_ldsw Pointer to the instance of the LDO.
 */
static void get_and_log_voltage(npmx_ldsw_t *p_ldsw)
{
	npmx_ldsw_voltage_t ldsw_voltage;

	if (npmx_ldsw_ldo_voltage_get(p_ldsw, &ldsw_voltage) != NPMX_SUCCESS) {
		LOG_ERR("Unable to get voltage status");
	}

	LOG_INF("Voltage LDO%d: %d mV", (p_ldsw->hw_index + 1),
		npmx_ldsw_voltage_convert_to_mv(ldsw_voltage));
}

/**
 * @brief Function for testing LDO voltage range,
 *        from start_voltage to stop_voltage with 100 mV step every one second.
 *
 * @param[in] p_ldsw        Pointer to the instance of the LDO.
 * @param[in] start_voltage Start test voltage level.
 */
static void verify_ldsw_voltage(npmx_ldsw_t *p_ldsw, npmx_ldsw_voltage_t start_voltage,
				npmx_ldsw_voltage_t stop_voltage)
{
	for (uint8_t i = (uint32_t)start_voltage; i <= (uint32_t)stop_voltage; i++) {
		/* Set the expected output voltage. */
		npmx_ldsw_ldo_voltage_set(p_ldsw, (npmx_ldsw_voltage_t)i);

		/* Check in nPM device which voltage is selected. */
		get_and_log_voltage(p_ldsw);

		/* Wait some time until the output voltage is stabilized, and measure it. */
		k_msleep(1000);
	}
}

void main(void)
{
	const struct device *pmic_dev = DEVICE_DT_GET(DT_NODELABEL(npm_0));

	if (!device_is_ready(pmic_dev)) {
		LOG_INF("PMIC device is not ready");
		return;
	}

	LOG_INF("PMIC device ok");

	/* Get the pointer to npmx device. */
	npmx_instance_t *npmx_instance = npmx_driver_instance_get(pmic_dev);

	npmx_buck_t *buck = npmx_buck_get(npmx_instance, 0);

	/* Set the output voltage of BUCK1. */
	npmx_buck_normal_voltage_set(buck, NPMX_BUCK_VOLTAGE_1V8);

	/* Have to be called each time to change output voltage. */
	npmx_buck_vout_select_set(buck, NPMX_BUCK_VOUT_SELECT_SOFTWARE);

	/* Enable BUCK1. */
	npmx_buck_task_trigger(buck, NPMX_BUCK_TASK_ENABLE);

	npmx_ldsw_t *ldsw_1 = npmx_ldsw_get(npmx_instance, 0);
	npmx_ldsw_t *ldsw_2 = npmx_ldsw_get(npmx_instance, 1);

	/* Configure LDSWs to work as LDOs. */
	npmx_ldsw_mode_set(ldsw_1, NPMX_LDSW_MODE_LDO);
	npmx_ldsw_mode_set(ldsw_2, NPMX_LDSW_MODE_LDO);

	/* Enable active discharge to see changes on output pins. */
	npmx_ldsw_active_discharge_enable_set(ldsw_1, true);
	npmx_ldsw_active_discharge_enable_set(ldsw_2, true);

	/* Set the initial voltages for both LDOs. */
	npmx_ldsw_ldo_voltage_set(ldsw_1, NPMX_LDSW_VOLTAGE_1V0);
	npmx_ldsw_ldo_voltage_set(ldsw_2, NPMX_LDSW_VOLTAGE_1V0);

	if (npmx_ldsw_task_trigger(ldsw_1, NPMX_LDSW_TASK_ENABLE) != NPMX_SUCCESS) {
		LOG_ERR("Unable to enable LDO1");
		return;
	}

	if (npmx_ldsw_task_trigger(ldsw_2, NPMX_LDSW_TASK_ENABLE) != NPMX_SUCCESS) {
		LOG_ERR("Unable to enable LDO2");
		return;
	}

	/* Test voltage ranges for LDOs. */
	verify_ldsw_voltage(ldsw_1, NPMX_LDSW_VOLTAGE_1V0, NPMX_LDSW_VOLTAGE_1V7);
	verify_ldsw_voltage(ldsw_2, NPMX_LDSW_VOLTAGE_1V0, NPMX_LDSW_VOLTAGE_2V9);

	/* Go back to initial voltages. */
	npmx_ldsw_ldo_voltage_set(ldsw_1, NPMX_LDSW_VOLTAGE_1V0);
	npmx_ldsw_ldo_voltage_set(ldsw_2, NPMX_LDSW_VOLTAGE_1V0);

	LOG_INF("Test output voltage OK");

	while (1) {
		k_sleep(K_FOREVER);
	}
}

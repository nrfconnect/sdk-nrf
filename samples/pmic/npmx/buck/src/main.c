/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <npmx_driver.h>
#include <npmx_buck.h>
#include <npmx_gpio.h>

#define LOG_MODULE_NAME pmic_buck
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* Values used to refer to the specified buck. */
#define BUCK1_IDX 0
#define BUCK2_IDX 1

#if CONFIG_TESTCASE_SET_BUCK_VOLTAGE || CONFIG_TESTCASE_OUTPUT_VOLTAGE ||                          \
	CONFIG_TESTCASE_ENABLE_BUCKS_USING_PIN
/**
 * @brief Function for setting the buck output voltage for the specified buck instance.
 *
 * @param[in] p_buck  Pointer to the instance of buck.
 * @param[in] voltage Selected voltage.
 *
 * @retval NPMX_SUCCESS  Operation performed successfully.
 * @retval NPMX_ERROR_IO Error using IO bus line.
 */
static npmx_error_t set_buck_voltage(npmx_buck_t *p_buck, npmx_buck_voltage_t voltage)
{
	npmx_error_t status;

	/* Set the output voltage. */
	status = npmx_buck_normal_voltage_set(p_buck, voltage);
	if (status != NPMX_SUCCESS) {
		LOG_ERR("Unable to set normal voltage");
		return status;
	}

	/* Have to be called each time to change output voltage. */
	status = npmx_buck_vout_select_set(p_buck, NPMX_BUCK_VOUT_SELECT_SOFTWARE);
	if (status != NPMX_SUCCESS) {
		LOG_ERR("Unable to select vout reference");
		return status;
	}
	return NPMX_SUCCESS;
}
#endif

#if CONFIG_TESTCASE_SET_BUCK_VOLTAGE
/**
 * @brief Function for testing simple use-case of buck voltage.
 *
 * @param[in] p_buck Pointer to the instance of buck.
 */
static void test_set_buck_voltage(npmx_buck_t *p_buck)
{
	LOG_INF("Test setting BUCK voltage");

	/* Set output voltage for BUCK1. */
	if (set_buck_voltage(p_buck, NPMX_BUCK_VOLTAGE_2V4) != NPMX_SUCCESS) {
		LOG_ERR("Unable to set buck voltage");
		return;
	}

	LOG_INF("Test setting BUCK voltage OK");
}
#endif

#if CONFIG_TESTCASE_OUTPUT_VOLTAGE
/**
 * @brief Function for reading and logging selected voltage from the specified buck instance.
 *
 * This function does not read the actual output voltage of the buck converter. For this sample, it
 * is used only to read and log voltage for BUCK1.
 *
 * @param[in] p_buck Pointer to the instance of buck.
 */
static void get_and_log_voltage(npmx_buck_t *p_buck)
{
	npmx_buck_voltage_t buck_voltage;

	if (npmx_buck_status_voltage_get(p_buck, &buck_voltage) != NPMX_SUCCESS) {
		LOG_ERR("Unable to get voltage status");
	}

	LOG_INF("Voltage BUCK1: %u mV", npmx_buck_voltage_convert_to_mv(buck_voltage));
}

/**
 * @brief Function for testing buck voltage range,
 *        from start_voltage to NPMX_BUCK_VOLTAGE_MAX with step every 200 ms.
 *
 * @param[in] p_buck        Pointer to the instance of buck.
 * @param[in] start_voltage Start test voltage level.
 */
static void verify_buck_voltage(npmx_buck_t *p_buck, npmx_buck_voltage_t start_voltage)
{
	for (uint8_t i = start_voltage; i <= NPMX_BUCK_VOLTAGE_MAX; i++) {
		/* Set the expected output voltage. */
		set_buck_voltage(p_buck, (npmx_buck_voltage_t)i);

		/* Wait some time to stabilize and measure the output voltage. */
		k_msleep(200);

		/* Check in nPM device which voltage is selected. */
		get_and_log_voltage(p_buck);
	}
}

/**
 * @brief Function for testing the output voltage of BUCK1.
 *
 * @param[in] p_buck Pointer to the instance of buck.
 */
static void test_output_voltage(npmx_buck_t *p_buck)
{
	LOG_INF("Test output voltage");

	npmx_error_t status;

	/* Voltage output testing for BUCK1. */
	verify_buck_voltage(p_buck, NPMX_BUCK_VOLTAGE_1V0);

	/* Go back to nominal voltage based on connected resistors.
	 * Note: without the load on output pins,
	 * the voltage can drop down a little slow.
	 */
	status = npmx_buck_vout_select_set(p_buck, NPMX_BUCK_VOUT_SELECT_VSET_PIN);
	if (status != NPMX_SUCCESS) {
		LOG_ERR("Unable to select vout reference");
		return;
	}

	/* With nPM1300-EK, output voltage should be 1.8 V for BUCK1. */
	get_and_log_voltage(p_buck);

	LOG_INF("Test output voltage OK");
}
#endif

#if CONFIG_TESTCASE_RETENTION_VOLTAGE
/**
 * @brief Function for testing the retention voltage with the selected external pin.
 *
 * @param[in] p_buck Pointer to the instance of buck.
 * @param[in] p_gpio Pointer to the instance of GPIO used for controlling retention mode.
 */
static void test_retention_voltage(npmx_buck_t *p_buck, npmx_gpio_t *p_gpio)
{
	/* GPIO1 as BUCK1 retention select. */
	LOG_INF("Test retention voltage");

	npmx_error_t status;

	/* Switch GPIO1 to input mode. */
	status = npmx_gpio_mode_set(p_gpio, NPMX_GPIO_MODE_INPUT);
	if (status != NPMX_SUCCESS) {
		LOG_ERR("Unable to switch GPIO1 to input mode");
		return;
	}

	/* Select voltages: in normal mode should be 1.7 V, in retention mode should be 3.3 V. */
	status = npmx_buck_normal_voltage_set(p_buck, NPMX_BUCK_VOLTAGE_1V7);
	if (status != NPMX_SUCCESS) {
		LOG_ERR("Unable to set normal voltage");
		return;
	}

	status = npmx_buck_retention_voltage_set(p_buck, NPMX_BUCK_VOLTAGE_3V3);
	if (status != NPMX_SUCCESS) {
		LOG_ERR("Unable to set retention voltage");
		return;
	}

	/* Apply voltages. */
	status = npmx_buck_vout_select_set(p_buck, NPMX_BUCK_VOUT_SELECT_SOFTWARE);
	if (status != NPMX_SUCCESS) {
		LOG_ERR("Unable to select vout reference");
		return;
	}

	/* Configuration for external pin. If inversion is false,
	 * retention is active when GPIO1 is in HIGH state.
	 */
	npmx_buck_gpio_config_t config = { .gpio = NPMX_BUCK_GPIO_1, .inverted = false };

	/* Set GPIO configuration to buck instance. */
	status = npmx_buck_retention_gpio_config_set(p_buck, &config);
	if (status != NPMX_SUCCESS) {
		LOG_ERR("Unable to select retention gpio");
		return;
	}

	LOG_INF("Test retention voltage OK");
}
#endif

#if TESTCASE_ENABLE_BUCKS_USING_PIN
/**
 * @brief Function for testing enable mode with the selected external pin.
 *
 * @param[in] p_buck Pointer to the instance of buck.
 * @param[in] p_gpio Pointer to the instance of GPIO used to enable buck.
 */
static void test_enable_bucks_using_pin(npmx_buck_t *p_buck, npmx_gpio_t *p_gpio)
{
	/* Enable and disable BUCK converter using selected GPIO pin. */
	LOG_INF("Test enable BUCK using connected pin");

	/* Disable BUCK instance for testing. */
	if (npmx_buck_task_trigger(p_buck, NPMX_BUCK_TASK_DISABLE) != NPMX_SUCCESS) {
		LOG_ERR("Unable to disable buck");
		return;
	}

	/* Switch GPIO3 to input mode. */
	if (npmx_gpio_mode_set(p_gpio, NPMX_GPIO_MODE_INPUT) != NPMX_SUCCESS) {
		LOG_ERR("Unable to switch GPIO3 to input mode");
		return;
	}

	/* Select output voltage to be 3.3 V. */
	set_buck_voltage(p_buck, NPMX_BUCK_VOLTAGE_3V3);

	/* Configuration for external pin. If inversion is false, */
	/* BUCK1 is active when GPIO1 is in HIGH state. */
	npmx_buck_gpio_config_t config = { .gpio = NPMX_BUCK_GPIO_3, .inverted = false };

	/* When GPIO3 changes to HIGH state, BUCK1 will start working. */
	if (npmx_buck_enable_gpio_config_set(p_buck, &config) != NPMX_SUCCESS) {
		LOG_ERR("Unable to connect GPIO3 to BUCK1");
		return;
	}

	/* Enable active discharge, so that the capacitor discharges faster,
	 * when there is no load connected to nPM1300.
	 */
	if (npmx_buck_active_discharge_enable_set(p_buck, true) != NPMX_SUCCESS) {
		LOG_ERR("Unable to activate auto discharge mode");
		return;
	}

	LOG_INF("Test enable BUCK using connected pin OK");
}
#endif

void main(void)
{
	const struct device *pmic_dev = DEVICE_DT_GET(DT_NODELABEL(npm_0));

	if (!device_is_ready(pmic_dev)) {
		LOG_INF("PMIC device is not ready");
		return;
	}

	LOG_INF("PMIC device ok");

	npmx_instance_t *npmx_instance = npmx_driver_instance_get(pmic_dev);

	npmx_buck_t *bucks[] = { npmx_buck_get(npmx_instance, BUCK1_IDX),
				 npmx_buck_get(npmx_instance, BUCK2_IDX) };

	/* After reset, BUCK converters are enabled by default,
	 * but to be sure, enable both at the beginning of the testing.
	 */
	npmx_buck_task_trigger(bucks[BUCK1_IDX], NPMX_BUCK_TASK_ENABLE);
	npmx_buck_task_trigger(bucks[BUCK2_IDX], NPMX_BUCK_TASK_ENABLE);

#if CONFIG_TESTCASE_SET_BUCK_VOLTAGE
	test_set_buck_voltage(bucks[BUCK1_IDX]);
#elif CONFIG_TESTCASE_OUTPUT_VOLTAGE
	test_output_voltage(bucks[BUCK1_IDX]);
#elif CONFIG_TESTCASE_RETENTION_VOLTAGE
	test_retention_voltage(bucks[BUCK1_IDX], npmx_gpio_get(npmx_instance, 1));
#elif CONFIG_TESTCASE_ENABLE_BUCKS_USING_PIN
	test_enable_bucks_using_pin(bucks[BUCK1_IDX], npmx_gpio_get(npmx_instance, 3));
#endif

	while (1) {
		k_sleep(K_FOREVER);
	}
}

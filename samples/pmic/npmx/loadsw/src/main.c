/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <npmx_buck.h>
#include <npmx_driver.h>
#include <npmx_gpio.h>
#include <npmx_ldsw.h>

#define LOG_MODULE_NAME pmic_loadsw
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/**
 * @brief Function for testing enabling and disabling LDSWs with software.
 *        In an infinite loop, switches are enabled and disabled alternately,
 *        with a delay of 5 seconds in between.
 *
 * @param[in] p_pm   Pointer to the PMIC instance.
 * @param[in] ldsw_1 Pointer to the LDSW instance.
 * @param[in] ldsw_2 Pointer to the LDSW instance.
 */
#if CONFIG_TESTCASE_SW_ENABLE
static void test_sw_enable(npmx_instance_t *p_pm, npmx_ldsw_t *ldsw_1, npmx_ldsw_t *ldsw_2)
{
	LOG_INF("Test enable LDSW with software");

	while (1) {
		/* Enable LDSW1 and disable LDSW2. */
		if (npmx_ldsw_task_trigger(ldsw_1, NPMX_LDSW_TASK_ENABLE) != NPMX_SUCCESS) {
			LOG_ERR("Unable to enable LDSW1");
			return;
		}

		if (npmx_ldsw_task_trigger(ldsw_2, NPMX_LDSW_TASK_DISABLE) != NPMX_SUCCESS) {
			LOG_ERR("Unable to disable LDSW2");
			return;
		}

		k_msleep(5000);

		/* Disable LDSW1 and enable LDSW2. */
		if (npmx_ldsw_task_trigger(ldsw_1, NPMX_LDSW_TASK_DISABLE) != NPMX_SUCCESS) {
			LOG_ERR("Unable to disable LDSW1");
			return;
		}

		if (npmx_ldsw_task_trigger(ldsw_2, NPMX_LDSW_TASK_ENABLE) != NPMX_SUCCESS) {
			LOG_ERR("Unable to enable LDSW2");
			return;
		}

		LOG_INF("Test enable LDSW with software OK");

		k_msleep(5000);
	}
}
#endif

/**
 * @brief Function for testing enabling and disabling LDSWs using external pins.
 *        When GPIO1 is in HIGH state, LDSW1 is enabled, otherwise disabled.
 *        When GPIO2 is in HIGH state, LDSW2 is enabled, otherwise disabled.
 *
 * @param[in] p_pm   Pointer to the PMIC instance.
 * @param[in] ldsw_1 Pointer to the LDSW instance.
 * @param[in] ldsw_2 Pointer to the LDSW instance.
 */
#if CONFIG_TESTCASE_GPIO_ENABLE
static void test_gpio_enable(npmx_instance_t *p_pm, npmx_ldsw_t *ldsw_1, npmx_ldsw_t *ldsw_2)
{
	LOG_INF("Test enable LDSW via connected pin");

	npmx_gpio_t *gpio_1 = npmx_gpio_get(p_pm, 1);
	npmx_gpio_t *gpio_2 = npmx_gpio_get(p_pm, 2);

	/* Switch GPIO1 and GPIO2 to input mode. */
	if (npmx_gpio_mode_set(gpio_1, NPMX_GPIO_MODE_INPUT) != NPMX_SUCCESS) {
		LOG_ERR("Unable to switch GPIO1 to input mode");
		return;
	}

	if (npmx_gpio_mode_set(gpio_2, NPMX_GPIO_MODE_INPUT) != NPMX_SUCCESS) {
		LOG_ERR("Unable to switch GPIO2 to input mode");
		return;
	}

	npmx_ldsw_gpio_config_t gpio_config = {
		.gpio = NPMX_LDSW_GPIO_1, /* GPIO1 to be used as the signal. */
#if defined(LDSW_LDSW1GPISEL_LDSW1GPIINV_Msk)
		.inverted = false, /* GPI state will not be inverted. */
#endif
	};

	/* Connect GPIO1 to enable LDSW1. */
	if (npmx_ldsw_enable_gpio_set(ldsw_1, &gpio_config) != NPMX_SUCCESS) {
		LOG_ERR("Unable to connect GPIO1 to LDSW1");
		return;
	}

	gpio_config.gpio = NPMX_LDSW_GPIO_2;

	/* Connect GPIO2 to enable LDSW2. */
	if (npmx_ldsw_enable_gpio_set(ldsw_2, &gpio_config) != NPMX_SUCCESS) {
		LOG_ERR("Unable to connect GPIO2 to LDSW2");
		return;
	}

	LOG_INF("Test enable LDSW via connected pin OK");
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

	/* Get the pointer to npmx device. */
	npmx_instance_t *npmx_instance = npmx_driver_instance_get(pmic_dev);

	npmx_buck_t *buck = npmx_buck_get(npmx_instance, 0);

	/* Set the output voltage of BUCK1. */
	npmx_buck_normal_voltage_set(buck, NPMX_BUCK_VOLTAGE_1V8);

	/* Have to be called each time to change output voltage of the BUCK1. */
	npmx_buck_vout_select_set(buck, NPMX_BUCK_VOUT_SELECT_SOFTWARE);

	/* Enable BUCK1. */
	npmx_buck_task_trigger(buck, NPMX_BUCK_TASK_ENABLE);

	npmx_ldsw_t *ldsw_1 = npmx_ldsw_get(npmx_instance, 0);
	npmx_ldsw_t *ldsw_2 = npmx_ldsw_get(npmx_instance, 1);

	/* Enable active discharge, to see changes on output pins. */
	npmx_ldsw_active_discharge_enable_set(ldsw_1, true);
	npmx_ldsw_active_discharge_enable_set(ldsw_2, true);

#if CONFIG_TESTCASE_SW_ENABLE
	test_sw_enable(npmx_instance, ldsw_1, ldsw_2);
#elif CONFIG_TESTCASE_GPIO_ENABLE
	test_gpio_enable(npmx_instance, ldsw_1, ldsw_2);
#endif

	while (1) {
		k_sleep(K_FOREVER);
	}
}

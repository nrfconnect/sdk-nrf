/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <npmx_driver.h>
#include <npmx_vbusin.h>
#include <npmx_core.h>

#define LOG_MODULE_NAME pmic_vbusin
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/**
 * @brief Callback function to be used when VBUSIN VOLTAGE event occurs.
 *
 * @param[in] p_pm Pointer to the instance of nPM device.
 * @param[in] type Callback type. Should be @ref NPMX_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE.
 * @param[in] mask Received event interrupt mask.
 */
void vbusin_voltage_callback(npmx_instance_t *p_pm, npmx_callback_type_t type, uint8_t mask)
{
	/* Task current apply have to be triggered each time when USB is (re)connected. */
	if (mask & NPMX_EVENT_GROUP_VBUSIN_DETECTED_MASK) {
		npmx_vbusin_task_trigger(npmx_vbusin_get(p_pm, 0),
					 NPMX_VBUSIN_TASK_APPLY_CURRENT_LIMIT);
	}

	/* Call generic callback to print interrupt data. */
	p_pm->generic_cb(p_pm, type, mask);
}

/**
 * @brief Callback function to be used when VBUSIN THERMAL event occurs.
 *
 * @param[in] p_pm Pointer to the instance of nPM device.
 * @param[in] type Callback type. Should be @ref NPMX_CALLBACK_TYPE_EVENT_VBUSIN_THERMAL_USB.
 * @param[in] mask Received event interrupt mask.
 */
void vbusin_thermal_callback(npmx_instance_t *p_pm, npmx_callback_type_t type, uint8_t mask)
{
	npmx_vbusin_cc_t cc1;
	npmx_vbusin_cc_t cc2;

	/* Get the status of CC lines. */
	if (npmx_vbusin_cc_status_get(npmx_vbusin_get(p_pm, 0), &cc1, &cc2) == NPMX_SUCCESS) {
		LOG_INF("CC1: %s", npmx_vbusin_cc_status_map_to_string(cc1));
		LOG_INF("CC2: %s", npmx_vbusin_cc_status_map_to_string(cc2));
	} else {
		LOG_ERR("Unable to read CC lines status");
	}

	/* Call generic callback to print interrupt data. */
	p_pm->generic_cb(p_pm, type, mask);
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

	/* Register callback for VBUSIN VALTAGE event. */
	npmx_core_register_cb(npmx_instance, vbusin_voltage_callback,
			      NPMX_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE);

	/* Register callback for VBUSIN THERMAL event used for detecting CC lines status. */
	npmx_core_register_cb(npmx_instance, vbusin_thermal_callback,
			      NPMX_EVENT_GROUP_VBUSIN_THERMAL);

	/* Enable detecting connected and removed USB. */
	npmx_core_event_interrupt_enable(npmx_instance, NPMX_EVENT_GROUP_VBUSIN_VOLTAGE,
					 NPMX_EVENT_GROUP_VBUSIN_DETECTED_MASK |
						 NPMX_EVENT_GROUP_VBUSIN_REMOVED_MASK);

	/* Enable detecting CC status changed. */
	npmx_core_event_interrupt_enable(npmx_instance, NPMX_EVENT_GROUP_VBUSIN_THERMAL,
					 NPMX_EVENT_GROUP_USB_CC1_MASK |
						 NPMX_EVENT_GROUP_USB_CC2_MASK);

	/* Set the current limit for the USB port. */
	/* Current limit needs to be applied after each USB (re)connection. */
	npmx_vbusin_current_limit_set(npmx_vbusin_get(npmx_instance, 0),
				      NPMX_VBUSIN_CURRENT_100_MA);

	LOG_INF("VBUSIN initialized");

	while (1) {
		k_sleep(K_FOREVER);
	}
}

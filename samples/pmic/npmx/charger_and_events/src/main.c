/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <npmx_adc.h>
#include <npmx_charger.h>
#include <npmx_core.h>
#include <npmx_driver.h>
#include <npmx_errlog.h>

#define LOG_MODULE_NAME pmic_charger
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/** @brief Possible events from requested nPM device. */
enum npm1300_charger_event_t {
	/* Event registered when battery connection detected. */
	APP_CHARGER_EVENT_BATTERY_DETECTED,
	/* Event registered when battery connection removed. */
	APP_CHARGER_EVENT_BATTERY_REMOVED,
	/* Event registered when VBUS connection detected. */
	APP_CHARGER_EVENT_VBUS_DETECTED,
	/* Event registered when VBSU connection removed. */
	APP_CHARGER_EVENT_VBUS_REMOVED,
	/* Event registered when trickle charging started. */
	APP_CHARGER_EVENT_CHARGING_TRICKE_STARTED,
	/* Event registered when constant current charging started. */
	APP_CHARGER_EVENT_CHARGING_CC_STARTED,
	/* Event registered when constant voltage charging started. */
	APP_CHARGER_EVENT_CHARGING_CV_STARTED,
	/* Event registered when charging completed. */
	APP_CHARGER_EVENT_CHARGING_COMPLETED,
	/* Event registered when first low battery voltage alert detected. */
	APP_CHARGER_EVENT_BATTERY_LOW_ALERT1,
	/* Event registered when second low battery voltage alert detected. */
	APP_CHARGER_EVENT_BATTERY_LOW_ALERT2,
};

/** @brief Possible nPM device working states. */
enum npm1300_state_t {
	/* State when VBUSIN disconnected and battery disconnected. */
	APP_STATE_BATTERY_DISCONNECTED,
	/* State when VBUSIN disconnected and battery connected. */
	APP_STATE_BATTERY_CONNECTED,
	/* State when VBUSIN connected and battery disconnected. */
	APP_STATE_VBUS_CONNECTED_BATTERY_DISCONNECTED,
	/* State when VBUSIN connected and battery connected. */
	APP_STATE_VBUS_CONNECTED_BATTERY_CONNECTED,
	/* State when VBUSIN connected, battery connected and charger in trickle mode. */
	APP_STATE_VBUS_CONNECTED_CHARGING_TRICKE,
	/* State when VBUSIN connected, battery connected and charger in constant current mode. */
	APP_STATE_VBUS_CONNECTED_CHARGING_CC,
	/* State when VBUSIN connected, battery connected and charger in constant voltage mode. */
	APP_STATE_VBUS_CONNECTED_CHARGING_CV,
	/* State when VBUSIN connected, battery connected and charger completed charging. */
	APP_STATE_VBUS_CONNECTED_CHARGING_COMPLETED,
	/* State when VBUSIN disconnected, battery connected. */
	APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING,
	/* State when VBUSIN disconnected. */
	/* Battery connected and battery voltage is below first alert threshold. */
	APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING_ALERT1,
	/* State when VBUSIN disconnected. */
	/* Battery connected and battery voltage is below second alert threshold. */
	APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING_ALERT2,
};

/**
 * @brief Register the new event received from nPM device.
 *
 * @param[in] event New event type.
 */
static void register_state_change(enum npm1300_charger_event_t event)
{
	static enum npm1300_state_t state = APP_STATE_BATTERY_DISCONNECTED;

	switch (event) {
	case APP_CHARGER_EVENT_BATTERY_DETECTED:
		if (state == APP_STATE_BATTERY_DISCONNECTED) {
			state = APP_STATE_BATTERY_CONNECTED;
			LOG_INF("State: BATTERY_CONNECTED");
		}

		if (state == APP_STATE_VBUS_CONNECTED_BATTERY_DISCONNECTED) {
			state = APP_STATE_VBUS_CONNECTED_BATTERY_CONNECTED;
			LOG_INF("State: VBUS_CONNECTED_BATTERY_CONNECTED");
		}

		if (state == APP_STATE_VBUS_CONNECTED_CHARGING_TRICKE ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_CC ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_CV ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_COMPLETED) {
			LOG_INF("State: BATTERY_CONNECTED");
		}
		break;
	case APP_CHARGER_EVENT_BATTERY_REMOVED:
		if (state == APP_STATE_BATTERY_CONNECTED) {
			state = APP_STATE_BATTERY_DISCONNECTED;
			LOG_INF("State: BATTERY_DISCONNECTED");
		}

		if (state == APP_STATE_VBUS_CONNECTED_BATTERY_CONNECTED ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_TRICKE ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_CC ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_CV ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_COMPLETED) {
			state = APP_STATE_VBUS_CONNECTED_BATTERY_DISCONNECTED;
			LOG_INF("State: VBUS_CONNECTED_BATTERY_DISCONNECTED");
		}
		break;
	case APP_CHARGER_EVENT_VBUS_DETECTED:
		if (state == APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING ||
		    state == APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING_ALERT1 ||
		    state == APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING_ALERT2) {
			state = APP_STATE_VBUS_CONNECTED_BATTERY_CONNECTED;
			LOG_INF("State: VBUS_CONNECTED_BATTERY_CONNECTED");
		}
		if (state == APP_STATE_BATTERY_DISCONNECTED) {
			state = APP_STATE_VBUS_CONNECTED_BATTERY_DISCONNECTED;
			LOG_INF("State: VBUS_CONNECTED_BATTERY_DISCONNECTED");
		}
		break;
	case APP_CHARGER_EVENT_VBUS_REMOVED:
		if (state == APP_STATE_VBUS_CONNECTED_CHARGING_TRICKE ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_CC ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_CV ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_COMPLETED ||
		    state == APP_STATE_VBUS_CONNECTED_BATTERY_CONNECTED) {
			state = APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING;
			LOG_INF("State: VBUS_NOT_CONNECTED_DISCHARGING");
		}
		break;
	case APP_CHARGER_EVENT_CHARGING_TRICKE_STARTED:
		if (state != APP_STATE_VBUS_CONNECTED_CHARGING_TRICKE &&
		    state != APP_STATE_VBUS_CONNECTED_CHARGING_CC &&
		    state != APP_STATE_VBUS_CONNECTED_CHARGING_CV &&
		    state != APP_STATE_VBUS_CONNECTED_CHARGING_COMPLETED) {
			state = APP_STATE_VBUS_CONNECTED_CHARGING_TRICKE;
			LOG_INF("State: VBUS_CONNECTED_CHARGING_TRICKE");
		}
		break;
	case APP_CHARGER_EVENT_CHARGING_CC_STARTED:
		if (state != APP_STATE_VBUS_CONNECTED_CHARGING_CC &&
		    state != APP_STATE_VBUS_CONNECTED_CHARGING_CV) {
			state = APP_STATE_VBUS_CONNECTED_CHARGING_CC;
			LOG_INF("State: VBUS_CONNECTED_CHARGING_CC");
		}
		break;
	case APP_CHARGER_EVENT_CHARGING_CV_STARTED:
		if (state != APP_STATE_VBUS_CONNECTED_CHARGING_CV) {
			state = APP_STATE_VBUS_CONNECTED_CHARGING_CV;
			LOG_INF("State: VBUS_CONNECTED_CHARGING_CV");
		}
		break;
	case APP_CHARGER_EVENT_CHARGING_COMPLETED:
		if (state != APP_STATE_VBUS_CONNECTED_CHARGING_COMPLETED) {
			state = APP_STATE_VBUS_CONNECTED_CHARGING_COMPLETED;
			LOG_INF("State: VBUS_CONNECTED_CHARGING_COMPLETED");
		}
		break;
	case APP_CHARGER_EVENT_BATTERY_LOW_ALERT1:
		if (state == APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING) {
			state = APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING_ALERT1;
			LOG_INF("State: VBUS_NOT_CONNECTED_DISCHARGING_ALERT1");
		}
		break;
	case APP_CHARGER_EVENT_BATTERY_LOW_ALERT2:
		if (state == APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING ||
		    state == APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING_ALERT1) {
			state = APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING_ALERT2;
			LOG_INF("State: VBUS_NOT_CONNECTED_DISCHARGING_ALERT2\n");
		}
		break;

	default:
		LOG_INF("Unsupported event: %d", event);
		break;
	}
}

/**
 * @brief Function callback for VBUSIN events.
 *
 * @param[in] p_pm Pointer to the instance of nPM device.
 * @param[in] type Type of callback, should be always NPMX_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE.
 * @param[in] mask Received event mask @ref npmx_event_group_vbusin_mask_t .
 */
static void vbusin_callback(npmx_instance_t *p_pm, npmx_callback_type_t type, uint8_t mask)
{
	if (mask & (uint8_t)NPMX_EVENT_GROUP_VBUSIN_DETECTED_MASK) {
		register_state_change(APP_CHARGER_EVENT_VBUS_DETECTED);
	}

	if (mask & (uint8_t)NPMX_EVENT_GROUP_VBUSIN_REMOVED_MASK) {
		register_state_change(APP_CHARGER_EVENT_VBUS_REMOVED);
	}
}

/**
 * @brief Function callback for ADC events.
 *
 * @param[in] p_pm Pointer to the instance of nPM device.
 * @param[in] type Type of callback, should be always NPMX_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE.
 * @param[in] mask Received event mask @ref npmx_event_group_vbusin_mask_t.
 */
static void adc_callback(npmx_instance_t *p_pm, npmx_callback_type_t type, uint8_t mask)
{
	if ((mask & (uint8_t)NPMX_EVENT_GROUP_ADC_BAT_READY_MASK)) {
		static uint16_t battery_voltage_millivolts_last;
		uint16_t battery_voltage_millivolts;

		if (npmx_adc_meas_get(npmx_adc_get(p_pm, 0), NPMX_ADC_MEAS_VBAT,
				      &battery_voltage_millivolts) == NPMX_SUCCESS) {
			if (battery_voltage_millivolts != battery_voltage_millivolts_last) {
				battery_voltage_millivolts_last = battery_voltage_millivolts;
				LOG_INF("Battery:\t %d mV", battery_voltage_millivolts);
			}
			if (battery_voltage_millivolts < CONFIG_BATTERY_VOLTAGE_THRESHOLD_2) {
				register_state_change(APP_CHARGER_EVENT_BATTERY_LOW_ALERT2);
			} else if (battery_voltage_millivolts <
				   CONFIG_BATTERY_VOLTAGE_THRESHOLD_1) {
				register_state_change(APP_CHARGER_EVENT_BATTERY_LOW_ALERT1);
			}
		}
	}
}

/**
 * @brief Function callback for charger status events.
 *
 * @param[in] p_pm Pointer to the instance of nPM device.
 * @param[in] type Type of callback, should always be NPMX_CALLBACK_TYPE_EVENT_BAT_CHAR_STATUS.
 * @param[in] mask Received event mask @ref npmx_event_group_charger_mask_t .
 */
static void charger_status_callback(npmx_instance_t *p_pm, npmx_callback_type_t type, uint8_t mask)
{
	npmx_charger_t *charger_instance = npmx_charger_get(p_pm, 0);

	if (mask & (uint8_t)NPMX_EVENT_GROUP_CHARGER_ERROR_MASK) {
		/* Check charger errors and run default debug callbacks to log error bits. */
		npmx_charger_errors_check(charger_instance);
	}

	npmx_charger_status_mask_t status;

	/* Delay required for status stabilization. */
	k_msleep(5);

	if (npmx_charger_status_get(charger_instance, &status) == NPMX_SUCCESS) {
		if (status & NPMX_CHARGER_STATUS_TRICKLE_CHARGE_MASK) {
			register_state_change(APP_CHARGER_EVENT_CHARGING_TRICKE_STARTED);
		}

		if (status & NPMX_CHARGER_STATUS_CONSTANT_CURRENT_MASK) {
			register_state_change(APP_CHARGER_EVENT_CHARGING_CC_STARTED);
		}

		if (status & NPMX_CHARGER_STATUS_CONSTANT_VOLTAGE_MASK) {
			register_state_change(APP_CHARGER_EVENT_CHARGING_CV_STARTED);
		}

		if (status & NPMX_CHARGER_STATUS_COMPLETED_MASK) {
			register_state_change(APP_CHARGER_EVENT_CHARGING_COMPLETED);
		}
	}
}

/**
 * @brief Function callback for battery events.
 *
 * @param[in] p_pm The pointer to the instance of nPM device.
 * @param[in] type The type of callback, should always be NPMX_CALLBACK_TYPE_EVENT_BAT_CHAR_BAT.
 * @param[in] mask Received event mask @ref npmx_event_group_battery_mask_t .
 */
static void charger_battery_callback(npmx_instance_t *p_pm, npmx_callback_type_t type, uint8_t mask)
{
	if (mask & (uint8_t)NPMX_EVENT_GROUP_BATTERY_DETECTED_MASK) {
		register_state_change(APP_CHARGER_EVENT_BATTERY_DETECTED);
	}

	if (mask & (uint8_t)NPMX_EVENT_GROUP_BATTERY_REMOVED_MASK) {
		register_state_change(APP_CHARGER_EVENT_BATTERY_REMOVED);
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

	/* Get pointer to npmx device. */
	npmx_instance_t *npmx_instance = npmx_driver_instance_get(pmic_dev);

	/* Get pointer to CHARGER instance. */
	npmx_charger_t *charger_instance = npmx_charger_get(npmx_instance, 0);

	uint8_t mask;

	/* Check if VBUS is connected. */
	npmx_vbusin_vbus_status_get(npmx_vbusin_get(npmx_instance, 0), &mask);
	if (mask & NPMX_VBUSIN_STATUS_CONNECTED_MASK) {
		register_state_change(APP_CHARGER_EVENT_VBUS_DETECTED);
	}

	/* Register callback for VBUS events. */
	npmx_core_register_cb(npmx_instance, vbusin_callback,
			      NPMX_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE);

	/* Register callback for ADC events. */
	npmx_core_register_cb(npmx_instance, adc_callback, NPMX_CALLBACK_TYPE_EVENT_ADC);

	/* Register callback for battery events. */
	npmx_core_register_cb(npmx_instance, charger_battery_callback,
			      NPMX_CALLBACK_TYPE_EVENT_BAT_CHAR_BAT);

	/* Register callback for charger status events. */
	npmx_core_register_cb(npmx_instance, charger_status_callback,
			      NPMX_CALLBACK_TYPE_EVENT_BAT_CHAR_STATUS);

	/* Check reset errors and run default debug callbacks to log error bits. */
	npmx_errlog_reset_errors_check(npmx_errlog_get(npmx_instance, 0));

	/* Disable NTC module to work with two-wire batteries. */
	npmx_charger_module_disable_set(charger_instance, NPMX_CHARGER_MODULE_NTC_LIMITS_MASK);

	/* Disable charger before changing charge current. */
	npmx_charger_module_disable_set(charger_instance, NPMX_CHARGER_MODULE_CHARGER_MASK);

	/* Set charging current. */
	npmx_charger_charging_current_set(charger_instance, CONFIG_CHARGING_CURRENT);

	/* Set battery termination voltage. */
	npmx_charger_termination_normal_voltage_set(
		charger_instance, npmx_charger_voltage_convert(CONFIG_TERMINATION_VOLTAGE));

	/* Enable charger for events handling. */
	npmx_charger_module_enable_set(charger_instance, NPMX_CHARGER_MODULE_CHARGER_MASK |
								 NPMX_CHARGER_MODULE_RECHARGE_MASK);

	/* Enable USB connections interrupts and events handling. */
	npmx_core_event_interrupt_enable(npmx_instance, NPMX_EVENT_GROUP_VBUSIN_VOLTAGE,
					 NPMX_EVENT_GROUP_VBUSIN_DETECTED_MASK |
						 NPMX_EVENT_GROUP_VBUSIN_REMOVED_MASK);

	/* Enable all charging status interrupts and events. */
	npmx_core_event_interrupt_enable(
		npmx_instance, NPMX_EVENT_GROUP_BAT_CHAR_STATUS,
		NPMX_EVENT_GROUP_CHARGER_SUPPLEMENT_MASK | NPMX_EVENT_GROUP_CHARGER_TRICKLE_MASK |
			NPMX_EVENT_GROUP_CHARGER_CC_MASK | NPMX_EVENT_GROUP_CHARGER_CV_MASK |
			NPMX_EVENT_GROUP_CHARGER_COMPLETED_MASK |
			NPMX_EVENT_GROUP_CHARGER_ERROR_MASK);

	/* Enable battery interrupts and events. */
	npmx_core_event_interrupt_enable(npmx_instance, NPMX_EVENT_GROUP_BAT_CHAR_BAT,
					 NPMX_EVENT_GROUP_BATTERY_DETECTED_MASK |
						 NPMX_EVENT_GROUP_BATTERY_REMOVED_MASK);

	/* Enable ADC measurements ready interrupts. */
	npmx_core_event_interrupt_enable(npmx_instance, NPMX_EVENT_GROUP_ADC,
					 NPMX_EVENT_GROUP_ADC_BAT_READY_MASK);

	/* Set NTC type for ADC measurements. */
	npmx_adc_ntc_set(npmx_adc_get(npmx_instance, 0), NPMX_ADC_NTC_TYPE_10_K);

	/* Enable ADC auto measurements every ~1s (default). */
	npmx_adc_config_t config = { .vbat_auto = true, .vbat_burst = false };

	npmx_adc_config_set(npmx_adc_get(npmx_instance, 0), &config);

	/* Check if charger errors occurred before the application starts. */
	npmx_charger_errors_check(charger_instance);

	while (1) {
		k_sleep(K_FOREVER);
	}
}

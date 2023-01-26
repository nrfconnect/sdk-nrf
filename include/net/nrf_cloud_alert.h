/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_ALERT_H_
#define NRF_CLOUD_ALERT_H_

/** @file nrf_cloud_alert.h
 * @brief Module to provide mechanism to send application alerts to nRF Cloud.
 */

#include <zephyr/kernel.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

struct nrf_cloud_rest_context;

/** @brief Special value to pass when value should not be sent. */
#define NRF_CLOUD_ALERT_UNUSED_VALUE NAN

/** @defgroup nrf_cloud_alert nRF Cloud Alert
 * @{
 */

/** @brief Alert type */
enum nrf_cloud_alert_type {
	/** Placeholder; not meant to be sent. */
	ALERT_TYPE_NONE,
	/** Device finished booting. */
	ALERT_TYPE_DEVICE_NOW_ONLINE,
	/** Device finished rebooting from a watchdog timeout. */
	ALERT_TYPE_DEVICE_NOW_ONLINE_WDT,
	/** Device is shutting down. */
	ALERT_TYPE_DEVICE_GOING_OFFLINE,
	/** Generic message (see description field). */
	ALERT_TYPE_MSG,
	/** Operating temperature outside of limits. Value is in degrees Celsius. */
	ALERT_TYPE_TEMPERATURE,
	/** Operating humidity outside of limits. Value is in percent relative humidity. */
	ALERT_TYPE_HUMIDITY,
	/** Battery level too low. Value is in volts. */
	ALERT_TYPE_BATTERY,
	/** Shock and vibration limits exceeded. Value is in m/s^2. */
	ALERT_TYPE_SHOCK,
	/** User-specific alerts start here. */
	ALERT_TYPE_CUSTOM = 100
};

/** @brief Alert data */
struct nrf_cloud_alert_info {
	/** The type of alert this represents. */
	enum nrf_cloud_alert_type type;
	/** A value associated with this alert. */
	float value;
	/** An optional human-readable description. */
	const char *description;
	/** A monotonically increasing alert counter used when current time not available. */
	unsigned int sequence;
	/** UNIX epoch timestamp (in milliseconds) of the data.
	 * If a value <= NRF_CLOUD_NO_TIMESTAMP (0) is provided, the timestamp will be ignored.
	 */
	int64_t ts_ms;
};

/**
 * @brief Transmit the specified alert to nRF Cloud using MQTT.
 *
 * @param[in] type The type of alert.
 * @param[in] value Optional numeric value associated with the alert. If set to
 *                  NRF_CLOUD_ALERT_UNUSED_VALUE, the library will not send this
 *                  field to nRF Cloud.
 * @param[in] description Optional text describing the alert.
 *
 * @return 0 if alert is successfully sent, otherwise, a negative error number.
 */
#if defined(CONFIG_NRF_CLOUD_ALERT) || defined(__DOXYGEN__)
int nrf_cloud_alert_send(enum nrf_cloud_alert_type type, float value, const char *description);
#else
static inline int nrf_cloud_alert_send(enum nrf_cloud_alert_type type, float value,
				       const char *description)
{
	return 0;
}
#endif /* CONFIG_NRF_CLOUD_ALERT */

/**
 * @brief Transmit the specified alert to nRF Cloud using REST.
 *
 * @param[in,out] rest_ctx Context for communicating with nRF Cloud's REST API.
 * @param[in]     device_id Null-terminated, unique device ID registered with nRF Cloud.
 * @param[in]     type The type of alert.
 * @param[in] value Optional numeric value associated with the alert. If set to
 *                  NRF_CLOUD_ALERT_UNUSED_VALUE, the library will not send this
 *                  field to nRF Cloud.
 * @param[in]     description Optional text describing the alert.
 *
 * @return 0 if alert is successfully sent, otherwise, a negative error number.
 */
#if defined(CONFIG_NRF_CLOUD_ALERT) || defined(__DOXYGEN__)
int nrf_cloud_rest_alert_send(struct nrf_cloud_rest_context *const rest_ctx,
			      const char *const device_id,
			      enum nrf_cloud_alert_type type,
			      float value,
			      const char *description);
#else
static inline int nrf_cloud_rest_alert_send(struct nrf_cloud_rest_context *const rest_ctx,
					    const char *const device_id,
					    enum nrf_cloud_alert_type type,
					    float value,
					    const char *description)
{
	return 0;
}
#endif /* CONFIG_NRF_CLOUD_ALERT */

/**
 * @brief Enable or disable use of alerts.
 *
 * @param[in] alerts_enable Value for alerts enabled.
 */
void nrf_cloud_alert_control_set(bool alerts_enable);

/**
 * @brief Find out if alerts are enabled.
 *
 * @return current alert enablement status.
 *
 */
bool nrf_cloud_alert_control_get(void);

/** @} */

#ifdef __cplusplus
#endif

#endif /* NRF_CLOUD_ALERT_H_ */

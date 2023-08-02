/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_LOG_H_
#define NRF_CLOUD_LOG_H_

/** @file nrf_cloud_log.h
 * @brief Module to provide nRF Cloud logging support to nRF9160 SiP.
 */

#include <zephyr/logging/log.h>

#ifdef __cplusplus
extern "C" {
#endif

struct nrf_cloud_rest_context;

/**
 * @defgroup nrf_cloud_log nRF Cloud Log
 * @{
 */

/**
 * @brief Initialize the nRF Cloud logging subsystem. Call after
 * the date_time library is ready.
 */
void nrf_cloud_log_init(void);

/**
 * @brief Set REST context for logging subsystem.
 *
 * Tell REST-based logger the REST context and device_id for later
 * use when outputting a log. This is used by the nRF Cloud logs backend
 * when CONFIG_NRF_CLOUD_LOG_BACKEND=y.
 *
 * @param[in] ctx REST context to use.
 * @param[in] dev_id Device ID to send message on behalf of.
 */
void nrf_cloud_log_rest_context_set(struct nrf_cloud_rest_context *ctx, const char *dev_id);

/**
 * @brief Set the logging level.
 *
 * Set the log level, but do not change whether the nRF Cloud logging backend
 * is enabled.
 *
 * Log messages with a log level larger than this will not be sent.
 * For example, if log_level is LOG_LEVEL_WRN (2), then
 * LOG_LEVEL_WRN (2) and LOG_LEVEL_ERR (1) messages will be sent, and
 * LOG_LEVEL_INF (3) and LOG_LEVEL_DBG (4) will not.
 * Set log_level to LOG_LEVEL_NONE (0) to disable logging.
 * See zephyr/include/zephyr/logging/log_core.h.
 *
 * @param level The log level.
 */
void nrf_cloud_log_level_set(int level);

/**
 * @brief Enable or disable logging to the cloud.
 *
 * When CONFIG_NRF_CLOUD_LOG_BACKEND=y, will enable or disable the log backend,
 * without changing the log level. Disable the log backend to conserve power by
 * eliminating unnecessary processing of log messages.
 *
 * When CONFIG_NRF_CLOUD_LOG_DIRECT=y, the log enable state determines whether
 * calls to nrf_cloud_log_send() or nrf_cloud_rest_log_send() will succeed.
 * This will conserve power by not transmitting log messages when not needed.
 *
 * @param[in] enable Set true to send logs to the cloud, false to disable.
 */
void nrf_cloud_log_enable(bool enable);

/**
 * @brief Find out if logger is enabled.
 *
 * @retval True if logger is enabled.
 */
bool nrf_cloud_log_is_enabled(void);

/**
 * @brief Control nRF Cloud logging.
 *
 * Set log level. If the level is set to LOG_LEVEL_NONE, the nRF Cloud
 * logging backend will be disabled, otherwise it will be enabled.
 * This combines the action of nrf_cloud_log_enable() and nrf_cloud_log_level_set().
 *
 * @param log_level The log level to go to the cloud.
 */
void nrf_cloud_log_control_set(int log_level);

/**
 * @brief Get current log level for logging to nRF Cloud.
 *
 * @retval Current log level.
 */
int nrf_cloud_log_control_get(void);

/**
 * @brief Determine if build is configured for text (JSON) cloud logging.
 *
 * @return bool True if build for JSON-based logs. False otherwise.
 */
bool nrf_cloud_is_text_logging_enabled(void);

/**
 * @brief Determine if build is configured for dictionary (binary) logging.
 *
 * @return bool True if build for dictionary logs. False otherwise.
 */
bool nrf_cloud_is_dict_logging_enabled(void);

#if defined(CONFIG_NRF_CLOUD_LOG_DIRECT)
#if defined(CONFIG_NRF_CLOUD_MQTT) || defined(CONFIG_NRF_CLOUD_COAP)
/**
 * @brief Directly log to the cloud. This does not use the Zephyr logging
 * system if CONFIG_NRF_CLOUD_LOG_BACKEND is disabled. Otherwise, it is passed to
 * the logging system.
 *
 * @param log_level The log level for this message; dropped if above set log level.
 * @param fmt Format string.
 * @param ... Format arguments.
 * @return 0 if log line is successfully sent, otherwise, a negative error number.
 */
int nrf_cloud_log_send(int log_level, const char *fmt, ...);

#elif defined(CONFIG_NRF_CLOUD_REST)
/**
 * @brief Directly log to the cloud. This does not use the Zephyr logging
 * system if CONFIG_NRF_CLOUD_LOG_BACKEND is disabled. Otherwise, it is passed to
 * the logging system.
 *
 * @param ctx REST interface context to use to send log message.
 * @param dev_id Device id to use when sending the log message via REST.
 * @param log_level The log level for this message; dropped if above set log level.
 * @param fmt Format string.
 * @param ... Format arguments.
 * @return 0 if log line is successfully sent, otherwise, a negative error number.
 */
int nrf_cloud_rest_log_send(struct nrf_cloud_rest_context *ctx, const char *dev_id,
			     int log_level, const char *fmt, ...);
#endif /* CONFIG_NRF_CLOUD_REST */
#else /* CONFIG_NRF_CLOUD_LOG_DIRECT */
#define nrf_cloud_log_send(log_level, fmt, ...) (0)
#endif /* CONFIG_NRF_CLOUD_LOG_DIRECT */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_LOG_H_ */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MODEM_SLM_H_
#define MODEM_SLM_H_

/**
 * @file modem_slm.h
 *
 * @defgroup modem_slm Modem SLM library
 *
 * @{
 *
 * @brief Public APIs for the Modem SLM library.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain/common.h>

/** Max size of AT command response is 2100 bytes. */
#define SLM_AT_CMD_RESPONSE_MAX_LEN 2100

/**
 * @brief AT command result codes
 */
enum at_cmd_state {
	AT_CMD_OK,
	AT_CMD_ERROR,
	AT_CMD_ERROR_CMS,
	AT_CMD_ERROR_CME,
	AT_CMD_PENDING
};

/**
 * @typedef slm_data_handler_t
 *
 * Handler to handle data received from SLM, which could be AT response, AT notification
 * or simply raw data (for example DFU image).
 *
 * @param data    Data received from SLM.
 * @param datalen Length of the data received.
 *
 * @note The handler runs from uart callback. It must not call @ref modem_slm_send_cmd. The data
 * should be copied out by the application as soon as called.
 */
typedef void (*slm_data_handler_t)(const uint8_t *data, size_t datalen);

/**
 * @typedef slm_ind_handler_t
 *
 * Handler to handle MODEM_SLM_INDICATE_PIN signal from SLM.
 */
typedef void (*slm_ind_handler_t)(void);

/**@brief Initialize Modem SLM library.
 *
 * @param handler Pointer to a handler function of type @ref slm_data_handler_t.
 *
 * @return Zero on success, non-zero otherwise.
 */
int modem_slm_init(slm_data_handler_t handler);

/**@brief Un-initialize Modem SLM library.
 *
 */
int modem_slm_uninit(void);

/**
 * @brief Register callback for MODEM_SLM_INDICATE_PIN indication
 *
 * @param handler Pointer to a handler function of type @ref slm_ind_handler_t.
 * @param wakeup  Enable/disable System Off wakeup by GPIO Sense.
 *
 * @retval Zero    Success.
 * @retval -EFAULT if MODEM_SLM_INDICATE_PIN is not defined.
 */
int modem_slm_register_ind(slm_ind_handler_t handler, bool wakeup);

/**
 * @brief Wakeup nRF9160 SiP via MODEM_SLM_WAKEUP_PIN
 *
 * @return Zero on success, non-zero otherwise.
 */
int modem_slm_wake_up(void);

/**
 * @brief Reset the RX function of the serial interface
 *
 */
void modem_slm_reset_uart(void);

/**
 * @brief Function to send an AT command in SLM command mode
 *
 * This function wait until command result is received. The response of the AT command is received
 * via the @ref slm_ind_handler_t registered in @ref modem_slm_init.
 *
 * @param command Pointer to null terminated AT command string without command terminator
 * @param timeout Response timeout for the command in seconds, Zero means infinite wait
 *
 * @retval state @ref at_cmd_state if command execution succeeds.
 * @retval -EAGAIN if command execution times out.
 * @retval other if command execution fails.
 */
int modem_slm_send_cmd(const char *const command, uint32_t timeout);

/**
 * @brief Function to send raw data in SLM data mode
 *
 * @param data    Raw data to send
 * @param datalen Length of the raw data
 *
 * @return Zero on success, non-zero otherwise.
 */
int modem_slm_send_data(const uint8_t *const data, size_t datalen);

/**
 * @brief SLM monitor callback.
 *
 * @param notif The AT notification callback.
 */
typedef void (*slm_monitor_handler_t)(const char *notif);

/**
 * @brief SLM monitor entry.
 */
struct slm_monitor_entry {
	/** The filter for this monitor. */
	const char *filter;
	/** Monitor callback. */
	const slm_monitor_handler_t handler;
	/** Monitor is paused. */
	uint8_t paused;
};

/** Wildcard. Match any notifications. */
#define MON_ANY NULL
/** Monitor is paused. */
#define MON_PAUSED 1
/** Monitor is active, default */
#define MON_ACTIVE 0

/**
 * @brief Define an SLM monitor to receive notifications in the system workqueue thread.
 *
 * @param name The monitor's name.
 * @param _filter The filter for AT notification the monitor should receive,
 *		  or @c MON_ANY to receive all notifications.
 * @param _handler The monitor callback.
 * @param ... Optional monitor initial state (@c MON_PAUSED or @c MON_ACTIVE).
 *	      The default initial state of a monitor is @c MON_ACTIVE.
 */
#define SLM_MONITOR(name, _filter, _handler, ...)                                                  \
	static void _handler(const char *);                                                        \
	static STRUCT_SECTION_ITERABLE(slm_monitor_entry, name) = {                                \
		.filter = _filter,                                                                 \
		.handler = _handler,                                                               \
		COND_CODE_1(__VA_ARGS__, (.paused = __VA_ARGS__,), ())                             \
	}

/**
 * @brief Pause monitor.
 *
 * Pause monitor @p mon from receiving notifications.
 *
 * @param mon The monitor to pause.
 */
static inline void slm_monitor_pause(struct slm_monitor_entry *mon)
{
	mon->paused = MON_PAUSED;
}

/**
 * @brief Resume monitor.
 *
 * Resume forwarding notifications to monitor @p mon.
 *
 * @param mon The monitor to resume.
 */
static inline void slm_monitor_resume(struct slm_monitor_entry *mon)
{
	mon->paused = MON_ACTIVE;
}


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* MODEM_SLM_H_ */

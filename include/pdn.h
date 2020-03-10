/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 *
 * @brief Interface for PDN connection controlling.
 *        No data can be exchanged over a PDN interface.
 *        The interface is used to manage connections to specific APNs.
 */

/**
 * @file pdn.h
 *
 * @brief Public APIs for the PDN library.
 */

#ifndef PDN_H_
#define PDN_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief PDN inet type */
enum pdn_inet_type {
	PDN_INET_DEFAULT = 0,
	PDN_INET_IPV4,
	PDN_INET_IPV6,
	PDN_INET_IPV4V6,
};

/** @brief State of the PDN */
enum pdn_state {
	/** PDN is not connected */
	PDN_DISCONNECTED = 0,
	/** PDN is inactive (connected) */
	PDN_INACTIVE,
	/** PDN is active*/
	PDN_ACTIVE
};

/** @brief Notify event types */
enum pdn_event { PDN_EVENT_NA = 0, PDN_EVENT_NW_ERROR, PDN_EVENT_DEACTIVATED };

/** @brief Data and time for PDN events */
struct pdn_time {
	u8_t year;
	u8_t mon;
	u8_t day;
	u8_t hour;
	u8_t min;
	u8_t sec;
	s8_t tz;
};

/** Invalid time zone value */
#define PDN_INVALID_TZ (99)

/**
 * @typedef pdn_notif_handler_t
 *
 * This notification handler let's application to receive notification
 * about incoming PDN events. Notifications will be dispatched to handler
 * registered using the @ref pdn_register_handler() function. Handlers
 * can be de-registered using the @ref pdn_deregister_handler() function.
 *
 * @param[in] handle    The pdn handle.
 * @param[in] event     Event id @ref enum pdn_event.
 */
typedef void (*pdn_notif_handler_t)(int handle, enum pdn_event event);

/**
 * @brief Function to connect a PDN to a remote address.
 *
 * @param[in] remote_addr  Name of the remote address to connect.
 * @param[in] family       Inet types bit mask to be used. @ref #pdn_inet_type
 *
 * @return  New handle if the procedure succeeds else negative value
 *          indicates the reason for failure.
 */
int pdn_connect(const char *const remote_addr, enum pdn_inet_type family);

/**
 * @brief Function to disconnect the PDN.
 *
 * @param[in] handle   Handle returned by @ref pdn_connect().
 *
 * @return  0 if the procedure succeeds, else negative value
 *          indicates the reason for failure.
 */
int pdn_disconnect(int handle);

/**
 * @brief Function to activate the PDN.
 *
 * @param[in] handle   Handle returned by @ref pdn_connect() or
 *                     value 0 for default context activation.
 *
 * @return  0 if the procedure succeeds, else negative value
 *          indicates the reason for failure.
 */
int pdn_activate(int handle);

/**
 * @brief Function to deactivate the PDN.
 *
 * @param[in] handle   Handle returned by @ref pdn_connect().
 *
 * @return  0 if the procedure succeeds, else negative value
 *          indicates the reason for failure.
 */
int pdn_deactivate(int handle);

/**
 * @brief Function returns current PDN inet types.
 *
 * @param[in] handle   Handle returned by @ref pdn_connect().
 *
 * @return  currently supported inet types bit mask, @ref #pdn_inet_type,
 *          else negative value indicates the reason for failure.
 */
int pdn_family_get(int handle);

/**
 * @brief Function to get the current state.
 *
 * @param[in] handle   Handle returned by @ref pdn_connect() or
 *                     value 0 for default context activation.
 *
 * @return  Current state ( @ref #pdn_state ) if the procedure succeeds,
 *          else negative value indicates the reason for failure.
 */
int pdn_state_get(int handle);

/**
 * @brief Function to get the PDN Context ID.
 *
 * @param[in] handle   Handle returned by @ref pdn_connect().
 *
 * @return  A valid context ID if the procedure succeeds,
 *          else negative value indicates the reason for failure.
 */
int pdn_context_id_get(int handle);

/**
 * @brief Function to get the PDN ID.
 *
 * @param[in] handle   Handle returned by @ref pdn_connect().
 *
 * @return  A valid PDN ID if the procedure succeeds,
 *          else negative value indicates the reason for failure.
 */
int pdn_id_get(int handle);

/**
 * @brief Sets client indication handler.
 *
 * @param[in] handle    Handle returned by @ref pdn_connect().
 * @param[in] callback  Pointer to a received notification handler function
 *                      of type @ref pdn_notif_handler_t.
 *
 * @return  0 if the procedure succeeds, else negative value
 *          indicates the reason for failure.
 */
int pdn_register_handler(int handle, pdn_notif_handler_t callback);

/**
 * @brief Removes client indication handler
 *
 * @param[in] handle   Handle returned by @ref pdn_connect().
 *
 * @return  0 if the procedure succeeds, else negative value
 *          indicates the reason for failure.
 */
int pdn_deregister_handler(int handle);

/**
 * @brief Get last connect / disconnect time.
 *
 * @param[in]  handle  Handle returned by @ref pdn_connect().
 * @param[out] time    Pointer to a target @ref pdn_time struct where
 *                     date and time values will be written.
 *
 * @return  0 if the procedure succeeds, else negative value
 *          indicates the reason for failure.
 */
int pdn_get_time(int handle, struct pdn_time *const time);

#ifdef __cplusplus
}
#endif

#endif /* PDN_H_ */

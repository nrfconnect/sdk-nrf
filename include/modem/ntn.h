/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NTN_H__
#define NTN_H__

/**
 * @file ntn.h
 *
 * @defgroup ntn NTN (Non-Terrestrial Network) library
 * @{
 * @brief Public APIs for the NTN library.
 *
 * @note This is only supported by the following modem firmware:
 *       - mfw_nrf9151-ntn
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief NTN event types.
 */
enum ntn_evt_type {
	/**
	 * Modem location update request.
	 *
	 * This event indicates whether the modem needs location updates. When location updates are
	 * requested, there are two options to provide location updates:
	 * 1. Call the ntn_modem_location_update() function with current location from GNSS
	 *    periodically. This is usually used when location from an external GNSS is available.
	 *    The required interval for calling the function depends on the requested accuracy and
	 *    the maximum speed of the device. The accuracy of the location should remain within
	 *    200 meters at all times. If the device can move at a maximum of 100 km/h i.e.
	 *    27.8 m/s, the location should be updated at least every 6 seconds, preferably every
	 *    5 seconds. The function can also be called once a second, because location will only
	 *    be updated to the modem when it is required to maintain the requested accuracy.
	 * 2. Call the ntn_modem_location_set() function to set the location to the modem
	 *    when necessary. This sets the current location directly to the modem and the validity
	 *    time can be set by the caller. This is suitable for devices which are more or less
	 *    stationary and when constant updates are not needed. The location can be invalidated
	 *    using the ntn_modem_location_invalidate() function.
	 *
	 * The associated payload is the @c ntn_evt.modem_location member of type
	 * @ref ntn_modem_location in the event.
	 */
	NTN_EVT_MODEM_LOCATION_UPDATE = 0,
};

/**
 * @brief Location update request.
 */
struct ntn_modem_location {
	/**
	 * If true, location updates are requested by the modem. When location updates are
	 * requested, the ntn_modem_location_update() function should be called with the current
	 * location from GNSS periodically. If false, the modem does not need location updates
	 * and the function does not need to be called.
	 */
	bool updates_requested;
};

/**
 * @brief NTN event.
 */
struct ntn_evt {
	/** Event type. */
	enum ntn_evt_type type;

	/** Event data. */
	union {
		/** Payload for event @ref NTN_EVT_MODEM_LOCATION_UPDATE. */
		struct ntn_modem_location modem_location;
	};
};

/**
 * @brief Event handler prototype.
 *
 * @param[in] evt Pointer to event structure.
 */
typedef void (*ntn_evt_handler_t)(const struct ntn_evt *evt);

/**
 * @brief Register an event handler.
 *
 * @param[in] handler Event handler. Can be NULL to unregister the handler.
 */
void ntn_register_handler(ntn_evt_handler_t handler);

/**
 * @brief Provide periodic location update.
 *
 * This function is used to provide current location for NTN usage, when location updates
 * have been requested using the @ref NTN_EVT_MODEM_LOCATION_UPDATE event. The library only
 * sends location updates to the modem when necessary to maintain the requested accuracy.
 * This reduces the number of AT commands sent to the modem.
 *
 * When location updates are requested, this function should be called periodically with
 * the current GNSS location. It needs to be called often enough to maintain 200 meter
 * accuracy at all times.
 *
 * @param[in] latitude Latitude in degrees.
 * @param[in] longitude Longitude in degrees.
 * @param[in] altitude Altitude in meters.
 * @param[in] speed Speed in m/s.
 */
void ntn_modem_location_update(double latitude, double longitude, float altitude, float speed);

/**
 * @brief Set location to the modem.
 *
 * This function is used to set the location to the modem for NTN usage.
 *
 * @param[in] latitude Latitude in degrees.
 * @param[in] longitude Longitude in degrees.
 * @param[in] altitude Altitude in meters.
 * @param[in] validity Location validity in seconds. 0 means infinite validity.
 *
 * @retval 0 if the location was set successfully.
 * @retval -EFAULT if an AT command could not be sent to the modem.
 */
int ntn_modem_location_set(double latitude, double longitude, float altitude, uint32_t validity);

/**
 * @brief Invalidate location.
 *
 * This function is used to invalidate the location required for NTN usage.
 *
 * @retval 0 if the location was invalidated successfully.
 * @retval -EFAULT if an AT command could not be sent to the modem.
 */
int ntn_modem_location_invalidate(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NTN_H__ */

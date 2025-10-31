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
	 * This event indicates whether the modem needs location updates. Modem needs to know
	 * the current location when NTN is used. When location updates are requested, the
	 * ntn_modem_location_update() function must be called to set the current location to the
	 * modem. The event contains the accuracy requested by the modem. When the event is
	 * received with @c nrf_modem_location.updates_requested set to true, the
	 * ntn_modem_location_update() function should be called as soon as possible even
	 * if location has been already set to the modem, because the modem may have requested
	 * location with better accuracy.
	 *
	 * The required interval for calling the function depends on the requested accuracy and
	 * speed of the device. The accuracy of the location should remain within the requested
	 * accuracy at all times. If the device can move at a maximum of 120 km/h (33.3 m/s),
	 * the location should be updated at least every six seconds. It's the responsibility of
	 * the application to maintain the location with good enough accuracy at all times. If
	 * the modem does not have the current location with sufficient accuracy, NTN usage is
	 * not possible.
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
	/** If true, location updates are requested by the modem. */
	bool updates_requested;

	/** Accuracy (in meters) requested by the modem. */
	uint32_t accuracy;
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

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file location.h
 * @brief Public APIs for the Location library.
 * @defgroup location Location library
 * @{
 */

#ifndef LOCATION_H_
#define LOCATION_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOC_MAX_METHODS 2

enum location_method {
	LOC_METHOD_CELL_ID = 1,
	LOC_METHOD_GNSS,
};

enum location_event {
	/** Location update. */
	LOC_EVT_LOCATION,
	/** Getting location timed out. */
	LOC_EVT_TIMEOUT,
	/** TODO: Is this needed? Probably not for GNSS, but could be needed for cell ID. */
	LOC_EVT_ERROR
};

struct location_data {
	enum location_method used_method;
	double latitude;
	double longitude;
	float accuracy;
};

struct location_config {
	/** Selected positioning methods in priority order. Index 0 has the highest priority. */
	enum location_method methods[LOC_MAX_METHODS];
	/** Number of positioning methods in the method list. */
	uint8_t method_count;
};

/** @brief Event handler prototype.
 *
 * @param[in] event Event ID.
 * @param[in] location Pointer to location data. Only valid when event is LOC_EVT_LOCATION.
 */
typedef void (*location_event_handler_t)(enum location_event event,
					 const struct location_data *location);

/** @brief Initializes the library.
 *
 * @details Initializes the library and sets the event handler function.
 *
 * @param[in] event_handler Event handler function.
 *
 * @return 0 on success, or negative error code on failure.
 */
int location_init(location_event_handler_t event_handler);

/** @brief Requests the current position.
 *
 * @details Requests the current position using the given configuration. The result is delivered
 *          to the event handler function.
 *
 * @param[in] config Pointer to the used configuration.
 *
 * @return 0 on success, or negative error code on failure.
 */
int location_get(const struct location_config *config);

/** @brief Starts periodic position updates.
 *
 * @details Requests periodic position updates using the given configuration and interval. The
 *          results are delivered to the event handler function.
 *
 *          Periodic position updates are provided with the given interval until
 *          location_periodic_stop() is called.
 *
 * @param[in] config Pointer to the used configuration.
 * @param[in] interval Interval for the position updates in seconds. TODO: Range?
 *
 * @return 0 on success, or negative error code on failure.
 */
int location_periodic_start(const struct location_config *config, uint16_t interval);

/** @brief Stops periodic position updates.
 *
 * @return 0 on success, or negative error code on failure.
 */
int location_periodic_stop(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* LOCATION_H_ */

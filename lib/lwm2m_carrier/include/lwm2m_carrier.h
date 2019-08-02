/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef LWM2M_CARRIER_H__

#include <stdint.h>
#include <stddef.h>

/**@file lwm2m_carrier.h
 *
 * @defgroup lwm2m_carrier_event LWM2M carrier library events
 * @{
 */
#define LWM2M_CARRIER_EVENT_BSDLIB_INIT 1  /**< BSD library initialized. */
#define LWM2M_CARRIER_EVENT_CONNECT     2  /**< LTE link connected. */
#define LWM2M_CARRIER_EVENT_DISCONNECT  3  /**< LTE link will disconnect. */
#define LWM2M_CARRIER_EVENT_READY       4  /**< LWM2M carrier registered. */
#define LWM2M_CARRIER_EVENT_FOTA_START  5  /**< Modem update started. */
#define LWM2M_CARRIER_EVENT_REBOOT      10 /**< Application will reboot. */
/**@} */

/**
 * @defgroup lwm2m_carrier_api LWM2M carrier library API
 * @brief LWM2M carrier library API functions.
 * @{
 */
/**
 * @brief LWM2M carrier library event structure.
 */
typedef struct {
	/** Event type. */
	uint32_t type;
	/** Event data. Can be NULL, depending on event type. */
	void *data;
} lwm2m_carrier_event_t;

/**
 * @brief Structure holding LWM2M carrier library initialization parameters.
 */
typedef struct {
	/** URI of the bootstrap server, null-terminated. */
	char *bootstrap_uri;
	/** Pre-shared key that the device will use. */
	char *psk;
	/** Length of the pre-shared key. */
	size_t psk_length;
} lwm2m_carrier_config_t;

/**
 * @brief Initialize the LWM2M carrier library.
 *
 * @param[in] config Configuration parameters for the library.
 *
 * @note The library does not create a copy of the config parameters, hence the
 *       application has to make sure that the parameters provided are valid
 *       throughout the application lifetime (i. e. placed in static memory
 *       or in flash).
 *
 * @return 0 on success, negative error code on error.
 */
int lwm2m_carrier_init(const lwm2m_carrier_config_t *config);

/**
 * @brief LWM2M carrier library main function.
 *
 * This is a non-return function, intended to run on a separate thread.
 */
void lwm2m_carrier_run(void);

/**
 * @brief Timezone function that returns a timezone string
 * based on the offset to GMT.
 *
 * @note  It can be overwritten by the application.
 *        Defaults to Etc/GMT+-X format
 *
 * @param[in] tz_offset Offset as the number of minutes west of GMT.
 * @param[in] dst       Daylight saving in minutes included in the @p tz_offset.
 *
 * @return  Pointer to the null terminated timezone string.
 *          In case no valid timezone found, NULL pointer can be returned.
 */
const char *lwm2m_carrier_timezone(int32_t tz_offset, int32_t dst);

/**
 * @brief LWM2M carrier library event handler.
 *
 * This function will be called by the LWM2M carrier library whenever some event
 * significant for the application occurs.
 *
 * @note This function has to be implemented by the application.
 *
 * @param[in] event LWM2M carrier event that occurred.
 */
void lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event);
/**@} */

#endif /* LWM2M_CARRIER_H__ */

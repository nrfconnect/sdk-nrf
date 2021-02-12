/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _APP_MODULE_EVENT_H_
#define _APP_MODULE_EVENT_H_

/**
 * @brief Application module event
 * @defgroup app_module_event Application module event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Application event types submitted by Application module. */
enum app_module_event_type {
	/** Signal that the application has done necessary setup, and
	 * now started.
	 */
	APP_EVT_START,

	/** Connect to LTE network. */
	APP_EVT_LTE_CONNECT,

	/** Disconnect from LTE network. */
	APP_EVT_LTE_DISCONNECT,

	/** Signal other modules to start sampling and report the data when
	 * it's ready.
	 * The event must also contain a list with requested data types,
	 * @ref app_module_data_type.
	 */
	APP_EVT_DATA_GET,

	/** Create a list with all available sensor types in the system and
	 * distribute it as a APP_EVT_DATA_GET event.
	 */
	APP_EVT_DATA_GET_ALL,

	/** Request latest configuration from the cloud. */
	APP_EVT_CONFIG_GET,

	/** The application module has performed all procedures to prepare for
	 * a shutdown of the system.
	 */
	APP_EVT_SHUTDOWN_READY,

	/** An error has occurred in the application module. Error details are
	 * attached in the event structure.
	 */
	APP_EVT_ERROR
};

/** @brief Data types that the application module requests samples for in
 * @ref app_module_event_type APP_EVT_DATA_GET.
 */
enum app_module_data_type {
	APP_DATA_ENVIRONMENTAL,
	APP_DATA_MOVEMENT,
	APP_DATA_MODEM_STATIC,
	APP_DATA_MODEM_DYNAMIC,
	APP_DATA_BATTERY,
	APP_DATA_GNSS,

	APP_DATA_COUNT,
};

/** @brief Application module event. */
struct app_module_event {
	struct event_header header;
	enum app_module_event_type type;
	enum app_module_data_type data_list[APP_DATA_COUNT];

	union {
		int err;
	} data;

	size_t count;

	/** The time each module has to fetch data before what is available
	 * is transmitted.
	 */
	int timeout;
};

/** Register app module events as an event type with the event manager. */
EVENT_TYPE_DECLARE(app_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _APP_MODULE_EVENT_H_ */

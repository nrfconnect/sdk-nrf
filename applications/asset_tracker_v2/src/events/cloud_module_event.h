/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CLOUD_MODULE_EVENT_H_
#define _CLOUD_MODULE_EVENT_H_

/**
 * @brief Cloud module event
 * @defgroup cloud_module_event Cloud module event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>
#include <qos.h>
#if defined(CONFIG_LOCATION)
#include <modem/location.h>
#endif

#include "cloud/cloud_codec/cloud_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Event types submitted by the cloud module. */
enum cloud_module_event_type {
	/** Cloud service is connected. */
	CLOUD_EVT_CONNECTED,

	/** Cloud service is disconnected. */
	CLOUD_EVT_DISCONNECTED,

	/** Connecting to a cloud service. */
	CLOUD_EVT_CONNECTING,

	/** Connection has timed out. */
	CLOUD_EVT_CONNECTION_TIMEOUT,

	/** Connect to LTE.
	 *  This event is sent out when the modem should connect to LTE (put into normal mode) post
	 *  provisioning of server credentials.
	 *
	 *  Only used when building for LwM2M.
	 */
	CLOUD_EVT_LTE_CONNECT,

	/** Disconnect from LTE.
	 *  This event is sent out when the modem should be put into offline mode prior to
	 *  provisioning of server credentials.
	 *
	 *  Only used when building for LwM2M.
	 */
	CLOUD_EVT_LTE_DISCONNECT,

	/** User association request received from cloud. */
	CLOUD_EVT_USER_ASSOCIATION_REQUEST,

	/** User association completed. */
	CLOUD_EVT_USER_ASSOCIATED,

	/** Reboot requested from cloud. */
	CLOUD_EVT_REBOOT_REQUEST,

	/** A new device configuration has been received from cloud.
	 *  The payload associated with this event is of type @ref cloud_data_cfg (config).
	 */
	CLOUD_EVT_CONFIG_RECEIVED,

	/** Cloud location data has been received from cloud.
	 *  The payload associated with this event is of type @ref location_data.
	 */
	CLOUD_EVT_CLOUD_LOCATION_RECEIVED,

	/** Cloud location response has been received but location could not be resolved. */
	CLOUD_EVT_CLOUD_LOCATION_ERROR,

	/** Cloud location resolution is unknown. */
	CLOUD_EVT_CLOUD_LOCATION_UNKNOWN,

	/** An empty device configuration has been received from cloud. */
	CLOUD_EVT_CONFIG_EMPTY,

	/** A FOTA update has started. */
	CLOUD_EVT_FOTA_START,

	/** FOTA has been performed, a reboot of the application is needed. */
	CLOUD_EVT_FOTA_DONE,

	/** An error occurred during a FOTA update. */
	CLOUD_EVT_FOTA_ERROR,

	/** Sending data to cloud using QoS library.
	 *  The payload associated with this event is of type @ref qos_data (message).
	 *
	 *  This event is only meant for the cloud module and is used to filter QoS data through
	 *  the module's internal message queue. This event is consumed by the cloud module as soon
	 *  as it has been processed.
	 */
	CLOUD_EVT_DATA_SEND_QOS,

	/** The cloud module has performed all procedures to prepare for
	 *  a shutdown of the system. The event carries the ID (id) of the module.
	 */
	CLOUD_EVT_SHUTDOWN_READY,

	/** An irrecoverable error has occurred in the cloud module. Error details are
	 *  attached in the event structure.
	 */
	CLOUD_EVT_ERROR
};

/** @brief Structure used to acknowledge messages sent to the cloud module. */
struct cloud_module_data_ack {
	/** Pointer to data that was attempted to be sent. */
	void *ptr;
	/** Length of data that was attempted to be sent. */
	size_t len;
};

/** @brief Cloud module event. */
struct cloud_module_event {
	/** Cloud module application event header. */
	struct app_event_header header;
	/** Cloud module event type. */
	enum cloud_module_event_type type;

	union {
		/** New configuration received from the cloud service. */
		struct cloud_data_cfg config;
#if defined(CONFIG_LOCATION)
		/** Cloud location data received from the cloud service. */
		struct location_data cloud_location;
#endif
		/** Data that was attempted to be sent. Could be used
		 *  to free allocated data post transmission.
		 */
		struct cloud_module_data_ack ack;
		/** The message that should be sent to cloud. */
		struct qos_data message;
		/** Module ID, used when acknowledging shutdown requests. */
		uint32_t id;
		/** Code signifying the cause of error. */
		int err;
	} data;
};

APP_EVENT_TYPE_DECLARE(cloud_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _CLOUD_MODULE_EVENT_H_ */

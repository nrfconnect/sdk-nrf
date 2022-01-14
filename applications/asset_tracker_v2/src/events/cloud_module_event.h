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

#include <event_manager.h>
#include <event_manager_profiler_tracer.h>
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

	/** User association request received from cloud. */
	CLOUD_EVT_USER_ASSOCIATION_REQUEST,

	/** User association completed. */
	CLOUD_EVT_USER_ASSOCIATED,

	/** A new device configuration has been received from cloud.
	 *  The payload associated with this event is of type @ref cloud_data_cfg (config).
	 */
	CLOUD_EVT_CONFIG_RECEIVED,

	/** An empty device configuration has been received from cloud. */
	CLOUD_EVT_CONFIG_EMPTY,

	/** A FOTA update has started. */
	CLOUD_EVT_FOTA_START,

	/** FOTA has been performed, a reboot of the application is needed. */
	CLOUD_EVT_FOTA_DONE,

	/** An error occurred during a FOTA update. */
	CLOUD_EVT_FOTA_ERROR,

	/** Sending of data to cloud has been attempted.
	 *  The payload associated with this event is of type @ref cloud_module_data_ack (ack).
	 */
	CLOUD_EVT_DATA_ACK,

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
	/* Flag to signify if the data was sent or not. */
	bool sent;
};

/** @brief Cloud module event. */
struct cloud_module_event {
	/** Cloud module event header. */
	struct event_header header;
	/** Cloud module event type. */
	enum cloud_module_event_type type;

	union {
		/** Variable that contains a new configuration received from the cloud service. */
		struct cloud_data_cfg config;
		/** Variable that contains data that was attempted to be sent. Could be used
		 *  to free allocated data post transmission.
		 */
		struct cloud_module_data_ack ack;
		/** Module ID, used when acknowledging shutdown requests. */
		uint32_t id;
		/** Code signifying the cause of error. */
		int err;
	} data;
};

EVENT_TYPE_DECLARE(cloud_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _CLOUD_MODULE_EVENT_H_ */

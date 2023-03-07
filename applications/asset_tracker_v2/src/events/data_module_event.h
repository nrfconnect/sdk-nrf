/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DATA_MODULE_EVENT_H_
#define _DATA_MODULE_EVENT_H_

/**
 * @brief Data module event
 * @defgroup data_module_event Data module event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>
#include "cloud/cloud_codec/cloud_codec.h"

#if defined(CONFIG_LWM2M)
#include <zephyr/net/lwm2m.h>
#else
#include "cloud/cloud_codec/lwm2m/lwm2m_dummy.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Data event types submitted by Data module. */
enum data_module_event_type {
	/** All data has been received for a given sample request. */
	DATA_EVT_DATA_READY,

	/** Send newly sampled data.
	 *  The event has an associated payload of type @ref data_module_data_buffers in
	 *  the `data.buffer` member.
	 *
	 *  If a non LwM2M build is used the data is heap allocated and must be freed after use by
	 *  calling k_free() on `data.buffer.buf`.
	 */
	DATA_EVT_DATA_SEND,

	/** Send older batched data.
	 *  The event has an associated payload of type @ref data_module_data_buffers in
	 *  the `data.buffer` member.
	 *
	 *  If a non LwM2M build is used the data is heap allocated and must be freed after use by
	 *  calling k_free() on `data.buffer.buf`.
	 */
	DATA_EVT_DATA_SEND_BATCH,

	/** Send UI button data.
	 *  The event has an associated payload of type @ref data_module_data_buffers in
	 *  the `data.buffer` member.
	 *
	 *  If a non LwM2M build is used the data is heap allocated and must be freed after use by
	 *  calling k_free() on `data.buffer.buf`.
	 */
	DATA_EVT_UI_DATA_SEND,

	/** UI button data is ready to be sent. */
	DATA_EVT_UI_DATA_READY,

	/** Impact data is ready to be sent. */
	DATA_EVT_IMPACT_DATA_READY,

	/** Send impact data, similar to DATA_EVT_UI_DATA_SEND */
	DATA_EVT_IMPACT_DATA_SEND,

	/** Send cloud location data.
	 *  The event has an associated payload of type @ref data_module_data_buffers in
	 *  the `data.buffer` member.
	 *
	 *  If a non LwM2M build is used the data is heap allocated and must be freed after use by
	 *  calling k_free() on `data.buffer.buf`.
	 */
	DATA_EVT_CLOUD_LOCATION_DATA_SEND,

	/** Send A-GPS request.
	 *  The event has an associated payload of type @ref data_module_data_buffers in
	 *  the `data.buffer` member.
	 *
	 *  If a non LwM2M build is used the data is heap allocated and must be freed after use by
	 *  calling k_free() on `data.buffer.buf`.
	 */
	DATA_EVT_AGPS_REQUEST_DATA_SEND,

	/** Send the initial device configuration.
	 *  The event has an associated payload of type @ref cloud_data_cfg in
	 *  the `data.cfg` member.
	 */
	DATA_EVT_CONFIG_INIT,

	/** Send the updated device configuration.
	 *  The event has an associated payload of type @ref cloud_data_cfg in
	 *  the `data.cfg` member.
	 */
	DATA_EVT_CONFIG_READY,

	/** Acknowledge the applied device configuration to cloud.
	 *  The event has an associated payload of type @ref data_module_data_buffers in
	 *  the `data.buffer` member.
	 *
	 *  If a non LwM2M build is used the data is heap allocated and must be freed after use by
	 *  calling k_free() on `data.buffer.buf`.
	 */
	DATA_EVT_CONFIG_SEND,

	/** Get the recent device configuration from cloud. */
	DATA_EVT_CONFIG_GET,

	/** Date time has been obtained. */
	DATA_EVT_DATE_TIME_OBTAINED,

	/** The data module has performed all procedures to prepare for
	 *  a shutdown of the system. The event carries the ID (id) of the module.
	 */
	DATA_EVT_SHUTDOWN_READY,

	/** An irrecoverable error has occurred in the data module. Error details are
	 *  attached in the event structure.
	 */
	DATA_EVT_ERROR
};

/** @brief Structure that contains a pointer to encoded data. */
struct data_module_data_buffers {
	char *buf;
	size_t len;
	/** Object paths used in lwM2M. NULL terminated. */
	struct lwm2m_obj_path paths[CONFIG_CLOUD_CODEC_LWM2M_PATH_LIST_ENTRIES_MAX];
	uint8_t valid_object_paths;
};

/** @brief Data module event. */
struct data_module_event {
	/** Data module application event header. */
	struct app_event_header header;
	/** Data module event type. */
	enum data_module_event_type type;

	union {
		/** Variable that carries a pointer to data encoded by the module. */
		struct data_module_data_buffers buffer;
		/** Variable that carries the current device configuration. */
		struct cloud_data_cfg cfg;
		/** Module ID, used when acknowledging shutdown requests. */
		uint32_t id;
		/** Code signifying the cause of error. */
		int err;
	} data;
};

APP_EVENT_TYPE_DECLARE(data_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DATA_MODULE_EVENT_H_ */

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

#include "event_manager.h"
#include "cloud/cloud_codec/cloud_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Cloud event types submitted by Cloud module. */
enum cloud_module_event_type {
	CLOUD_EVT_CONNECTED,
	CLOUD_EVT_DISCONNECTED,
	CLOUD_EVT_CONNECTING,
	CLOUD_EVT_CONNECTION_TIMEOUT,
	CLOUD_EVT_CONFIG_RECEIVED,
	CLOUD_EVT_CONFIG_EMPTY,
	CLOUD_EVT_FOTA_DONE,
	CLOUD_EVT_DATA_ACK,
	CLOUD_EVT_SHUTDOWN_READY,
	CLOUD_EVT_ERROR
};

struct cloud_module_event_data {
	char *buf;
	size_t len;
};

struct cloud_module_data_ack {
	void *ptr;
	size_t len;
	/* Flag to signify if the data was sent or not. */
	bool sent;
};

/** @brief Cloud event. */
struct cloud_module_event {
	struct event_header header;
	enum cloud_module_event_type type;

	union {
		struct cloud_data_cfg config;
		struct cloud_module_data_ack ack;
		/* Module ID, used when acknowledging shutdown requests. */
		uint32_t id;
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

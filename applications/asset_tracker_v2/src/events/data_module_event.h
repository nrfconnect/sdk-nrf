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

#include "event_manager.h"
#include "cloud/cloud_codec/cloud_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Data event types submitted by Data module. */
enum data_module_event_type {
	DATA_EVT_DATA_READY,
	DATA_EVT_DATA_SEND,
	DATA_EVT_DATA_SEND_BATCH,
	DATA_EVT_UI_DATA_SEND,
	DATA_EVT_UI_DATA_READY,
	DATA_EVT_NEIGHBOR_CELLS_DATA_SEND,
	DATA_EVT_CONFIG_INIT,
	DATA_EVT_CONFIG_READY,
	DATA_EVT_CONFIG_SEND,
	DATA_EVT_CONFIG_GET,
	DATA_EVT_SHUTDOWN_READY,
	DATA_EVT_DATE_TIME_OBTAINED,
	DATA_EVT_ERROR
};

/** Struct containing pointer to array of data elements. */
struct data_module_data_buffers {
	char *buf;
	size_t len;
};

/** @brief Data event. */
struct data_module_event {
	struct event_header header;
	enum data_module_event_type type;

	union {
		struct data_module_data_buffers buffer;
		struct cloud_data_cfg cfg;
		struct cloud_data_ui ui;
		/* Module ID, used when acknowledging shutdown requests. */
		uint32_t id;
		int err;
	} data;
};

EVENT_TYPE_DECLARE(data_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DATA_MODULE_EVENT_H_ */

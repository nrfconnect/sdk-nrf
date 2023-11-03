/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _PEER_CONN_EVENT_H_
#define _PEER_CONN_EVENT_H_

/**
 * @brief Peer Connection Event
 * @defgroup peer_conn_event Peer Connection Event
 * @{
 */

#include <string.h>
#include <zephyr/toolchain.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Peer type list. */
#define PEER_ID_LIST	\
	X(USB)		\
	X(BLE)

/** Peer ID list. */
enum peer_id {
#define X(name) _CONCAT(PEER_ID_, name),
	PEER_ID_LIST
#undef X

	PEER_ID_COUNT
};

enum peer_conn_state {
	PEER_STATE_CONNECTED,
	PEER_STATE_DISCONNECTED
};

/** Peer connection event. */
struct peer_conn_event {
	struct app_event_header header;

	enum peer_id peer_id;
	uint8_t dev_idx;
	enum peer_conn_state conn_state;
	uint32_t baudrate;
};

APP_EVENT_TYPE_DECLARE(peer_conn_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _PEER_CONN_EVENT_H_ */

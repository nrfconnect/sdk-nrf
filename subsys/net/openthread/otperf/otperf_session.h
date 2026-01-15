/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef __OTPERF_SESSION_H
#define __OTPERF_SESSION_H

#include "otperf_internal.h"
#include <openthread/ip6.h>

/* Type definition */
enum state {
	STATE_NULL,	/* Session has not yet started */
	STATE_ONGOING,	/* 1st packet has been received, last packet not yet */
	STATE_COMPLETED /* Session completed, stats pkt can be sent if needed */
};

struct session {
	int id;

	/* Tuple for UDP */
	otSockAddr socket_address;

	enum state state;

	/* Stat data */
	uint32_t counter;
	uint32_t next_id;
	uint32_t outorder;
	uint32_t error;
	uint64_t length;
	int64_t start_time;
	int32_t jitter;
	int32_t last_transit_time;

	/* Stats packet*/
	struct otperf_server_hdr stat;
};

struct session *get_session(const struct otSockAddr *addr);
void otperf_reset_session_stats(struct session *session);
void otperf_session_reset(void);

#endif /* __OTPERF_SESSION_H */

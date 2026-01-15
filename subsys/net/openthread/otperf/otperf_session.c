/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "otperf_session.h"

static struct session sessions[CONFIG_OTPERF_MAX_SESSIONS];

/* Get session from a given packet */
struct session *get_session(const struct otSockAddr *addr)
{
	struct session *active = NULL;
	struct session *free = NULL;
	int i = 0;

	/* Check whether we already have an active session */
	while (!active && i < CONFIG_OTPERF_MAX_SESSIONS) {
		struct session *ptr = &sessions[i];

		if (ptr->socket_address.mPort == addr->mPort &&
		    otIp6IsAddressEqual(&ptr->socket_address.mAddress, &addr->mAddress)) {
			/* We found an active session */
			active = ptr;
			break;
		}

		if (!free && (ptr->state == STATE_NULL || ptr->state == STATE_COMPLETED)) {
			/* We found a free slot - just in case */
			free = ptr;
		}

		i++;
	}

	/* If no active session then create a new one */
	if (!active && free) {
		active = free;
		active->socket_address = *addr;
	}

	return active;
}

void otperf_reset_session_stats(struct session *session)
{
	if (!session) {
		return;
	}

	session->counter = 0U;
	session->start_time = 0U;
	session->next_id = 1U;
	session->length = 0U;
	session->outorder = 0U;
	session->error = 0U;
	session->jitter = 0;
	session->last_transit_time = 0;
}

void otperf_session_reset(void)
{
	for (int j = 0; j < CONFIG_OTPERF_MAX_SESSIONS; j++) {
		sessions[j].state = STATE_NULL;
		sessions[j].id = j;
		otperf_reset_session_stats(&(sessions[j]));
	}
}

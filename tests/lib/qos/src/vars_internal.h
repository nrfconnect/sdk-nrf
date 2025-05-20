/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/slist.h>
#include <qos.h>

struct qos_metadata {
	/* Mandatory variable used to construct a linked list. */
	sys_snode_t header;

	/* Message associated with each node in the linked list. */
	struct qos_data message;
};

extern struct ctx {
	/* Library event handler. Used in callbacks to the caller. */
	qos_evt_handler_t app_evt_handler;

	/* Internal array of pending messages. Used in combination with linked list. */
	struct qos_metadata list_internal[CONFIG_QOS_PENDING_MESSAGES_MAX];

	/* Linked list used to keep track of pending messages. */
	sys_slist_t pending_list;

	/* Variable used to prevent multiple library initializations. */
	bool initialized;

	/* Delayed work used as the backoff timer. */
	struct k_work_delayable timeout_handler_work;

	/* Internal variable used to generate message IDs. */
	uint16_t message_id_next;
} ctx;

extern void timeout_handler_work_fn(struct k_work *work);

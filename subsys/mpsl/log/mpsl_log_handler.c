/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <mpsl.h>
#include "mpsl_log_config.h"

/*
 * Reference implementation of the MPSL log handler.
 *
 * Registers the log handlers with mpsl_log_handlers_set() and prints the
 * messages from a separate thread fed by the mpsl_log_msgq message queue.
 * Messages are resolved with bsearch(), as the tables are sorted by id.
 *
 * Disable CONFIG_MPSL_LOG_HANDLER_BUILTIN to provide a custom handler.
 *
 * Assumptions:
 * - A formatted log line, including the null terminator, fits in
 *   MPSL_LOG_LINE_BUF_SIZE characters.
 * - Library message tables are sorted by id in ascending order.
 */

struct mpsl_log_msg_row {
	uint16_t id;
	uint8_t level;
	const char *fmt;
};

#if defined(CONFIG_MPSL_LOG)
#include "mpsl_log_msg.h"
#define MPSL_LOG_LIB_ID_MPSL   0U /* MPSL library id */
BUILD_ASSERT(sizeof(mpsl_log_msgs[0]) == sizeof(struct mpsl_log_msg_row));
static const char mpsl_lib_name[] = "MPSL";
#endif

#if defined(CONFIG_BT_CTLR_SDC_LOG)
#include "sdc_log_msg.h"
#define MPSL_LOG_LIB_ID_SDC    1U /* SDC library id */
BUILD_ASSERT(sizeof(sdc_log_msgs[0]) == sizeof(struct mpsl_log_msg_row));
static const char sdc_lib_name[] = "SDC";
#endif

LOG_MODULE_REGISTER(mpsl_log, CONFIG_MPSL_LOG_PRINT_LEVEL);

#define MPSL_LOG_MAX_ARGS      3U
#define MPSL_LOG_LINE_BUF_SIZE 64U

#define MPSL_LOG_LIB_ID(id)    (((id) >> 16) & 0xFFU)
#define MPSL_LOG_MSG_ID(id)    ((uint16_t)((id) & 0xFFFFU))

struct mpsl_log_entry {
	uint32_t id;
	uint32_t argv[MPSL_LOG_MAX_ARGS];
};

K_MSGQ_DEFINE(mpsl_log_msgq, sizeof(struct mpsl_log_entry), CONFIG_MPSL_LOG_FIFO_SIZE, 4);

static int mpsl_log_id_cmp(const void *key, const void *elem)
{
	uint16_t id = *(const uint16_t *)key;
	const struct mpsl_log_msg_row *entry = elem;

	if (id < entry->id) {
		return -1;
	}
	if (id > entry->id) {
		return 1;
	}
	return 0;
}

static void mpsl_log_print(const struct mpsl_log_entry *entry)
{
	const struct mpsl_log_msg_row *msg = NULL;
	char line[MPSL_LOG_LINE_BUF_SIZE];
	const char *lib_name = NULL;

	uint16_t lib_id = MPSL_LOG_LIB_ID(entry->id);
	uint16_t msg_id = MPSL_LOG_MSG_ID(entry->id);

	switch (lib_id) {
#if defined(CONFIG_MPSL_LOG)
	case MPSL_LOG_LIB_ID_MPSL:
		lib_name = mpsl_lib_name;
		msg = bsearch(&msg_id, mpsl_log_msgs, ARRAY_SIZE(mpsl_log_msgs) - 1,
			      sizeof(mpsl_log_msgs[0]), mpsl_log_id_cmp);
		break;
#endif
#if defined(CONFIG_BT_CTLR_SDC_LOG)
	case MPSL_LOG_LIB_ID_SDC:
		lib_name = sdc_lib_name;
		msg = bsearch(&msg_id, sdc_log_msgs, ARRAY_SIZE(sdc_log_msgs) - 1,
			      sizeof(sdc_log_msgs[0]), mpsl_log_id_cmp);
		break;
#endif
	default:
		return;
	}

	if (msg == NULL || msg->fmt == NULL) {
		return;
	}

	/* fmt comes from the build-time generated tables, never from runtime input. */
	(void)snprintf(line, sizeof(line), msg->fmt,
		       entry->argv[0], entry->argv[1], entry->argv[2]);

	switch (msg->level) {
	case LOG_LEVEL_ERR:
		LOG_ERR("%s: %s", lib_name, line);
		break;
	case LOG_LEVEL_WRN:
		LOG_WRN("%s: %s", lib_name, line);
		break;
	case LOG_LEVEL_DBG:
		LOG_DBG("%s: %s", lib_name, line);
		break;
	default:
		LOG_INF("%s: %s", lib_name, line);
		break;
	}
}

static atomic_t dropped_cnt;

static void mpsl_log_thread_fn(void *p1, void *p2, void *p3)
{
	struct mpsl_log_entry entry;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		if (k_msgq_get(&mpsl_log_msgq, &entry, K_FOREVER) == 0) {
			mpsl_log_print(&entry);
		}

		if (k_msgq_num_used_get(&mpsl_log_msgq) == 0) {
			atomic_val_t dropped = atomic_set(&dropped_cnt, 0);

			if (dropped) {
				/* Consider increasing CONFIG_MPSL_LOG_FIFO_SIZE if it happens */
				LOG_ERR("%ld MPSL log messages dropped", (long)dropped);
			}
		}
	}
}

K_THREAD_DEFINE(mpsl_log_thread, CONFIG_MPSL_LOG_THREAD_STACK_SIZE,
		mpsl_log_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

/*
 * Called from the MPSL-based libraries.
 * Only enqueue the message id and arguments; mpsl_log_thread prints them.
 */
static void mpsl_log_enqueue0(uint32_t id)
{
	struct mpsl_log_entry entry = { .id = id };

	if (k_msgq_put(&mpsl_log_msgq, &entry, K_NO_WAIT)) {
		atomic_inc(&dropped_cnt);
	}
}

static void mpsl_log_enqueue1(uint32_t id, uint32_t a0)
{
	struct mpsl_log_entry entry = { .id = id, .argv = { a0 } };

	if (k_msgq_put(&mpsl_log_msgq, &entry, K_NO_WAIT)) {
		atomic_inc(&dropped_cnt);
	}
}

static void mpsl_log_enqueue2(uint32_t id, uint32_t a0, uint32_t a1)
{
	struct mpsl_log_entry entry = { .id = id, .argv = { a0, a1 } };

	if (k_msgq_put(&mpsl_log_msgq, &entry, K_NO_WAIT)) {
		atomic_inc(&dropped_cnt);
	}
}

static void mpsl_log_enqueue3(uint32_t id, uint32_t a0, uint32_t a1, uint32_t a2)
{
	struct mpsl_log_entry entry = { .id = id, .argv = { a0, a1, a2 } };

	if (k_msgq_put(&mpsl_log_msgq, &entry, K_NO_WAIT)) {
		atomic_inc(&dropped_cnt);
	}
}

static const mpsl_log_handlers_t mpsl_log_handlers = {
	.log0 = mpsl_log_enqueue0,
	.log1 = mpsl_log_enqueue1,
	.log2 = mpsl_log_enqueue2,
	.log3 = mpsl_log_enqueue3,
};

static int mpsl_log_register(void)
{
	return mpsl_log_handlers_set(&mpsl_log_handlers);
}

SYS_INIT(mpsl_log_register, PRE_KERNEL_1, 0);

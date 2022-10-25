/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <string.h>
#include <zephyr/logging/log.h>

#include <bluetooth/services/cgms.h>

#include "cgms_internal.h"

LOG_MODULE_DECLARE(cgms, CONFIG_BT_CGMS_LOG_LEVEL);

#define CGMS_RACP_LENGTH 20

#define RACP_Q_STACK_SIZE 2048
#define RACP_Q_PRIORITY 1
K_THREAD_STACK_DEFINE(racp_q_stack_area, RACP_Q_STACK_SIZE);

/* The number of measurement that can be stored */
#define RECORD_NUM       CONFIG_BT_CGMS_MAX_MEASUREMENT_RECORD

/**@brief Record Access Control Point opcodes. */
enum racp_opcode {
	RACP_OPCODE_RESERVED = 0,
	RACP_OPCODE_REPORT_RECS = 1,
	RACP_OPCODE_DELETE_RECS = 2,
	RACP_OPCODE_ABORT_OPERATION = 3,
	RACP_OPCODE_REPORT_NUM_RECS = 4,
	RACP_OPCODE_NUM_RECS_RESPONSE = 5,
	RACP_OPCODE_RESPONSE_CODE = 6,
};

/**@brief Record Access Control Point operators. */
enum racp_operator {
	RACP_OPERATOR_NULL = 0,
	RACP_OPERATOR_ALL = 1,
	RACP_OPERATOR_LESS_OR_EQUAL = 2,
	RACP_OPERATOR_GREATER_OR_EQUAL = 3,
	RACP_OPERATOR_RANGE = 4,
	RACP_OPERATOR_FIRST = 5,
	RACP_OPERATOR_LAST = 6,
	RACP_OPERATOR_RFU_START = 7,
};

/**@brief Record Access Control Point Operand Filter Type Value. */
enum racp_operand_filter {
	RACP_OPERAND_FILTER_TYPE_TIME_OFFSET = 1,
	RACP_OPERAND_FILTER_TYPE_FACING_TIME = 2,
};

/**@brief Record Access Control Point response codes. */
enum racp_rsp_code {
	RACP_RESPONSE_RESERVED = 0,
	RACP_RESPONSE_SUCCESS = 1,
	RACP_RESPONSE_OPCODE_UNSUPPORTED = 2,
	RACP_RESPONSE_INVALID_OPERATOR = 3,
	RACP_RESPONSE_OPERATOR_UNSUPPORTED = 4,
	RACP_RESPONSE_INVALID_OPERAND = 5,
	RACP_RESPONSE_NO_RECORDS_FOUND = 6,
	RACP_RESPONSE_ABORT_FAILED = 7,
	RACP_RESPONSE_PROCEDURE_NOT_DONE = 8,
	RACP_RESPONSE_OPERAND_UNSUPPORTED = 9,
};

/* structure of a record in db  */
struct cgms_meas_db_entry {
	sys_snode_t node;
	bool used;
	struct cgms_meas meas;
};

/** structure of racp task */
struct racp_task {
	struct k_work item;
	struct bt_conn *peer;
	struct net_buf_simple req;
	uint8_t req_buf[CGMS_RACP_LENGTH];
};

static struct cgms_meas_db_entry cgms_meas_db_entry_pool[RECORD_NUM];

static struct k_mutex lock;

static sys_slist_t database;

static struct k_work_q racp_work_q;

static struct racp_task report_record_task;

static struct cgms_meas_db_entry *entry_alloc(void)
{
	for (int i = 0; i < ARRAY_SIZE(cgms_meas_db_entry_pool); i++) {
		if (!cgms_meas_db_entry_pool[i].used) {
			cgms_meas_db_entry_pool[i].used = true;
			return &cgms_meas_db_entry_pool[i];
		}
	}
	return NULL;
}

static int generic_handler(struct bt_conn *peer, uint8_t opcode, uint8_t response_code)
{
	NET_BUF_SIMPLE_DEFINE(rsp, CGMS_RACP_LENGTH);

	net_buf_simple_add_u8(&rsp, RACP_OPCODE_RESPONSE_CODE);
	net_buf_simple_add_u8(&rsp, RACP_OPERATOR_NULL);
	net_buf_simple_add_u8(&rsp, opcode);
	net_buf_simple_add_u8(&rsp, response_code);

	return cgms_racp_send_response(peer, &rsp);
}

static int report_recs_all_handler(struct bt_conn *peer)
{
	int rc;
	struct cgms_meas_db_entry *entry;
	sys_snode_t *record_node;

	/* Check if database is empty */
	if (sys_slist_is_empty(&database)) {
		return generic_handler(peer, RACP_OPCODE_REPORT_RECS,
				RACP_RESPONSE_NO_RECORDS_FOUND);
	}

	SYS_SLIST_FOR_EACH_NODE(&database, record_node) {
		entry = CONTAINER_OF(record_node, struct cgms_meas_db_entry, node);
		rc = cgms_racp_send_record(peer, &entry->meas);
		if (rc != 0) {
			LOG_WRN("Error occurs when transmitting record: %d", rc);
			return generic_handler(peer, RACP_OPCODE_REPORT_RECS,
					RACP_RESPONSE_PROCEDURE_NOT_DONE);
		}

	}

	return generic_handler(peer, RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_SUCCESS);
}

static int report_recs_greater_or_equal_handler(struct bt_conn *peer,
					struct net_buf_simple *operand)
{
	int rc;
	enum racp_operand_filter filter;
	uint16_t time_offset_limit;
	struct cgms_meas_db_entry *entry;
	sys_snode_t *record_node;

	/* In this case, the length of operand is at least 3 bytes,
	 * 1 for filter type, another 2 for filter value.
	 */
	if (operand->len < 3) {
		return generic_handler(peer, RACP_OPCODE_REPORT_RECS,
				RACP_RESPONSE_INVALID_OPERAND);
	}

	filter = net_buf_simple_pull_u8(operand);
	if (filter != RACP_OPERAND_FILTER_TYPE_TIME_OFFSET) {
		return generic_handler(peer, RACP_OPCODE_REPORT_RECS,
				RACP_RESPONSE_OPERAND_UNSUPPORTED);
	}

	time_offset_limit = net_buf_simple_pull_le16(operand);

	/* Check if database is empty */
	if (sys_slist_is_empty(&database)) {
		return generic_handler(peer, RACP_OPCODE_REPORT_RECS,
				RACP_RESPONSE_NO_RECORDS_FOUND);
	}

	entry = CONTAINER_OF(sys_slist_peek_tail(&database), struct cgms_meas_db_entry, node);
	/* If the latest one has a smaller time offset than threshold, no record is found */
	if (entry->meas.time_offset < time_offset_limit) {
		return generic_handler(peer, RACP_OPCODE_REPORT_RECS,
				RACP_RESPONSE_NO_RECORDS_FOUND);
	}

	SYS_SLIST_FOR_EACH_NODE(&database, record_node) {
		entry = CONTAINER_OF(record_node, struct cgms_meas_db_entry, node);
		if (entry->meas.time_offset >= time_offset_limit) {
			rc = cgms_racp_send_record(peer, &entry->meas);
			if (rc != 0) {
				LOG_WRN("Error occurs when transmitting record: %d", rc);
				return generic_handler(peer, RACP_OPCODE_REPORT_RECS,
						RACP_RESPONSE_PROCEDURE_NOT_DONE);
			}
		}
	}

	return generic_handler(peer, RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_SUCCESS);
}

static int report_recs_first_handler(struct bt_conn *peer)
{
	int rc;
	struct cgms_meas_db_entry *entry;

	if (sys_slist_is_empty(&database)) {
		return generic_handler(peer, RACP_OPCODE_REPORT_RECS,
				RACP_RESPONSE_NO_RECORDS_FOUND);
	}

	entry = CONTAINER_OF(sys_slist_peek_head(&database), struct cgms_meas_db_entry, node);
	rc = cgms_racp_send_record(peer, &entry->meas);
	if (rc != 0) {
		LOG_WRN("Error occurs when transmitting record: %d", rc);
		return generic_handler(peer, RACP_OPCODE_REPORT_RECS,
				RACP_RESPONSE_PROCEDURE_NOT_DONE);
	}

	return generic_handler(peer, RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_SUCCESS);
}

static int report_recs_last_handler(struct bt_conn *peer)
{
	int rc;
	struct cgms_meas_db_entry *entry;

	if (sys_slist_is_empty(&database)) {
		return generic_handler(peer, RACP_OPCODE_REPORT_RECS,
				RACP_RESPONSE_NO_RECORDS_FOUND);
	}

	entry = CONTAINER_OF(sys_slist_peek_tail(&database), struct cgms_meas_db_entry, node);
	rc = cgms_racp_send_record(peer, &entry->meas);
	if (rc != 0) {
		LOG_WRN("Error occurs when transmitting record: %d", rc);
		return generic_handler(peer, RACP_OPCODE_REPORT_RECS,
				RACP_RESPONSE_PROCEDURE_NOT_DONE);
	}

	return generic_handler(peer, RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_SUCCESS);
}

static int report_recs_handler(struct bt_conn *peer, struct net_buf_simple *operators)
{
	int rc = 0;
	enum racp_operator operator;

	if (operators->len == 0) {
		return -ENODATA;
	}

	operator = net_buf_simple_pull_u8(operators);
	switch (operator) {
	case RACP_OPERATOR_ALL:
		rc = report_recs_all_handler(peer);
		break;
	case RACP_OPERATOR_GREATER_OR_EQUAL:
		rc = report_recs_greater_or_equal_handler(peer, operators);
		break;
	case RACP_OPERATOR_FIRST:
		rc = report_recs_first_handler(peer);
		break;
	case RACP_OPERATOR_LAST:
		rc = report_recs_last_handler(peer);
		break;
	default:
		rc = generic_handler(peer, RACP_OPCODE_REPORT_RECS,
				RACP_RESPONSE_OPERATOR_UNSUPPORTED);
		break;
	}

	return rc;
}

static int report_num_recs_all_handler(struct bt_conn *peer)
{
	uint16_t count = 0;
	sys_snode_t *record_node;

	NET_BUF_SIMPLE_DEFINE(rsp, CGMS_RACP_LENGTH);

	if (!sys_slist_is_empty(&database)) {
		SYS_SLIST_FOR_EACH_NODE(&database, record_node) {
			count++;
		}
	}

	net_buf_simple_add_u8(&rsp, RACP_OPCODE_NUM_RECS_RESPONSE);
	net_buf_simple_add_u8(&rsp, RACP_OPERATOR_NULL);
	net_buf_simple_add_le16(&rsp, count);

	return cgms_racp_send_response(peer, &rsp);
}

static int report_num_recs_greater_or_equal_handler(struct bt_conn *peer,
				struct net_buf_simple *operand)
{
	enum racp_operand_filter filter;
	uint16_t time_offset_limit;
	uint16_t count = 0;
	struct cgms_meas_db_entry *entry;
	sys_snode_t *record_node;

	NET_BUF_SIMPLE_DEFINE(rsp, CGMS_RACP_LENGTH);

	if (operand->len < 3) {
		return generic_handler(peer, RACP_OPCODE_REPORT_RECS,
				RACP_RESPONSE_INVALID_OPERAND);
	}

	filter = net_buf_simple_pull_u8(operand);
	if (filter != RACP_OPERAND_FILTER_TYPE_TIME_OFFSET) {
		return generic_handler(peer, RACP_OPCODE_REPORT_RECS,
			RACP_RESPONSE_OPERAND_UNSUPPORTED);
	}

	time_offset_limit = net_buf_simple_pull_le16(operand);

	if (!sys_slist_is_empty(&database)) {
		SYS_SLIST_FOR_EACH_NODE(&database, record_node) {
			entry = CONTAINER_OF(record_node, struct cgms_meas_db_entry, node);
			if (entry->meas.time_offset >= time_offset_limit) {
				count++;
			}
		}
	}

	net_buf_simple_add_u8(&rsp, RACP_OPCODE_NUM_RECS_RESPONSE);
	net_buf_simple_add_u8(&rsp, RACP_OPERATOR_NULL);
	net_buf_simple_add_le16(&rsp, count);

	return cgms_racp_send_response(peer, &rsp);
}

static int report_num_recs_handler(struct bt_conn *peer, struct net_buf_simple *operators)
{
	enum racp_operator operator;

	if (operators->len < 1) {
		return generic_handler(peer, RACP_OPCODE_REPORT_NUM_RECS,
			RACP_RESPONSE_INVALID_OPERATOR);
	}

	operator = net_buf_simple_pull_u8(operators);
	if (operator == RACP_OPERATOR_ALL) {
		return report_num_recs_all_handler(peer);
	}
	if (operator == RACP_OPERATOR_GREATER_OR_EQUAL) {
		return report_num_recs_greater_or_equal_handler(peer, operators);
	}

	return generic_handler(peer, RACP_OPCODE_REPORT_NUM_RECS,
			RACP_RESPONSE_OPERATOR_UNSUPPORTED);
}

static void racp_task_handler(struct k_work *work_item)
{
	int rc;
	struct racp_task *task;
	enum racp_opcode opcode;

	task = CONTAINER_OF(work_item, struct racp_task, item);
	opcode = net_buf_simple_pull_u8(&task->req);

	if (k_mutex_lock(&lock, K_NO_WAIT) == 0) {
		switch (opcode) {
		case RACP_OPCODE_REPORT_RECS:
			rc = report_recs_handler(task->peer,
				&task->req);
			break;
		case RACP_OPCODE_REPORT_NUM_RECS:
			rc = report_num_recs_handler(task->peer,
				&task->req);
			break;
		default:
			rc = generic_handler(task->peer, opcode,
					RACP_RESPONSE_OPCODE_UNSUPPORTED);
			break;
		}
		k_mutex_unlock(&lock);
	} else {
		rc = -EBUSY;
	}
}

static int abort_handler(struct bt_conn *peer, struct net_buf_simple *operators)
{
	int rc;
	enum racp_operator operator;

	/* Check if operator exists */
	if (operators->len < 1) {
		LOG_INF("RACP: abortion failed due to missing operator");
		return generic_handler(peer, RACP_OPCODE_ABORT_OPERATION,
			RACP_RESPONSE_INVALID_OPERATOR);
	}
	/* Check if operator is RACP_OPERATOR_NULL */
	operator = net_buf_simple_pull_u8(operators);
	if (operator != RACP_OPERATOR_NULL) {
		LOG_INF("RACP: abortion failed due to invalid operator");
		return generic_handler(peer, RACP_OPCODE_ABORT_OPERATION,
			RACP_RESPONSE_INVALID_OPERATOR);
	}

	rc = k_work_cancel(&report_record_task.item);
	if (rc == 0) {
		LOG_INF("RACP: work aborted");
		rc = generic_handler(peer, RACP_OPCODE_ABORT_OPERATION,
		RACP_RESPONSE_SUCCESS);
	} else {
		LOG_WRN("RACP: cannot abort work");
		rc = generic_handler(peer, RACP_OPCODE_ABORT_OPERATION,
		RACP_RESPONSE_ABORT_FAILED);
	}
	return rc;
}

int cgms_racp_recv_request(struct bt_conn *peer, const uint8_t *req_data, uint16_t req_len)
{
	int rc;
	enum racp_opcode opcode;
	struct net_buf_simple req;

	/* Early check of the data length. */
	if (!req_data || req_len == 0) {
		LOG_INF("RACP: Empty command");
		return -ENODATA;
	}

	net_buf_simple_init_with_data(&req, (void *)req_data, req_len);

	/* All tasks are executed in system workqueue except abortion.
	 * So we need to handler abortion request here.
	 */
	opcode = net_buf_simple_pull_u8(&req);
	if (opcode == RACP_OPCODE_ABORT_OPERATION) {
		return abort_handler(peer, &req);
	}

	/* For other requests, prepare the data and submit to workqueue. */
	report_record_task.peer = peer;
	memcpy(report_record_task.req_buf, req_data, req_len);
	net_buf_simple_init_with_data(&report_record_task.req, report_record_task.req_buf, req_len);
	rc = k_work_submit_to_queue(&racp_work_q, &report_record_task.item);
	LOG_INF("RACP: work submission %s", rc > 0 ? "done" : "failed");
	return rc;
}

int cgms_racp_meas_add(struct cgms_meas meas)
{
	int rc;
	struct cgms_meas_db_entry *entry;

	if (k_mutex_lock(&lock, K_NO_WAIT) == 0) {
		entry = entry_alloc();
		if (entry == NULL) {
			entry = CONTAINER_OF(sys_slist_get(&database),
					struct cgms_meas_db_entry, node);
		}
		memcpy(&entry->meas, &meas, sizeof(meas));
		sys_slist_append(&database, &entry->node);
		k_mutex_unlock(&lock);
		rc = 0;
	} else {
		rc = -EBUSY;
	}

	return rc;
}

int cgms_racp_meas_get_latest(struct cgms_meas *meas)
{
	int rc;
	struct cgms_meas_db_entry *entry;

	if (k_mutex_lock(&lock, K_NO_WAIT) == 0) {
		if (!sys_slist_is_empty(&database)) {
			entry = CONTAINER_OF(sys_slist_peek_tail(&database),
				struct cgms_meas_db_entry, node);
			memcpy(meas, &entry->meas, sizeof(struct cgms_meas));
			rc = 0;
		} else {
			rc = -ENODATA;
		}
		k_mutex_unlock(&lock);
	} else {
		rc = -EBUSY;
	}

	return rc;
}

void cgms_racp_init(void)
{
	memset(&cgms_meas_db_entry_pool, 0, sizeof(cgms_meas_db_entry_pool));

	sys_slist_init(&database);

	k_mutex_init(&lock);

	k_work_queue_init(&racp_work_q);
	k_work_queue_start(&racp_work_q, racp_q_stack_area,
			K_THREAD_STACK_SIZEOF(racp_q_stack_area), RACP_Q_PRIORITY, NULL);
	k_work_init(&report_record_task.item, racp_task_handler);
}

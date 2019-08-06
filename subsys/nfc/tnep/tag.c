/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <nfc/tnep/tag.h>
#include <stdbool.h>
#include <nfc/ndef/nfc_ndef_msg.h>
#include <nfc/ndef/msg_parser.h>
#include "protocol_timer.h"
#include <logging/log.h>


#define TNEP_MASKED_VALUE(x, mask) ((x > mask)?mask:(x & mask))

#define NFC_TNEP_MAX_EXEC_MASK (0x0fU)
#define NFC_TNEP_MAX_EXEC_NO(N_wait) TNEP_MASKED_VALUE(N_wait,\
					NFC_TNEP_MAX_EXEC_MASK)
/*TODO:
 * Minimum waiting time.
 * WT_INT contains the 6 least significant bits
 * of the minimum waiting time field.
 * t_wait = 2^(WT_INIT/4 - 1) [ms].
 * WT_INT value between 0 and 63.
 */
#define NFC_TNEP_WT_INIT_MASK (0x3f)
#define NFC_TNEP_WT_INT(wt_init) TNEP_MASKED_VALUE(wt_init,\
						  NFC_TNEP_WT_INIT_MASK)
#define NFC_TNEP_MIN_WAIT_TIME(x) ((x > 4)?(1<<(NFC_TNEP_WT_INT(x)/4 - 1)):1U)
LOG_MODULE_REGISTER(nfc_tnep_tag);

enum tnep_signal_msg_rx {
	TNEP_SIGNAL_MSG_RX_DUMMY,
	TNEP_SIGNAL_MSG_RX_INIT,
	TNEP_SIGNAL_MSG_RX_NEW,
};

enum tnep_signal_msg_tx {
	TNEP_SIGNAL_MSG_TX_DUMMY,
	TNEP_SIGNAL_MSG_TX_NEW,
	TNEP_SIGNAL_MSG_TX_FINISH
};

enum tnep_event {
	TNEP_EVENT_DUMMY,
	TNEP_EVENT_MSG_RX_OFFSET,
	TNEP_EVENT_MSG_INIT,
	TNEP_EVENT_MSG_RX_NEW,
	TNEP_EVENT_MSG_TX_OFFSET,
	TNEP_EVENT_MSG_TX_NEW,
	TNEP_EVENT_MSG_FINISH,
	TNEP_EVENT_TIME_OFFSET,
	TNEP_EVENT_TIME_T_WAIT,
	TNEP_EVENT_TIME_N_WAIT,
};

enum tnep_state_name {
	TNEP_STATE_DISABLED,
	TNEP_STATE_SERVICE_READY,
	TNEP_STATE_SERVICE_SELECTED,
};

static enum tnep_state_name current_state = TNEP_STATE_DISABLED;

enum tnep_svc_record_status {
	TNEP_SVC_NOT_FOUND, TNEP_SVC_SELECTED, TNEP_SVC_DESELECTED,
};

static struct nfc_tnep_service *tnep_svcs;
static size_t tnep_svcs_amount;
static volatile struct nfc_tnep_service *active_svc;

NFC_NDEF_MSG_DEF(tnep_tx_msg, NFC_TNEP_MSG_MAX_RECORDS);

static size_t nfc_tnep_tx_buffer_len;
static u8_t *nfc_tnep_tx_buffer;

static size_t nfc_tnep_rx_buffer_len;
static u8_t *nfc_tnep_rx_buffer;
K_MUTEX_DEFINE(tnep_tx_msg_mutex);

struct k_poll_event nfc_tnep_events[NFC_TNEP_EVENTS_NUMBER];
static int nfc_tnep_event_offset[NFC_TNEP_EVENTS_NUMBER];
static struct k_poll_signal nfc_tnep_msg_rx;
static struct k_poll_signal nfc_tnep_msg_tx;

struct tnep_state {
	enum tnep_state_name name;
	void (*process)(enum tnep_event);
};

NFC_NDEF_RECORD_BIN_DATA_DEF(empty, TNF_EMPTY, NULL, 0, NULL, 0, NULL, 0);

NFC_TNEP_STATUS_RECORD_DESC_DEF(status_record, NFC_TNEP_STATUS_PROTOCOL_ERROR);

static int tnep_tx_msg_clear(void)
{
	int err = 0;

	if (k_mutex_lock(&tnep_tx_msg_mutex,
			 K_MSEC(NFC_TNEP_MSG_ADD_REC_TIMEOUT)) == 0) {

		nfc_ndef_msg_clear(&NFC_NDEF_MSG(tnep_tx_msg));

		k_mutex_unlock(&tnep_tx_msg_mutex);
	} else {
		err = -ETIMEDOUT;
	}

	return err;
}

static int tnep_tx_msg_encode(void)
{
	int err = 0;

	size_t len = NFC_TNEP_MSG_MAX_SIZE;

	if (k_mutex_lock(&tnep_tx_msg_mutex,
			 K_MSEC(NFC_TNEP_MSG_ADD_REC_TIMEOUT)) == 0) {

		/* Calculate message length */
		err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(tnep_tx_msg), NULL,
					  &len);

		if (err) {
			k_mutex_unlock(&tnep_tx_msg_mutex);

			return err;
		}

		/* Encode NDEF message */
		u8_t new_msg_buf[len];

		err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(tnep_tx_msg),
					  new_msg_buf, &len);

		k_mutex_unlock(&tnep_tx_msg_mutex);

		if (err) {
			LOG_DBG("Can't encode NDEF message to buffer");
			return err;
		}

		/* Write encoded NDEF message to NFC buffer */
		if (len > nfc_tnep_tx_buffer_len) {
			LOG_DBG("Response NDEF message is too long");
			return -ENOMEM;
		}

		unsigned int key = irq_lock();

		memset(nfc_tnep_tx_buffer, 0x00, nfc_tnep_tx_buffer_len);

		memcpy(nfc_tnep_tx_buffer, new_msg_buf, len);

		irq_unlock(key);

	} else {
		err = -ETIMEDOUT;
	}

	return err;
}

static int tnep_tx_msg_add_rec(struct nfc_ndef_record_desc *record)
{
	int err = 0;

	if (!record) {
		return -EINVAL;
	}

	if (k_mutex_lock(&tnep_tx_msg_mutex,
			 K_MSEC(NFC_TNEP_MSG_ADD_REC_TIMEOUT)) == 0) {

		err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(tnep_tx_msg),
					      record);

		k_mutex_unlock(&tnep_tx_msg_mutex);
	} else {
		err = -ETIMEDOUT;
	}

	return err;
}

static int tnep_tx_msg_add_rec_status(enum nfc_tnep_status_value status)
{
	int err = 0;

	if (status == NFC_TNEP_STATUS_PROTOCOL_ERROR) {
		err = tnep_tx_msg_clear();

		if (err) {
			LOG_DBG("Can't clear before write Error Status");
			return err;
		}
	}

	status_record.status_type = status;

	err = tnep_tx_msg_add_rec(&NFC_NDEF_TNEP_RECORD_DESC(status_record));

	return err;
}

static int tnep_tx_msg_add_rec_services(void)
{
	int err = 0;

	for (size_t i = 0; (i < tnep_svcs_amount) && (!err); i++) {
		err = tnep_tx_msg_add_rec(tnep_svcs[i].ndef_record);
	}

	return err;
}

static bool ndef_check_rec_type(const struct nfc_ndef_record_desc *record,
				const u8_t *type_field,
				const u32_t type_field_length)
{
	if (record->type_length != type_field_length) {
		return false;
	}

	int cmp_res = memcmp(record->type, type_field, type_field_length);

	return (cmp_res == 0);
}

static int tnep_svc_active_svc_config(void)
{
	int err = 0;

	/* Call Service's callback*/
	u8_t result = active_svc->callbacks->selected();

	/* Prepare TNEP Status Message */
	if ((result >= NFC_TNEP_STATUS_SERVICE_ERROR_BEGIN)
			&& (result <= NFC_TNEP_STATUS_SERVICE_ERROR_END)) {
		err = tnep_tx_msg_add_rec_status(result);
	} else {
		err = tnep_tx_msg_add_rec_status(NFC_TNEP_STATUS_SUCCESS);
	}

	/* Restart timer with new parameters*/
	u8_t n = NFC_TNEP_MAX_EXEC_NO(
			active_svc->parameters->max_waiting_time_ext);
	size_t t_wait = NFC_TNEP_MIN_WAIT_TIME(
			active_svc->parameters->min_waiting_time);

	nfc_tnep_timer_stop();

	nfc_tnep_timer_init(t_wait, n);

	return err;
}

static int tnep_svc_set_active(u8_t *uri_name, size_t uri_length)
{

	for (size_t i = 0; i < tnep_svcs_amount; i++) {
		if (tnep_svcs[i].parameters->svc_name_uri_length
				== uri_length) {

			bool equals = !memcmp(
					uri_name,
					tnep_svcs[i].parameters->svc_name_uri,
					uri_length);

			if (equals) {
				active_svc = &tnep_svcs[i];

				return TNEP_SVC_SELECTED;
			}
		}
	}

	active_svc = NULL;

	return TNEP_SVC_DESELECTED;
}

static int tnep_svc_select_from_msg(void)
{
	int err;

	enum tnep_svc_record_status svc_status = TNEP_SVC_NOT_FOUND;

	/* Parse rx buffer to NDEF message */
	u8_t desc_buf[NFC_NDEF_PARSER_REQIRED_MEMO_SIZE_CALC(
			NFC_TNEP_MSG_MAX_RECORDS)];
	size_t desc_buf_len = sizeof(desc_buf);

	err = nfc_ndef_msg_parse(desc_buf, &desc_buf_len, nfc_tnep_rx_buffer,
				 &nfc_tnep_rx_buffer_len);
	if (err) {
		return -EINVAL;
	}

	const struct nfc_ndef_msg_desc *msg_p;

	msg_p = (struct nfc_ndef_msg_desc *) desc_buf;

	/* Search for service records */
	for (size_t i = 0; i < (msg_p->record_count); i++) {
		bool is_service_select_rec = ndef_check_rec_type(
				msg_p->record[i],
				nfc_ndef_tnep_rec_type_svc_select,
				NFC_NDEF_TNEP_REC_TYPE_LEN);

		if (is_service_select_rec) {
			/* Set service from record as active (or deselect) */
			u8_t bin_rec[NFC_TNEP_RECORD_MAX_SZIE];

			u32_t bin_rec_len = sizeof(bin_rec);

			memset(bin_rec, 0x00, bin_rec_len);

			msg_p->record[i]->payload_constructor(
					msg_p->record[i]->payload_descriptor,
					bin_rec, &bin_rec_len);

			struct nfc_ndef_tnep_svc_select service = {
					.uri_name_len = bin_rec[0],
					.uri_name = &bin_rec[1] };

			svc_status = tnep_svc_set_active(service.uri_name,
							 service.uri_name_len);
		}
	}

	return svc_status;
}

static void tnep_error_check(int error_code)
{
	if (!error_code) {
		return;
	}

	LOG_DBG("TNEP error: %d", error_code);

	if (active_svc) {
		active_svc->callbacks->error_detected(error_code);
	}
}

static void tnep_sm_disabled(enum tnep_event event)
{
	LOG_DBG("TNEP Disabled!");
}

static void tnep_sm_service_ready(enum tnep_event event)
{
	LOG_DBG("TNEP Service Ready");

	int err = 0;

	switch (event) {
	case TNEP_EVENT_MSG_RX_NEW: {
		err = tnep_tx_msg_clear();

		if (err) {
			LOG_DBG("Clear message error %d", err);

			break;
		}

		enum tnep_svc_record_status status;

		status = tnep_svc_select_from_msg();

		switch (status) {
		case TNEP_SVC_SELECTED:
			err = tnep_svc_active_svc_config();

			current_state = TNEP_STATE_SERVICE_SELECTED;
			break;
		case TNEP_SVC_NOT_FOUND:
		case TNEP_SVC_DESELECTED:
			LOG_DBG("Already in Service Ready state");

			err = tnep_tx_msg_add_rec_services();

			break;
		default:
			LOG_DBG("Incorrect service select status %d", status);

			err = -ENOTSUP;

			break;
		}

		break;
	}
	case TNEP_EVENT_MSG_INIT:
		LOG_DBG("Initial NDEF Message preparation");

		break;
	case TNEP_EVENT_MSG_TX_NEW:
	case TNEP_EVENT_MSG_FINISH:
	case TNEP_EVENT_TIME_T_WAIT:
	case TNEP_EVENT_TIME_N_WAIT:
		LOG_DBG("Operation not enable in Service Ready.");

		break;
	default:
		LOG_DBG("Unknown event in Service Ready %d", event);

		err = -ENOTSUP;

		break;
	}

	tnep_error_check(err);
}

static void tnep_sm_service_selected(enum tnep_event event)
{
	LOG_DBG("TNEP Service Selected");

	int err = 0;

	if (!active_svc) {
		LOG_DBG("No active service in service selected state!");

		return;
	}

	volatile struct nfc_tnep_service *last_service = active_svc;

	switch (event) {
	case TNEP_EVENT_MSG_RX_NEW: {
		err = tnep_tx_msg_clear();

		if (err) {
			LOG_DBG("Clear message error %d", err);

			break;
		}

		enum tnep_svc_record_status status;

		status = tnep_svc_select_from_msg();

		switch (status) {
		case TNEP_SVC_NOT_FOUND:
			active_svc->callbacks->message_received();

			break;
		case TNEP_SVC_SELECTED:
			last_service->callbacks->deselected();

			err = tnep_svc_active_svc_config();

			break;
		case TNEP_SVC_DESELECTED:
			err = tnep_tx_msg_add_rec_services();

			nfc_tnep_timer_stop();

			last_service->callbacks->deselected();

			current_state = TNEP_STATE_SERVICE_READY;

			break;
		default:
			LOG_DBG("Incorrect service select status %d", status);

			err = -ENOTSUP;

			break;
		}
		break;
	}
	case TNEP_EVENT_MSG_TX_NEW:
		LOG_DBG("New Application Data");

		break;
	case TNEP_EVENT_MSG_FINISH:
		err = tnep_tx_msg_clear();

		if (err) {
			LOG_DBG("Clear message error %d", err);

			break;
		}

		err = tnep_tx_msg_add_rec_status(NFC_TNEP_STATUS_SUCCESS);

		break;
	case TNEP_EVENT_TIME_T_WAIT:
		LOG_DBG("period timeout");

		break;
	case TNEP_EVENT_TIME_N_WAIT:
		LOG_DBG("communication timeout!");

		last_service->callbacks->rx_timeout();

		break;
	default:
		LOG_DBG("unknown signal %d", event);
		break;
	}

	tnep_error_check(err);
}

const struct tnep_state tnep_state_machine[] = {
		{
				TNEP_STATE_DISABLED,
				tnep_sm_disabled },
		{
				TNEP_STATE_SERVICE_READY,
				tnep_sm_service_ready },
		{
				TNEP_STATE_SERVICE_SELECTED,
				tnep_sm_service_selected }, };

int nfc_tnep_tx_msg_buffer_register(u8_t *tx_buffer, size_t len)
{
	if (!tx_buffer || !len) {
		LOG_DBG("Invalid buffer");
		return -EINVAL;
	}

	nfc_tnep_tx_buffer_len = len;
	nfc_tnep_tx_buffer = tx_buffer;

	return 0;
}

void nfc_tnep_rx_msg_indicate(u8_t *rx_buffer, size_t len)
{
	k_poll_signal_raise(&nfc_tnep_msg_rx, TNEP_SIGNAL_MSG_RX_NEW);

	nfc_tnep_rx_buffer_len = len;
	nfc_tnep_rx_buffer = rx_buffer;

	if (active_svc) {
		nfc_tnep_timer_start();
	}
}

int nfc_tnep_init(struct nfc_tnep_service *services, size_t services_amount)
{
	LOG_DBG("TNEP init");

	int err = 0;

	if (current_state != TNEP_STATE_DISABLED) {
		LOG_DBG("TNEP already running");
		return -ENOTSUP;
	}

	LOG_DBG("k_pool init");
	k_poll_signal_init(&nfc_tnep_msg_rx);
	k_poll_signal_init(&nfc_tnep_msg_tx);
	k_poll_signal_init(&nfc_tnep_sig_timer);

	k_poll_event_init(&nfc_tnep_events[0],
	K_POLL_TYPE_SIGNAL,
			  K_POLL_MODE_NOTIFY_ONLY, &nfc_tnep_msg_rx);

	k_poll_event_init(&nfc_tnep_events[1],
	K_POLL_TYPE_SIGNAL,
			  K_POLL_MODE_NOTIFY_ONLY, &nfc_tnep_msg_tx);

	k_poll_event_init(&nfc_tnep_events[2],
	K_POLL_TYPE_SIGNAL,
			  K_POLL_MODE_NOTIFY_ONLY, &nfc_tnep_sig_timer);

	nfc_tnep_event_offset[0] = TNEP_EVENT_MSG_RX_OFFSET;
	nfc_tnep_event_offset[1] = TNEP_EVENT_MSG_TX_OFFSET;
	nfc_tnep_event_offset[2] = TNEP_EVENT_TIME_OFFSET;

	LOG_DBG("service init");
	if (!services_amount || !services) {
		return -EINVAL;
	}

	LOG_DBG("output buffer check");
	if (!nfc_tnep_tx_buffer || !nfc_tnep_tx_buffer_len) {
		return -EIO;
	}

	tnep_svcs = services;
	tnep_svcs_amount = services_amount;

	LOG_DBG("NDEF message init");
	err = tnep_tx_msg_add_rec_services();

	if (err) {
		LOG_DBG("Error adding service record. Code: %d", err);
		return err;
	}

	LOG_DBG("state machine init");
	current_state = TNEP_STATE_SERVICE_READY;

	k_poll_signal_raise(&nfc_tnep_msg_rx, TNEP_SIGNAL_MSG_RX_INIT);

	return 0;
}

int nfc_tnep_process(void)
{
	int err = k_poll(nfc_tnep_events, NFC_TNEP_EVENTS_NUMBER, K_FOREVER);

	if (err) {
		return err;
	}

	/* Check for signals */
	for (size_t i = 0; i < NFC_TNEP_EVENTS_NUMBER; i++) {

		if (nfc_tnep_events[i].state == K_POLL_STATE_SIGNALED) {
			enum tnep_event event;

			event = nfc_tnep_event_offset[i]
					+ nfc_tnep_events[i].signal->result;

			nfc_tnep_events[i].signal->signaled = 0;
			nfc_tnep_events[i].state = K_POLL_STATE_NOT_READY;

			/* Run TNEP State Machine - prepare response */
			tnep_state_machine[current_state].process(event);

			/* Write response to NFC buffer*/
			err = tnep_tx_msg_encode();
		}
	}

	return err;
}

int nfc_tnep_tx_msg_app_data(struct nfc_ndef_record_desc *record)
{
	if (!record) {
		LOG_DBG("No data in record");
		return -EINVAL;
	};

	int err = tnep_tx_msg_add_rec(record);

	k_poll_signal_raise(&nfc_tnep_msg_tx, TNEP_SIGNAL_MSG_TX_NEW);

	return err;
}

void nfc_tnep_tx_msg_no_app_data(void)
{
	k_poll_signal_raise(&nfc_tnep_msg_tx, TNEP_SIGNAL_MSG_TX_FINISH);
}

void nfc_tnep_uninit(void)
{
	if (active_svc) {
		nfc_tnep_timer_stop();

		active_svc = NULL;
	}

	tnep_tx_msg_clear();

	nfc_tnep_tx_buffer = NULL;
	nfc_tnep_tx_buffer_len = 0;

	current_state = TNEP_STATE_DISABLED;
}

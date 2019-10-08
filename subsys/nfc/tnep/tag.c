/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdbool.h>
#include <kernel.h>
#include <nfc/tnep/tag.h>
#include <nfc/ndef/nfc_ndef_msg.h>
#include <nfc/ndef/msg_parser.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(nfc_tnep_tag, CONFIG_NFC_TNEP_TAG_LOG_LEVEL);

#define TNEP_EVENT_RX_IDX 0
#define TNEP_EVENT_TX_IDX 1

enum tnep_event {
	TNEP_EVENT_DUMMY,
	TNEP_EVENT_MSG_RX_NEW,
	TNEP_EVENT_MSG_TX_NEW,
	TNEP_EVENT_MSG_TX_FINISH,
	TNEP_EVENT_TAG_SELECTED,
};

enum tnep_state_name {
	TNEP_STATE_DISABLED,
	TNEP_STATE_SERVICE_READY,
	TNEP_STATE_SERVICE_SELECTED,
};

enum tnep_svc_record_status {
	TNEP_SVC_NOT_FOUND,
	TNEP_SVC_SELECTED,
	TNEP_SVC_DESELECTED,
};

enum tnep_app_data_state {
	TNEP_APP_DATA_UNEXPECTED,
	TNEP_APP_DATA_EXPECTED
};

static atomic_t current_state = TNEP_STATE_DISABLED;

struct tnep_buffer {
	u8_t *data;
	u8_t *swap_data;
	size_t len;
};

struct tnep_rx_buffer {
	const u8_t *data;
	size_t len;
};

struct tnep_control {
	struct k_poll_event *events;
	struct k_poll_signal msg_rx;
	struct k_poll_signal msg_tx;
};

struct tnep_app_data {
	const struct nfc_ndef_record_desc *record;
	size_t cnt;
	enum nfc_tnep_status_value status;
};

struct tnep_tag {
	struct nfc_ndef_msg_desc *msg;
	struct tnep_buffer tx;
	struct tnep_rx_buffer rx;
	struct tnep_app_data app_data;
	const struct nfc_tnep_tag_service *svc_active;
	const struct nfc_tnep_tag_service *svc;
	const struct nfc_ndef_record_desc *records;
	size_t svc_cnt;
	size_t records_cnt;
	atomic_t app_data_expected;
	nfc_payload_set_t data_set;
	u8_t *current_buff;
};

static struct tnep_control tnep_ctrl;
static struct tnep_tag tnep;

struct tnep_state {
	enum tnep_state_name name;
	void (*process)(enum tnep_event);
};

NFC_TNEP_STATUS_RECORD_DESC_DEF(status_record, NFC_TNEP_STATUS_PROTOCOL_ERROR);

static void tnep_sm_disabled(enum tnep_event event);
static void tnep_sm_service_ready(enum tnep_event event);
static void tnep_sm_service_selected(enum tnep_event event);

const struct tnep_state tnep_state_machine[] = {
	{
		TNEP_STATE_DISABLED,
		tnep_sm_disabled
	},
	{
		TNEP_STATE_SERVICE_READY,
		tnep_sm_service_ready
	},
	{
		TNEP_STATE_SERVICE_SELECTED,
		tnep_sm_service_selected
	},
};

static void tnep_tx_msg_clear(void)
{
	nfc_ndef_msg_clear(tnep.msg);
}

static int tnep_tx_msg_encode(void)
{
	int err = 0;
	unsigned int key;
	size_t len;


	if (tnep.current_buff == tnep.tx.data) {
		tnep.current_buff = tnep.tx.swap_data;
	} else {
		tnep.current_buff = tnep.tx.data;
	}

	memset(tnep.current_buff, 0, tnep.tx.len);

	len = tnep.tx.len;

	if (tnep.msg->record_count > 0) {
		err = nfc_ndef_msg_encode(tnep.msg,
					  tnep.current_buff,
					  &len);
	}

	key = irq_lock();

	__ASSERT_NO_MSG(tnep.data_set);

	tnep.data_set(tnep.current_buff, tnep.tx.len);

	irq_unlock(key);

	return err;
}

static int tnep_tx_msg_add_rec(const struct nfc_ndef_record_desc *record)
{
	int err = 0;

	if (!record) {
		return -EINVAL;
	}

	err = nfc_ndef_msg_record_add(tnep.msg,
				      record);

	return err;
}

static int tnep_tx_msg_add_rec_status(enum nfc_tnep_status_value status)
{
	int err = 0;

	if (status == NFC_TNEP_STATUS_PROTOCOL_ERROR) {
		tnep_tx_msg_clear();
	}

	status_record.status = status;

	err = tnep_tx_msg_add_rec(&NFC_NDEF_TNEP_RECORD_DESC(status_record));

	return err;
}

static int tnep_tx_initial_msg_set(void)
{
	int err = 0;

	tnep_tx_msg_clear();

	for (size_t i = 0; i < tnep.svc_cnt; i++) {
		err = tnep_tx_msg_add_rec(tnep.svc[i].ndef_record);
		if (err) {
			return err;
		}
	}

	if (!tnep.records) {
		return tnep_tx_msg_encode();
	}

	for (size_t i = 0; i < tnep.records_cnt; i++) {
		err = tnep_tx_msg_add_rec(&tnep.records[i]);
		if (err) {
			return err;
		}
	}

	return tnep_tx_msg_encode();
}

static bool ndef_check_rec_type(const struct nfc_ndef_record_desc *record,
				const u8_t *type_field,
				const u32_t type_field_length)
{
	int cmp_res;

	if (record->type_length != type_field_length) {
		return false;
	}

	cmp_res = memcmp(record->type, type_field, type_field_length);

	return (cmp_res == 0);
}

static int tnep_svc_set_active(const u8_t *uri_name, size_t uri_length)
{

	for (size_t i = 0; i < tnep.svc_cnt; i++) {
		if ((tnep.svc[i].parameters->uri_length == uri_length) &&
		    !memcmp(uri_name,
			    tnep.svc[i].parameters->uri, uri_length)) {
			tnep.svc_active = &tnep.svc[i];

			return TNEP_SVC_SELECTED;
		}
	}

	tnep.svc_active = NULL;

	return TNEP_SVC_DESELECTED;
}

static int tnep_svc_select_from_msg(void)
{
	int err;
	enum tnep_svc_record_status svc_status = TNEP_SVC_NOT_FOUND;
	const struct nfc_ndef_msg_desc *msg_p;
	u8_t desc_buf[NFC_NDEF_PARSER_REQIRED_MEMO_SIZE_CALC(
				CONFIG_NFC_TNEP_RX_MAX_RECORD_CNT)];
	size_t desc_buf_len = sizeof(desc_buf);

	err = nfc_ndef_msg_parse(desc_buf, &desc_buf_len, tnep.rx.data,
				 &tnep.rx.len);
	if (err) {
		return -EINVAL;
	}

	msg_p = (struct nfc_ndef_msg_desc *) desc_buf;

	/* Search for service records */
	for (size_t i = 0; i < msg_p->record_count; i++) {
		bool select_rec = ndef_check_rec_type(msg_p->record[i],
						      nfc_ndef_tnep_rec_type_svc_select,
						      NFC_NDEF_TNEP_REC_TYPE_LEN);

		if (select_rec) {
			/* Set service from record as active (or deselect) */
			u8_t bin_rec[CONFIG_NFC_TNEP_RX_MAX_RECORD_SIZE];
			u32_t bin_rec_len = sizeof(bin_rec);
			struct nfc_ndef_tnep_rec_svc_select service;

			memset(bin_rec, 0x00, bin_rec_len);

			msg_p->record[i]->payload_constructor(
					msg_p->record[i]->payload_descriptor,
					bin_rec, &bin_rec_len);

			service.uri_len = bin_rec[0];
			service.uri = &bin_rec[1];

			svc_status = tnep_svc_set_active(service.uri,
							 service.uri_len);

			break;
		}
	}

	return svc_status;
}

static void tnep_error_check(int error_code)
{
	if (!error_code) {
		return;
	}

	LOG_ERR("TNEP error: %d", error_code);

	if (tnep.svc_active) {
		tnep.svc_active->callbacks->error_detected(error_code);
	}
}

static void tnep_sm_disabled(enum tnep_event event)
{
	LOG_DBG("TNEP Disabled!");
}

static int tnep_new_app_msg_prepare(void)
{
	__ASSERT_NO_MSG(tnep.app_data.record);
	__ASSERT_NO_MSG(tnep.app_data.cnt > 0);

	int err;
	size_t cnt = tnep.app_data.cnt;

	tnep_tx_msg_clear();

	err = tnep_tx_msg_add_rec_status(tnep.app_data.status);
	if (err) {
		return err;
	}

	for (size_t i = 0; i < cnt; i++) {
		err = tnep_tx_msg_add_rec(&tnep.app_data.record[i]);
		if (err) {
			return err;
		}
	}

	memset(&tnep.app_data, 0, sizeof(tnep.app_data));

	return tnep_tx_msg_encode();
}

static int service_ready_rx_status_process(enum tnep_svc_record_status status)
{
	int err = 0;

	switch (status) {
	case TNEP_SVC_SELECTED:
		atomic_set(&tnep.app_data_expected, TNEP_APP_DATA_EXPECTED);
		atomic_set(&current_state, TNEP_STATE_SERVICE_SELECTED);

		tnep.svc_active->callbacks->selected();

		break;

	case TNEP_SVC_NOT_FOUND:
	case TNEP_SVC_DESELECTED:
		LOG_DBG("Already in Service Ready state");

		err = tnep_tx_initial_msg_set();

		break;

	default:
		LOG_DBG("Incorrect service select status %d", status);

		err = -ENOTSUP;

		break;
	}

	return err;
}

static void tnep_sm_service_ready(enum tnep_event event)
{
	int err = 0;
	enum tnep_svc_record_status status;

	LOG_DBG("TNEP Service Ready");

	switch (event) {
	case TNEP_EVENT_MSG_RX_NEW:
		tnep_tx_msg_clear();

		/* Encode empty RX message. */
		err = tnep_tx_msg_encode();
		if (err) {
			break;
		}

		status = tnep_svc_select_from_msg();
		err = service_ready_rx_status_process(status);

		break;

	case TNEP_EVENT_TAG_SELECTED:
		err = tnep_tx_initial_msg_set();

		break;

	case TNEP_EVENT_MSG_TX_NEW:
	case TNEP_EVENT_MSG_TX_FINISH:
		LOG_DBG("Operation not enable in Service Ready.");
		break;

	default:
		LOG_ERR("Unknown event in Service Ready %d", event);

		err = -ENOTSUP;

		break;
	}

	tnep_error_check(err);
}

static int service_selected_rx_status_process(const struct nfc_tnep_tag_service *service,
					      enum tnep_svc_record_status status)
{
	int err = 0;

	switch (status) {
	case TNEP_SVC_NOT_FOUND:
		atomic_set(&tnep.app_data_expected, TNEP_APP_DATA_EXPECTED);

		tnep.svc_active->callbacks->message_received(tnep.rx.data,
							     tnep.rx.len);

		break;

	case TNEP_SVC_SELECTED:
		service->callbacks->deselected();

		atomic_set(&tnep.app_data_expected, TNEP_APP_DATA_EXPECTED);

		tnep.svc_active->callbacks->selected();

		break;

	case TNEP_SVC_DESELECTED:
		err = tnep_tx_initial_msg_set();

		service->callbacks->deselected();

		atomic_set(&tnep.app_data_expected, TNEP_APP_DATA_UNEXPECTED);
		atomic_set(&current_state, TNEP_STATE_SERVICE_READY);

		break;

	default:
		LOG_ERR("Incorrect service select status %d", status);

		err = -ENOTSUP;

		break;
	}

	return err;
}

static void tnep_sm_service_selected(enum tnep_event event)
{
	int err = 0;
	const struct nfc_tnep_tag_service *last_service = tnep.svc_active;
	enum tnep_svc_record_status status;

	LOG_DBG("TNEP Service Selected");

	if (!tnep.svc_active) {
		LOG_DBG("No active service in service selected state!");
		return;
	}

	switch (event) {
	case TNEP_EVENT_MSG_RX_NEW:
		tnep_tx_msg_clear();

		/* Encode empty RX message. */
		err = tnep_tx_msg_encode();
		if (err) {
			break;
		}

		status = tnep_svc_select_from_msg();
		err = service_selected_rx_status_process(last_service,
							 status);

		break;

	case TNEP_EVENT_MSG_TX_NEW:
		LOG_DBG("New Application Data");
		err = tnep_new_app_msg_prepare();

		break;

	case TNEP_EVENT_MSG_TX_FINISH:
		tnep_tx_msg_clear();

		err = tnep_tx_msg_add_rec_status(NFC_TNEP_STATUS_SUCCESS);
		if (err) {
			break;
		}

		err = tnep_tx_msg_encode();

		break;

	case TNEP_EVENT_TAG_SELECTED:
		err = tnep_tx_initial_msg_set();

		atomic_set(&current_state, TNEP_STATE_SERVICE_READY);

		break;

	default:
		LOG_DBG("Unknown signal %d", event);
		break;
	}

	tnep_error_check(err);
}

int nfc_tnep_tag_tx_msg_buffer_register(u8_t *tx_buff,
					u8_t *tx_swap_buff,
					size_t len)
{
	if (!tx_buff || !tx_swap_buff || !len) {
		LOG_DBG("Invalid buffer");
		return -EINVAL;
	}

	tnep.tx.len = len;
	tnep.tx.data = tx_buff;
	tnep.tx.swap_data = tx_swap_buff;

	return 0;
}

void nfc_tnep_tag_rx_msg_indicate(const u8_t *rx_buffer, size_t len)
{
	if ((len < 1) || !rx_buffer) {
		return;
	}

	tnep.rx.len = len;
	tnep.rx.data = rx_buffer;

	k_poll_signal_raise(&tnep_ctrl.msg_rx, TNEP_EVENT_MSG_RX_NEW);
}

int nfc_tnep_tag_init(struct k_poll_event *events, u8_t event_cnt,
		      struct nfc_ndef_msg_desc *msg,
		      nfc_payload_set_t payload_set)
{
	LOG_DBG("TNEP initialization");

	if (!events || !msg) {
		return -EINVAL;
	}

	if (msg->max_record_count < 1) {
		LOG_ERR("NDEF Message have to have at leat one place for NDEF Record");
		return -ENOMEM;
	}

	if (!payload_set) {
		LOG_ERR("No function for NFC payload set provided");
		return -EINVAL;
	}

	if (event_cnt != NFC_TNEP_EVENTS_NUMBER) {
		LOG_ERR("Invalid k_pool events count. Got %d events, required %d",
			event_cnt, NFC_TNEP_EVENTS_NUMBER);
		return -EINVAL;
	}

	if (!atomic_cas(&current_state, TNEP_STATE_DISABLED,
			TNEP_STATE_DISABLED)) {
		LOG_ERR("TNEP already running");
		return -ENOTSUP;
	}

	LOG_DBG("Output buffer check");
	if (!tnep.tx.data || !tnep.tx.swap_data || !tnep.tx.len) {
		LOG_ERR("Output buffers are not provided");
		return -EIO;
	}

	tnep_ctrl.events = events;
	tnep.current_buff = tnep.tx.data;
	tnep.msg = msg;
	tnep.data_set = payload_set;

	LOG_DBG("k_pool signals initialization");

	k_poll_signal_init(&tnep_ctrl.msg_rx);
	k_poll_signal_init(&tnep_ctrl.msg_tx);

	k_poll_event_init(&tnep_ctrl.events[TNEP_EVENT_RX_IDX],
			  K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
			  &tnep_ctrl.msg_rx);

	k_poll_event_init(&tnep_ctrl.events[TNEP_EVENT_TX_IDX],
			  K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
			  &tnep_ctrl.msg_tx);

	return 0;
}

int nfc_tnep_tag_initial_msg_create(const struct nfc_tnep_tag_service *svc,
				    size_t svc_cnt,
				    const struct nfc_ndef_record_desc *records,
				    size_t records_cnt)
{
	int err;

	if (!svc || !svc_cnt) {
		return -EINVAL;
	}

	tnep.svc = svc;
	tnep.svc_cnt = svc_cnt;
	tnep.records = records;
	tnep.records_cnt = records_cnt;

	LOG_DBG("NDEF message init");
	err = tnep_tx_initial_msg_set();
	if (err) {
		LOG_ERR("Error adding service record. Code: %d", err);
		return err;
	}

	atomic_set(&current_state, TNEP_STATE_SERVICE_READY);

	return err;
}

void nfc_tnep_tag_process(void)
{
	/* Check for signals */
	for (size_t i = 0; i < NFC_TNEP_EVENTS_NUMBER; i++) {
		if (tnep_ctrl.events[i].state == K_POLL_STATE_SIGNALED) {
			enum tnep_event event;

			event = tnep_ctrl.events[i].signal->result;

			k_poll_signal_reset(tnep_ctrl.events[i].signal);
			tnep_ctrl.events[i].state = K_POLL_STATE_NOT_READY;

			/* Run TNEP State Machine - prepare response */
			tnep_state_machine[atomic_get(&current_state)].process(event);
		}
	}
}

int nfc_tnep_tag_tx_msg_app_data(const struct nfc_ndef_record_desc *record,
				 size_t cnt, enum nfc_tnep_status_value status)
{
	if (!record || (cnt < 1)) {
		LOG_ERR("No data in record");
		return -EINVAL;
	};

	/* Check the state of TNEP service. */
	if (!atomic_cas(&current_state, TNEP_STATE_SERVICE_SELECTED,
			TNEP_STATE_SERVICE_SELECTED)) {
		LOG_ERR("Invalid state. App data can be provided only in service selected state");
		return -EACCES;
	}

	if (!atomic_cas(&tnep.app_data_expected, TNEP_APP_DATA_EXPECTED,
			TNEP_APP_DATA_UNEXPECTED)) {
		LOG_ERR("TNEP App data already set.");
		return -EACCES;
	}

	tnep.app_data.record = record;
	tnep.app_data.cnt = cnt;
	tnep.app_data.status = status;

	k_poll_signal_raise(&tnep_ctrl.msg_tx, TNEP_EVENT_MSG_TX_NEW);

	return 0;
}

int nfc_tnep_tag_tx_msg_no_app_data(void)
{
	/* Check the state of TNEP service. */
	if (!atomic_cas(&current_state, TNEP_STATE_SERVICE_SELECTED,
			TNEP_STATE_SERVICE_SELECTED)) {
		LOG_ERR("Invalid state. App data can be provided only in service selected state");
		return -EACCES;
	}

	if (!atomic_cas(&tnep.app_data_expected, TNEP_APP_DATA_EXPECTED,
			TNEP_APP_DATA_UNEXPECTED)) {
		LOG_ERR("TNEP App data already set.");
		return -EACCES;
	}

	k_poll_signal_raise(&tnep_ctrl.msg_tx, TNEP_EVENT_MSG_TX_FINISH);

	return 0;
}

void nfc_tnep_tag_on_selected(void)
{
	k_poll_signal_raise(&tnep_ctrl.msg_tx, TNEP_EVENT_TAG_SELECTED);
}

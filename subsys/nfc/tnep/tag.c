/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <nfc/tnep/tag.h>
#include <nfc/ndef/msg.h>
#include <nfc/t4t/ndef_file.h>
#include <nfc/ndef/msg_parser.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nfc_tnep_tag, CONFIG_NFC_TNEP_TAG_LOG_LEVEL);

#define TNEP_EVENT_RX_IDX 0
#define TNEP_EVENT_TX_IDX 1

enum tnep_event {
	TNEP_EVENT_DUMMY,
	TNEP_EVENT_MSG_RX_NEW,
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
	uint8_t *data;
	uint8_t *swap_data;
	size_t len;
};

struct tnep_rx_buffer {
	const uint8_t *data;
	size_t len;
};

struct tnep_control {
	struct k_poll_event *events;
	struct k_poll_signal msg_rx;
	struct k_poll_signal msg_tx;
};

struct tnep_tag {
	struct nfc_ndef_msg_desc *msg;
	struct tnep_buffer tx;
	struct tnep_rx_buffer rx;
	const struct nfc_tnep_tag_service *svc_active;
	size_t svc_cnt;
	size_t max_records_cnt;
	atomic_t app_data_expected;
	nfc_payload_set_t data_set;
	initial_msg_encode_t initial_msg_encode;
	uint8_t *current_buff;
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

static void tnep_tx_msg_clear(struct nfc_ndef_msg_desc *msg)
{
	nfc_ndef_msg_clear(msg);
}

static int tnep_tx_msg_encode(struct nfc_ndef_msg_desc *msg)
{
	int err = 0;
	unsigned int key;
	size_t len;
	uint8_t *data;


	if (tnep.current_buff == tnep.tx.data) {
		tnep.current_buff = tnep.tx.swap_data;
	} else {
		tnep.current_buff = tnep.tx.data;
	}

	memset(tnep.current_buff, 0, tnep.tx.len);

	len = tnep.tx.len;
	data = tnep.current_buff;

	if (IS_ENABLED(CONFIG_NFC_T4T_NRFXLIB)) {
		len = nfc_t4t_ndef_file_msg_size_get(len);
		data = nfc_t4t_ndef_file_msg_get(data);
	}

	if (msg && (msg->record_count > 0)) {
		err = nfc_ndef_msg_encode(msg,
					  data,
					  &len);
	}

	if (IS_ENABLED(CONFIG_NFC_T4T_NRFXLIB)) {
		nfc_t4t_ndef_file_encode(tnep.current_buff, &len);
	}

	key = irq_lock();

	__ASSERT_NO_MSG(tnep.data_set);

	tnep.data_set(tnep.current_buff, tnep.tx.len);

	irq_unlock(key);

	return err;
}

static int tnep_tx_msg_add_rec(struct nfc_ndef_msg_desc *msg,
			       const struct nfc_ndef_record_desc *record)
{
	int err = 0;

	if (!record) {
		return -EINVAL;
	}

	err = nfc_ndef_msg_record_add(msg,
				      record);

	return err;
}

static int tnep_tx_msg_add_rec_status(struct nfc_ndef_msg_desc *msg,
				      enum nfc_tnep_status_value status)
{
	int err = 0;

	if (status == NFC_TNEP_STATUS_PROTOCOL_ERROR) {
		tnep_tx_msg_clear(msg);
	}

	status_record.status = status;

	err = tnep_tx_msg_add_rec(msg,
				  &NFC_NDEF_TNEP_RECORD_DESC(status_record));

	return err;
}

static int tnep_tx_initial_msg_set(void)
{
	int err = 0;

	NFC_NDEF_MSG_DEF(initial_msg, tnep.svc_cnt + tnep.max_records_cnt);

	tnep_tx_msg_clear(&NFC_NDEF_MSG(initial_msg));

	/* If initial message callback is available call it to encode
	 * non-TNEP NDEF Records.
	 */
	if (tnep.initial_msg_encode && (tnep.max_records_cnt > 0)) {
		return tnep.initial_msg_encode(&NFC_NDEF_MSG(initial_msg));
	}

	STRUCT_SECTION_FOREACH(nfc_tnep_tag_service, tnep_svc) {
		err = tnep_tx_msg_add_rec(&NFC_NDEF_MSG(initial_msg),
					  tnep_svc->ndef_record);
		if (err) {
			return err;
		}
	}

	return tnep_tx_msg_encode(&NFC_NDEF_MSG(initial_msg));
}

static bool ndef_check_rec_type(const struct nfc_ndef_record_desc *record,
				const uint8_t *type_field,
				const uint32_t type_field_length)
{
	int cmp_res;

	if (record->type_length != type_field_length) {
		return false;
	}

	cmp_res = memcmp(record->type, type_field, type_field_length);

	return (cmp_res == 0);
}

static int tnep_svc_set_active(const uint8_t *uri_name, size_t uri_length)
{
	STRUCT_SECTION_FOREACH(nfc_tnep_tag_service, tnep_svc) {
		if ((tnep_svc->parameters->uri_length == uri_length) &&
		    !memcmp(uri_name,
			    tnep_svc->parameters->uri, uri_length)) {
			tnep.svc_active = tnep_svc;

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
	uint8_t desc_buf[NFC_NDEF_PARSER_REQUIRED_MEM(
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
			uint8_t bin_rec[CONFIG_NFC_TNEP_RX_MAX_RECORD_SIZE];
			uint32_t bin_rec_len = sizeof(bin_rec);
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

static int tnep_new_app_msg_prepare(struct nfc_ndef_msg_desc *msg,
				    enum nfc_tnep_status_value status)
{
	__ASSERT_NO_MSG(msg);

	int err;

	err = tnep_tx_msg_add_rec_status(msg, status);
	if (err) {
		return err;
	}

	return tnep_tx_msg_encode(msg);
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
		/* Encode empty RX message. */
		err = tnep_tx_msg_encode(NULL);
		if (err) {
			break;
		}

		status = tnep_svc_select_from_msg();
		err = service_ready_rx_status_process(status);

		break;

	case TNEP_EVENT_TAG_SELECTED:
		err = tnep_tx_initial_msg_set();

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
		/* Encode empty RX message. */
		err = tnep_tx_msg_encode(NULL);
		if (err) {
			break;
		}

		status = tnep_svc_select_from_msg();
		err = service_selected_rx_status_process(last_service,
							 status);

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

int nfc_tnep_tag_tx_msg_buffer_register(uint8_t *tx_buff,
					uint8_t *tx_swap_buff,
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

void nfc_tnep_tag_rx_msg_indicate(const uint8_t *rx_buffer, size_t len)
{
	if ((len < 1) || !rx_buffer) {
		return;
	}

	tnep.rx.len = len;
	tnep.rx.data = rx_buffer;

	k_poll_signal_raise(&tnep_ctrl.msg_rx, TNEP_EVENT_MSG_RX_NEW);
}

int nfc_tnep_tag_init(struct k_poll_event *events, uint8_t event_cnt,
		      nfc_payload_set_t payload_set)
{
	extern struct nfc_tnep_tag_service
			_nfc_tnep_tag_service_list_start[];
	extern struct nfc_tnep_tag_service
			_nfc_tnep_tag_service_list_end[];

	LOG_DBG("TNEP initialization");

	if (!events) {
		return -EINVAL;
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
	tnep.data_set = payload_set;
	tnep.svc_cnt = _nfc_tnep_tag_service_list_end -
		       _nfc_tnep_tag_service_list_start;

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

int nfc_tnep_tag_initial_msg_create(size_t max_record_cnt,
				    initial_msg_encode_t msg_encode_cb)
{
	int err;

	if (max_record_cnt && !msg_encode_cb) {
		LOG_ERR("No callback provides when maximum NDEF Record count id greater than 0");
		return -EINVAL;
	}

	if (tnep.svc_cnt < 1) {
		LOG_ERR("TNEP Tag services are not defined");
		return -EFAULT;
	}

	tnep.max_records_cnt = max_record_cnt;
	tnep.initial_msg_encode = msg_encode_cb;

	LOG_DBG("NDEF message init");
	err = tnep_tx_initial_msg_set();
	if (err) {
		LOG_ERR("Error adding service record. Code: %d", err);
		return err;
	}

	atomic_set(&current_state, TNEP_STATE_SERVICE_READY);

	return err;
}

int nfc_tnep_initial_msg_encode(struct nfc_ndef_msg_desc *msg,
				const struct nfc_ndef_record_desc *records,
				size_t records_cnt)
{
	int err;
	size_t used_records;

	if (!msg) {
		return -EINVAL;
	}

	if (records && (records_cnt < 1)) {
		return -EINVAL;
	}

	used_records = msg->record_count + tnep.svc_cnt;

	if (records_cnt > (tnep.max_records_cnt - used_records)) {
		return -ENOMEM;
	}

	for (size_t i = 0; i < records_cnt; i++) {
		err = tnep_tx_msg_add_rec(msg,
					  &records[i]);
		if (err) {
			return err;
		}
	}

	STRUCT_SECTION_FOREACH(nfc_tnep_tag_service, tnep_svc) {
		err = tnep_tx_msg_add_rec(msg,
					  tnep_svc->ndef_record);
		if (err) {
			return err;
		}
	}

	return tnep_tx_msg_encode(msg);
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

int nfc_tnep_tag_tx_msg_app_data(struct nfc_ndef_msg_desc *msg,
				 enum nfc_tnep_status_value status)
{
	if (!msg) {
		LOG_ERR("No application data NDEF Message");
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

	return tnep_new_app_msg_prepare(msg, status);
}

int nfc_tnep_tag_tx_msg_no_app_data(void)
{
	int err;

	NFC_NDEF_MSG_DEF(app_msg, 1);

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

	tnep_tx_msg_clear(&NFC_NDEF_MSG(app_msg));

	err = tnep_tx_msg_add_rec_status(&NFC_NDEF_MSG(app_msg),
					 NFC_TNEP_STATUS_SUCCESS);
	if (err) {
		return err;
	}

	return tnep_tx_msg_encode(&NFC_NDEF_MSG(app_msg));
}

void nfc_tnep_tag_on_selected(void)
{
	k_poll_signal_raise(&tnep_ctrl.msg_tx, TNEP_EVENT_TAG_SELECTED);
}

size_t nfc_tnep_tag_svc_count_get(void)
{
	return tnep.svc_cnt;
}

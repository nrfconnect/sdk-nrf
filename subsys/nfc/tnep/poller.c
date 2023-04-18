/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <nfc/t4t/ndef_file.h>
#include <nfc/ndef/msg_parser.h>
#include <nfc/ndef/tnep_rec.h>
#include <nfc/tnep/poller.h>

LOG_MODULE_REGISTER(nfc_tnep_poller, CONFIG_NFC_TNEP_POLLER_LOG_LEVEL);

#define TNEP_SELECT_MSG_RECORD_CNT 1
#define TNEP_DESELECT_MSG_RECORD_CNT 1

/* Minimum len of service parameters records payload. According to
 * NFC Forum Tag NDEF Exchange Protocol 1.0 4.1.2.
 */
#define NFC_TNEP_SERVICE_PARAM_MIN_LEN 7

#define NFC_TNEP_STATUS_RECORD_LEN 1

/* Minimum Waiting time. According to NFC Forum NDEF Tag
 * Exchange Protocol 1.0 4.1.6
 */
#define NFC_TNEP_MIN_WAIT_TIME(_time) \
	(1 << (((DIV_ROUND_UP((_time), 4)) - 1)))

enum tnep_poller_state {
	TNEP_POLLER_STATE_IDLE,
	TNEP_POLLER_STATE_SELECTING,
	TNEP_POLLER_STATE_DESELECTING,
	TNEP_POLLER_STATE_READING,
	TNEP_POLLER_STATE_UPDATING
};

struct tnep_poller {
	struct k_work_delayable tnep_work;
	const struct nfc_tnep_buf *rx;
	const struct nfc_tnep_buf *tx;
	const struct nfc_ndef_tnep_rec_svc_param *svc_to_select;
	const struct nfc_ndef_tnep_rec_svc_param *active_svc;
	const struct nfc_tnep_poller_cb *cb;
	const struct nfc_tnep_poller_ndef_api *api;
	sys_slist_t callback_list;
	int64_t last_time;
	enum tnep_poller_state state;
	enum nfc_tnep_tag_type type;
	uint32_t wait_time;
	uint8_t retry_cnt;
};

static struct tnep_poller tnep;

static void svc_select_notify(const struct nfc_ndef_tnep_rec_svc_param *param,
			      const struct nfc_tnep_poller_msg *msg,
			      bool timeout)
{
	struct nfc_tnep_poller_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&tnep.callback_list, cb, node) {
		if (cb->svc_selected) {
			cb->svc_selected(param, msg, timeout);
		}
	}
}

static void svc_deselect_notify(void)
{
	struct nfc_tnep_poller_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&tnep.callback_list, cb, node) {
		if (cb->svc_deselected) {
			cb->svc_deselected();
		}
	}
}

static void svc_received_notify(const struct nfc_ndef_tnep_rec_svc_param *param,
				const struct nfc_tnep_poller_msg *msg,
				bool timeout)
{
	struct nfc_tnep_poller_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&tnep.callback_list, cb, node) {
		if (cb->svc_received) {
			cb->svc_received(param, msg, timeout);
		}
	}
}

static void svc_sent_notify(const struct nfc_ndef_tnep_rec_svc_param *param,
			    const struct nfc_tnep_poller_msg *msg,
			    bool timeout)
{
	struct nfc_tnep_poller_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&tnep.callback_list, cb, node) {
		if (cb->svc_sent) {
			cb->svc_sent(param, msg, timeout);
		}
	}
}

static void svc_error_notify(int err)
{
	struct nfc_tnep_poller_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&tnep.callback_list, cb, node) {
		if (cb->error) {
			cb->error(err);
		}
	}
}

static int tnep_tag_update_prepare(const struct nfc_ndef_msg_desc *msg,
				   size_t *tx_len)
{
	__ASSERT(tnep.api, "No provided TNEP NDEF Api");
	__ASSERT(tnep.api->ndef_read, "No provided API for reading NDEF");
	__ASSERT(tnep.api->ndef_update, "No provided API for writing NDEF");

	int err;
	size_t len = tnep.tx->size;
	uint8_t *data = tnep.tx->data;

	*tx_len = 0;

	if (tnep.type == NFC_TNEP_TAG_TYPE_T4T) {
		data = nfc_t4t_ndef_file_msg_get(data);

		/* Subtract NDEF NLEN field, to calculate
		 * available space for NDEF Message correctly.
		 */
		len = nfc_t4t_ndef_file_msg_size_get(len);
	}

	/* Encode NDEF Message into raw buffer. */
	err = nfc_ndef_msg_encode(msg,
				  data,
				  &len);
	if (err) {
		LOG_ERR("NDEF message encoding error");
		return err;
	}

	if (tnep.type == NFC_TNEP_TAG_TYPE_T4T) {
		nfc_t4t_ndef_file_encode(tnep.tx->data, &len);
	}

	*tx_len = len;

	return 0;
}

static bool status_record_check(const struct nfc_ndef_record_desc *record,
				const uint8_t *type_field,
				size_t type_field_length)
{
	if (!record->type_length) {
		return false;
	}

	return (memcmp(record->type, type_field, type_field_length) == 0);
}

static int tnep_data_analyze(const struct nfc_ndef_msg_desc *msg,
			     struct nfc_tnep_poller_msg *poller_msg)
{
	/* Look for TNEP Status Record. */
	for (size_t i = 0; i < msg->record_count; i++) {
		const struct nfc_ndef_record_desc *record =
			msg->record[i];
		const struct nfc_ndef_bin_payload_desc *desc =
			(struct nfc_ndef_bin_payload_desc *)record->payload_descriptor;

		if (status_record_check(record, nfc_ndef_tnep_rec_type_status,
					NFC_NDEF_TNEP_REC_TYPE_LEN)) {
			if (desc->payload_length != NFC_TNEP_STATUS_RECORD_LEN) {
				LOG_ERR("Invalid TNEP status record length: %d. Length should be %d",
					desc->payload_length,
					NFC_TNEP_STATUS_RECORD_LEN);

				return -EINVAL;
			}

			poller_msg->msg = msg;
			poller_msg->status = desc->payload[0];

			return 0;
		}
	}

	LOG_DBG("Message does not have any status record");

	return -EAGAIN;
}

static bool service_param_rec_check(uint8_t tnf, const uint8_t *type, uint8_t length)
{
	__ASSERT_NO_MSG(type);

	if (tnf != TNF_WELL_KNOWN) {
		return false;
	}

	if (length != NFC_NDEF_TNEP_REC_TYPE_LEN) {
		return false;
	}

	return (memcmp(type, nfc_ndef_tnep_rec_type_svc_param,
		       NFC_NDEF_TNEP_REC_TYPE_LEN) == 0);
}

static int svc_param_record_get(const struct nfc_ndef_bin_payload_desc *bin_pay_desc,
				struct nfc_ndef_tnep_rec_svc_param *param)
{
	__ASSERT_NO_MSG(bin_pay_desc);
	__ASSERT_NO_MSG(param);

	const uint8_t *nfc_data = bin_pay_desc->payload;

	/* Check length. */
	if (bin_pay_desc->payload_length < NFC_TNEP_SERVICE_PARAM_MIN_LEN) {
		LOG_ERR("To short Service Parameters Record.");
		return -EFAULT;
	}

	param->version = *(nfc_data++);
	param->uri_length = *(nfc_data++);

	/* Check the whole record length with Service Name URI. */
	if (bin_pay_desc->payload_length !=
	    (NFC_TNEP_SERVICE_PARAM_MIN_LEN + param->uri_length)) {
		LOG_ERR("Invalid Service Parameters Record length, expected %d, received: %d.",
			NFC_TNEP_SERVICE_PARAM_MIN_LEN + param->uri_length,
			bin_pay_desc->payload_length);
		return -EFAULT;
	}

	param->uri = nfc_data;

	nfc_data += param->uri_length;

	param->communication_mode = *(nfc_data++);
	param->min_time = *(nfc_data++);
	param->max_time_ext = *(nfc_data++);
	param->max_size = sys_get_be16(nfc_data);

	return 0;
}

static void on_svc_delayed_operation(void)
{
	int ret;
	int64_t time_spent;

	time_spent = k_uptime_delta(&tnep.last_time);

	ret = k_work_reschedule(&tnep.tnep_work,
				time_spent > tnep.wait_time ?
				K_NO_WAIT :
				K_MSEC(tnep.wait_time - time_spent));
	if (ret < 0) {
		svc_error_notify(ret);
		return;
	}

	__ASSERT_NO_MSG(ret == 1);
}

static void tnep_delay_handler(struct k_work *work)
{
	int err = 0;

	switch (tnep.state) {
	case TNEP_POLLER_STATE_SELECTING:
	case TNEP_POLLER_STATE_READING:
	case TNEP_POLLER_STATE_UPDATING:
		err = tnep.api->ndef_read(tnep.rx->data, tnep.rx->size);

		break;

	default:
		err = -EPERM;
		break;
	}

	if (err) {
		svc_error_notify(err);
	}
}

static int on_ndef_read_cb(struct nfc_tnep_poller_msg *poller_msg, bool timeout)
{
	switch (tnep.state) {
	case TNEP_POLLER_STATE_SELECTING:
		svc_select_notify(tnep.active_svc, poller_msg,
				  timeout);

		break;

	case TNEP_POLLER_STATE_READING:
		svc_received_notify(tnep.active_svc, poller_msg,
				    timeout);

		break;

	case TNEP_POLLER_STATE_UPDATING:
		svc_sent_notify(tnep.active_svc, poller_msg,
				timeout);

		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

void nfc_tnep_poller_cb_register(struct nfc_tnep_poller_cb *cb)
{
	if (!cb) {
		return;
	}

	sys_slist_append(&tnep.callback_list, &cb->node);
}

int nfc_tnep_poller_init(const struct nfc_tnep_buf *tx_buf)
{
	if (!tx_buf) {
		return -EINVAL;
	}

	if (!tx_buf->data) {
		LOG_ERR("NULL buffer provided");
		return -EINVAL;
	}

	if (tx_buf->size < 1) {
		LOG_ERR("Tx Buffer length cannot have length equal to 0");
		return -ENOMEM;
	}

	tnep.tx = tx_buf;
	tnep.state = TNEP_POLLER_STATE_IDLE;

	k_work_init_delayable(&tnep.tnep_work, tnep_delay_handler);

	return 0;
}

int nfc_tnep_poller_api_set(const struct nfc_tnep_poller_ndef_api *api,
			    enum nfc_tnep_tag_type tag_type)
{
	if (!api) {
		return -EINVAL;
	}

	if ((tag_type != NFC_TNEP_TAG_TYPE_T2T) &&
	    (tag_type != NFC_TNEP_TAG_TYPE_T4T)) {
		LOG_ERR("Unknown NFC Tag Type");
		return -EINVAL;
	}

	tnep.api = api;
	tnep.type = tag_type;

	return 0;
}

int nfc_tnep_poller_svc_search(const struct nfc_ndef_msg_desc *ndef_msg,
			       struct nfc_ndef_tnep_rec_svc_param *params,
			       uint8_t *cnt)
{
	int err;
	const struct nfc_ndef_record_desc *record;
	uint8_t size;

	if (!ndef_msg || !params || !cnt) {
		LOG_ERR("NULL argument.");
		return -EINVAL;
	}

	if ((*cnt) < 1) {
		return -ENOMEM;
	}

	size = *cnt;
	*cnt = 0;

	if (ndef_msg->record_count < 1) {
		LOG_DBG("No NDEF Records in message.");
		return 0;
	}

	/* Look for TNEP Service Parameters Records. */
	for (size_t i = 0; i < ndef_msg->record_count; i++) {
		record = ndef_msg->record[i];

		/* Check if record is Service Parameters Record. */
		if (service_param_rec_check(record->tnf, record->type,
					    record->type_length)) {
			/* Check if we have memory for service. */
			if ((*cnt) > size) {
				return -ENOMEM;
			}

			/* If service parameters record corrupted.
			 * Search for next valid record.
			 */
			err = svc_param_record_get(record->payload_descriptor,
						   &params[*cnt]);
			if (err) {
				LOG_DBG("Service Parameter Record corrupted.");
				continue;
			}

			(*cnt)++;
		}
	}

	return 0;
}

int nfc_tnep_poller_svc_select(const struct nfc_tnep_buf *svc_buf,
			       const struct nfc_ndef_tnep_rec_svc_param *svc,
			       uint32_t max_ndef_area_size)
{
	int err;
	size_t len = 0;

	NFC_NDEF_MSG_DEF(select_msg, TNEP_SELECT_MSG_RECORD_CNT);

	if (!svc_buf || !svc) {
		return -EINVAL;
	}

	if (!svc_buf->data) {
		LOG_ERR("NULL buffer provided");
		return -EINVAL;
	}

	if (svc_buf->size < 1) {
		LOG_ERR("service buffer length cannot have length equal to 0");
		return -ENOMEM;
	}

	tnep.state = TNEP_POLLER_STATE_SELECTING;

	/* If service is currently selected do not select it twice. */
	if (svc == tnep.active_svc) {
		LOG_DBG("Service currently selected. Reading data");

		on_svc_delayed_operation();

		return 0;
	}

	if (svc->communication_mode != NFC_TNEP_COMM_MODE_SINGLE_RESPONSE) {
		LOG_DBG("Unsupported communication mode");
		return -ENOTSUP;
	}

	if (svc->max_size > max_ndef_area_size) {
		LOG_DBG("Service NDEF max size greater than max NDEF size.");
		LOG_DBG("Service message size should be smaller than %d for this service",
			max_ndef_area_size);
		return -ENOMEM;
	}

	NFC_TNEP_SERIVCE_SELECT_RECORD_DESC_DEF(select_rec, svc->uri_length,
						svc->uri);

	/* Prepare the NDEF message and add the Service Select
	 * Record to it.
	 */
	nfc_ndef_msg_clear(&NFC_NDEF_MSG(select_msg));
	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(select_msg),
				      &NFC_NDEF_TNEP_RECORD_DESC(select_rec));
	if (err) {
		LOG_ERR("Select record cannot be added to the NDEF message");
		return err;
	}

	err = tnep_tag_update_prepare(&NFC_NDEF_MSG(select_msg), &len);
	if (err) {
		LOG_ERR("TNEP NDEF Update prepare error");
		return err;
	}

	tnep.svc_to_select = svc;
	tnep.rx = svc_buf;

	/* Call API to send Update Request. */
	return tnep.api->ndef_update(tnep.tx->data, len);
}

int nfc_tnep_poller_svc_deselect(void)
{
	int err;
	size_t len = 0;

	NFC_NDEF_MSG_DEF(deselect_msg, TNEP_SELECT_MSG_RECORD_CNT);

	/* Check if any service is selected. */
	if (!tnep.active_svc) {
		LOG_DBG("No service is active");
		return 0;
	}

	NFC_TNEP_SERIVCE_SELECT_RECORD_DESC_DEF(deselect_rec, 0,
						NULL);

	nfc_ndef_msg_clear(&NFC_NDEF_MSG(deselect_msg));
	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(deselect_msg),
				      &NFC_NDEF_TNEP_RECORD_DESC(deselect_rec));
	if (err) {
		LOG_ERR("Service Deselect record cannot be added to the NDEF message");
		return err;
	}

	err = tnep_tag_update_prepare(&NFC_NDEF_MSG(deselect_msg), &len);
	if (err) {
		LOG_ERR("TNEP NDEF Update preapre error");
		return err;
	}

	tnep.state = TNEP_POLLER_STATE_DESELECTING;

	/* Call API to send Update Request. */
	return tnep.api->ndef_update(tnep.tx->data, len);
}

int nfc_tnep_poller_svc_read(const struct nfc_tnep_buf *svc_buf)
{
	if (!svc_buf) {
		return -EINVAL;
	}

	if (!svc_buf->data) {
		LOG_ERR("NULL buffer provided");
		return -EINVAL;
	}

	if (svc_buf->size < 1) {
		LOG_ERR("Service buffer length cannot have length equal to 0");
		return -ENOMEM;
	}

	tnep.rx = svc_buf;
	tnep.state = TNEP_POLLER_STATE_READING;

	on_svc_delayed_operation();

	return 0;
}

int nfc_tnep_poller_svc_write(const struct nfc_ndef_msg_desc *msg,
			      const struct nfc_tnep_buf *resp_buf)
{
	int err;
	size_t len = 0;

	if (!msg || !resp_buf) {
		return -EINVAL;
	}

	if (msg->record_count < 1) {
		LOG_ERR("NDEF Message to Update without any NDEF Records");
		return -ENOMEM;
	}

	if (!resp_buf->data) {
		LOG_ERR("NULL buffer provided");
		return -EINVAL;
	}

	if (resp_buf->size < 1) {
		LOG_ERR("Buffer size smaller than 1");
		return -ENOMEM;
	}

	tnep.rx = resp_buf;
	tnep.state = TNEP_POLLER_STATE_UPDATING;

	err = tnep_tag_update_prepare(msg, &len);
	if (err) {
		LOG_ERR("TNEP NDEF Update prepare error");
		return err;
	}

	/* Call API to send Update Request. */
	return tnep.api->ndef_update(tnep.tx->data, len);
}

int nfc_tnep_poller_on_ndef_read(const uint8_t *data, size_t len)
{
	int err;
	uint8_t desc_buf[NFC_NDEF_PARSER_REQUIRED_MEM(CONFIG_NFC_TNEP_POLLER_RX_MAX_RECORD_CNT)];
	size_t desc_buf_len = sizeof(desc_buf);
	struct nfc_tnep_poller_msg poller_msg;
	struct nfc_ndef_msg_desc *msg;

	tnep.last_time = k_uptime_get();

	if (len == 0) {
		if (tnep.retry_cnt < tnep.active_svc->max_time_ext) {
			tnep.retry_cnt++;

			on_svc_delayed_operation();

			return 0;
		}

		return on_ndef_read_cb(NULL, true);
	}

	tnep.retry_cnt = 0;
	memset(&poller_msg, 0, sizeof(poller_msg));

	err = nfc_ndef_msg_parse(desc_buf,
				 &desc_buf_len,
				 data,
				 &len);
	if (err) {
		printk("Error during parsing a NDEF message, err: %d.\n", err);
		return err;
	}

	msg = (struct nfc_ndef_msg_desc *)desc_buf;

	err = tnep_data_analyze(msg, &poller_msg);
	if (err) {
		return err;
	}

	return on_ndef_read_cb(&poller_msg, false);
}

int nfc_tnep_poller_on_ndef_write(void)
{
	tnep.last_time = k_uptime_get();

	switch (tnep.state) {
	case TNEP_POLLER_STATE_SELECTING:
		tnep.active_svc = tnep.svc_to_select;
		tnep.wait_time =
			NFC_TNEP_MIN_WAIT_TIME(tnep.active_svc->min_time);
		tnep.svc_to_select = NULL;

		/* Read service data. */
		on_svc_delayed_operation();

		break;

	case TNEP_POLLER_STATE_DESELECTING:
		tnep.active_svc = NULL;
		tnep.state = TNEP_POLLER_STATE_IDLE;

		svc_deselect_notify();

		break;

	case TNEP_POLLER_STATE_UPDATING:
		/* Read service data after update. */
		on_svc_delayed_operation();

		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

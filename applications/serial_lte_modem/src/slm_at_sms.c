/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <modem/sms.h>
#include "slm_util.h"
#include "slm_at_host.h"

LOG_MODULE_REGISTER(slm_sms, CONFIG_SLM_LOG_LEVEL);

#define MAX_CONCATENATED_MESSAGE  3
#define SLM_SMS_AT_HEADER_INFO_MAX_LEN 64

/**@brief SMS operations. */
enum slm_sms_operation {
	AT_SMS_STOP,
	AT_SMS_START,
	AT_SMS_SEND
};

struct sms_concat_info {
	sys_snode_t node;
	char messages[8 - 1][SMS_MAX_PAYLOAD_LEN_CHARS + 1];
	char *rsp_buf;
	uint16_t ref_number;
	uint8_t total_msgs;
	uint8_t count;
	int64_t last_received_uptime;
};

static sys_slist_t sms_concat_list;

static K_MUTEX_DEFINE(list_mtx);

static int sms_handle;

static struct sms_concat_info *sms_concat_info_node_find(
	struct sms_concat_info **prev_out,
	uint16_t ref_number)
{
	struct sms_concat_info *prev = NULL, *curr;

	SYS_SLIST_FOR_EACH_CONTAINER(&sms_concat_list, curr, node) {
		if (curr->ref_number == ref_number) {
			*prev_out = prev;
			return curr;
		}
		prev = curr;
	}
	return NULL;
}

static void sms_concat_cleanup(void)
{
	struct sms_concat_info *prev = NULL, *curr, *tmp;
	int64_t uptime = k_uptime_get();

	LOG_WRN("sms_concat_cleanup sys_slist_len=%d, uptime=%lld", sys_slist_len(&sms_concat_list), uptime); // TODO REMOVE

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&sms_concat_list, curr, tmp, node) {
		LOG_WRN("ref_number=%d, last_received_uptime=%lld", curr->ref_number, curr->last_received_uptime); // TODO REMOVE
		/* Remove messages that haven't received more parts in 1 hour after
		 * the previous part was received.
		 */
		if (uptime - curr->last_received_uptime > 30000 /* TODO real value: SEC_PER_HOUR * MSEC_PER_SEC*/) {
			LOG_WRN("Cleaned SMS ref number %d, last_received_uptime=%lld, uptime=%lld",
				curr->ref_number, curr->last_received_uptime, uptime);
			sys_slist_remove(&sms_concat_list, &prev->node, &curr->node);
		} else {
			prev = curr;
		}
	}
}

static struct sms_concat_info *sms_concat_info_append(struct sms_deliver_header *header)
{
	struct sms_concat_info *to_ins;

	k_mutex_lock(&list_mtx, K_FOREVER);

	/* Allocate memory and fill. */
	to_ins = (struct sms_concat_info *)k_malloc(sizeof(struct sms_concat_info));
	if (to_ins == NULL) {
		LOG_ERR("Out of memory");
		k_mutex_unlock(&list_mtx);
		return NULL;
	}
	memset(to_ins, 0, sizeof(struct sms_concat_info));
	to_ins->ref_number = header->concatenated.ref_number;
	to_ins->total_msgs = header->concatenated.total_msgs;
	to_ins->count = 0;
	to_ins->last_received_uptime = k_uptime_get();

	size_t concat_msg_size = to_ins->total_msgs * SMS_MAX_PAYLOAD_LEN_CHARS + SLM_SMS_AT_HEADER_INFO_MAX_LEN;
	to_ins->rsp_buf = k_malloc(concat_msg_size);
	memset(to_ins->rsp_buf, 0, concat_msg_size);
	if (to_ins->rsp_buf == NULL) {
		LOG_ERR("No memory for concatenated SMS for %d bytes", concat_msg_size);
		return NULL;
	}

	/* Insert ref_number in the list. */
	sys_slist_append(&sms_concat_list, &to_ins->node);
	k_mutex_unlock(&list_mtx);
	return to_ins;
}

void sms_concat_info_remove(uint16_t ref_number)
{
	struct sms_concat_info *curr, *prev = NULL;

	k_mutex_lock(&list_mtx, K_FOREVER);

	/* Check if the ref_number is registered before removing it. */
	curr = sms_concat_info_node_find(&prev, ref_number);
	if (curr == NULL) {
		LOG_WRN("ref_number not registered. Nothing to do");
		k_mutex_unlock(&list_mtx);
		return;
	}

	k_free(curr->rsp_buf);

	/* Remove the message info from the list. */
	sys_slist_remove(&sms_concat_list, &prev->node, &curr->node);
	k_free(curr);

	k_mutex_unlock(&list_mtx);
	return;
}

static void sms_callback(struct sms_data *const data, void *context)
{
	//static char concat_rsp_buf[SLM_SMS_AT_HEADER_INFO_MAX_LEN + SMS_MAX_PAYLOAD_LEN_CHARS * 8] = {0};
	static char rsp_buf[SLM_SMS_AT_HEADER_INFO_MAX_LEN + SMS_MAX_PAYLOAD_LEN_CHARS] = {0};
	struct sms_concat_info *to_ins = NULL;
	struct sms_concat_info *curr;

	ARG_UNUSED(context);

	if (data == NULL) {
		LOG_WRN("NULL data");
		return;
	}

	if (data->type == SMS_TYPE_DELIVER) {
		struct sms_deliver_header *header = &data->header.deliver;
		LOG_WRN("SMS_TYPE_DELIVER %d", sys_slist_len(&sms_concat_list));

		if (!header->concatenated.present) {
			sprintf(rsp_buf,
				"\r\n#XSMS: \"%02d-%02d-%02d %02d:%02d:%02d UTC%+03d:%02d\",\"",
				header->time.year, header->time.month, header->time.day,
				header->time.hour, header->time.minute, header->time.second,
				header->time.timezone * 15 / 60,
				abs(header->time.timezone) * 15 % 60);
			strcat(rsp_buf, header->originating_address.address_str);
			strcat(rsp_buf, "\",\"");
			strcat(rsp_buf, data->payload);
			strcat(rsp_buf, "\"\r\n");
			rsp_send("%s", rsp_buf);
			rsp_buf[0] = '\0';
		} else {
			LOG_DBG("Concatenated message %d, %d, %d",
				header->concatenated.ref_number,
				header->concatenated.total_msgs,
				header->concatenated.seq_number);

			curr = sms_concat_info_node_find(&to_ins, header->concatenated.ref_number);
			if (curr == NULL) {
				curr = sms_concat_info_append(header);
				if (curr == NULL) {
					goto done;
				}
			}

			/* total_msgs should remain unchanged */
			if (curr->total_msgs != header->concatenated.total_msgs) {
				LOG_ERR("SMS concatenated message total_msgs error: %d, %d",
					curr->total_msgs, header->concatenated.total_msgs);
				goto done;
			}

			/* seq_number should start with 1 but could arrive in random order */
			if (header->concatenated.seq_number == 0 ||
			    header->concatenated.seq_number > curr->total_msgs) {
				LOG_ERR("SMS concatenated message seq_number error: %d, %d",
					header->concatenated.seq_number, curr->total_msgs);
				goto done;
			}
			if (header->concatenated.seq_number == 1) {
				sprintf(curr->rsp_buf,
					"\r\n#XSMS: \"%02d-%02d-%02d %02d:%02d:%02d\",\"",
					header->time.year, header->time.month, header->time.day,
					header->time.hour, header->time.minute,
					header->time.second);
				strcat(curr->rsp_buf, header->originating_address.address_str);
				strcat(curr->rsp_buf, "\",\"");
				strcat(curr->rsp_buf, data->payload);
				//strcpy(concat_rsp_buf, curr->rsp_buf);
			} else {
				strcpy(curr->messages[header->concatenated.seq_number - 2], data->payload);
				strcpy(curr->rsp_buf + SLM_SMS_AT_HEADER_INFO_MAX_LEN + (header->concatenated.seq_number - 1) * SMS_MAX_PAYLOAD_LEN_CHARS,
				       data->payload);
			}
			curr->count++;
			if (curr->count == curr->total_msgs) {
				for (int i = 1; i < curr->total_msgs; i++) {
					//strcat(concat_rsp_buf, curr->messages[i-1]); // TODO REMOVE
					strncat(curr->rsp_buf, curr->rsp_buf + SLM_SMS_AT_HEADER_INFO_MAX_LEN + i * SMS_MAX_PAYLOAD_LEN_CHARS, SMS_MAX_PAYLOAD_LEN_CHARS);
				}
 				// TODO REMOVE
				/*if (strcmp(concat_rsp_buf, curr->rsp_buf) != 0) {
					LOG_ERR("DIFFER");
				}*/
				//LOG_WRN("concat_rsp_buf: %s", concat_rsp_buf);
				LOG_WRN("curr->rsp_buf:  %s", curr->rsp_buf); // TODO REMOVE
				strcat(curr->rsp_buf, "\"\r\n");
				rsp_send("%s", curr->rsp_buf);
				sms_concat_info_remove(curr->ref_number);
			}
		}
	} else if (data->type == SMS_TYPE_STATUS_REPORT) {
		LOG_INF("Status report received");
	} else {
		LOG_WRN("Unknown type: %d", data->type);
	}

done:
	sms_concat_cleanup();
}

static int do_sms_start(void)
{
	int err = 0;

	if (sms_handle >= 0) {
		/* already registered */
		return -EBUSY;
	}

	sms_handle = sms_register_listener(sms_callback, NULL);
	if (sms_handle < 0) {
		err = sms_handle;
		LOG_ERR("SMS start error: %d", err);
		sms_handle = -1;
	}

	return err;
}

static int do_sms_stop(void)
{
	sms_unregister_listener(sms_handle);
	sms_handle = -1;

	return 0;
}

static int do_sms_send(const char *number, const char *message)
{
	int err;

	if (sms_handle < 0) {
		LOG_ERR("SMS not registered");
		return -EPERM;
	}

	err = sms_send_text(number, message);
	if (err) {
		LOG_ERR("SMS send error: %d", err);
	}

	LOG_WRN("do_sms_send sys_slist_len=%d", sys_slist_len(&sms_concat_list)); // TODO REMOVE

	return err;
}

SLM_AT_CMD_CUSTOM(xsms, "AT#XSMS", handle_at_sms);
static int handle_at_sms(enum at_parser_cmd_type cmd_type, struct at_parser *parser, uint32_t)
{
	int err = -EINVAL;
	uint16_t op;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		err = at_parser_num_get(parser, 1, &op);
		if (err) {
			return err;
		}
		if (op == AT_SMS_STOP) {
			err = do_sms_stop();
		} else if (op == AT_SMS_START) {
			err = do_sms_start();
		} else if (op ==  AT_SMS_SEND) {
			char number[SMS_MAX_ADDRESS_LEN_CHARS + 1];
			char message[MAX_CONCATENATED_MESSAGE * SMS_MAX_PAYLOAD_LEN_CHARS];
			int size;

			size = SMS_MAX_ADDRESS_LEN_CHARS + 1;
			err = util_string_get(parser, 2, number, &size);
			if (err) {
				return err;
			}
			size = MAX_CONCATENATED_MESSAGE * SMS_MAX_PAYLOAD_LEN_CHARS;
			err = util_string_get(parser, 3, message, &size);
			if (err) {
				return err;
			}
			err = do_sms_send(number, message);
		} else {
			LOG_WRN("Unknown SMS operation: %d", op);
			err = -EINVAL;
		}
		break;

	case AT_PARSER_CMD_TYPE_TEST:
		rsp_send("\r\n#XSMS: (%d,%d,%d),<number>,<message>\r\n",
			AT_SMS_STOP, AT_SMS_START, AT_SMS_SEND);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief API to initialize SMS AT commands handler
 */
int slm_at_sms_init(void)
{
	sms_handle = -1;

	return 0;
}

/**@brief API to uninitialize SMS AT commands handler
 */
int slm_at_sms_uninit(void)
{
	if (sms_handle >= 0) {
		sms_unregister_listener(sms_handle);
	}

	return 0;
}

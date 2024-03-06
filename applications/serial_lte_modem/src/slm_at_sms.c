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

/**@brief SMS operations. */
enum slm_sms_operation {
	AT_SMS_STOP,
	AT_SMS_START,
	AT_SMS_SEND
};

static int sms_handle;

static void sms_callback(struct sms_data *const data, void *context)
{
	static uint16_t ref_number;
	static uint8_t total_msgs;
	static uint8_t count;
	static char messages[MAX_CONCATENATED_MESSAGE - 1][SMS_MAX_PAYLOAD_LEN_CHARS + 1];
	char rsp_buf[MAX_CONCATENATED_MESSAGE * SMS_MAX_PAYLOAD_LEN_CHARS + 64] = {0};

	ARG_UNUSED(context);

	if (data == NULL) {
		LOG_WRN("NULL data");
		return;
	}

	if (data->type == SMS_TYPE_DELIVER) {
		struct sms_deliver_header *header = &data->header.deliver;

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
		} else {
			LOG_DBG("concatenated message %d, %d, %d",
				header->concatenated.ref_number,
				total_msgs = header->concatenated.total_msgs,
				header->concatenated.seq_number);
			/* ref_number and total_msgs should remain unchanged */
			if (ref_number == 0) {
				ref_number = header->concatenated.ref_number;
			}
			if (ref_number != header->concatenated.ref_number) {
				LOG_ERR("SMS concatenated message ref_number error: %d, %d",
					ref_number, header->concatenated.ref_number);
				goto done;
			}
			if (total_msgs == 0) {
				total_msgs = header->concatenated.total_msgs;
			}
			if (total_msgs != header->concatenated.total_msgs) {
				LOG_ERR("SMS concatenated message total_msgs error: %d, %d",
					total_msgs, header->concatenated.total_msgs);
				goto done;
			}
			if (total_msgs > MAX_CONCATENATED_MESSAGE) {
				LOG_ERR("SMS concatenated message no memory: %d", total_msgs);
				goto done;
			}
			/* seq_number should start with 1 but could arrive in random order */
			if (header->concatenated.seq_number == 0 ||
			    header->concatenated.seq_number > total_msgs) {
				LOG_ERR("SMS concatenated message seq_number error: %d, %d",
					header->concatenated.seq_number, total_msgs);
				goto done;
			}
			if (header->concatenated.seq_number == 1) {
				sprintf(rsp_buf, "\r\n#XSMS: \"%02d-%02d-%02d %02d:%02d:%02d\",\"",
					header->time.year, header->time.month, header->time.day,
					header->time.hour, header->time.minute,
					header->time.second);
				strcat(rsp_buf, header->originating_address.address_str);
				strcat(rsp_buf, "\",\"");
				strcat(rsp_buf, data->payload);
				count++;
			} else {
				strcpy(messages[header->concatenated.seq_number - 2],
					data->payload);
				count++;
			}
			if (count == total_msgs) {
				for (int i = 0; i < (total_msgs - 1); i++) {
					strcat(rsp_buf, messages[i]);
				}
				strcat(rsp_buf, "\"\r\n");
				rsp_send("%s", rsp_buf);
			} else {
				return;
			}
done:
			ref_number = 0;
			total_msgs = 0;
			count = 0;
		}
	} else if (data->type == SMS_TYPE_STATUS_REPORT) {
		LOG_INF("Status report received");
	} else {
		LOG_WRN("Unknown type: %d", data->type);
	}
}

static int do_sms_start(void)
{
	int err = 0;

	if (sms_handle >= 0) {
		/* already registered */
		return -EINVAL;
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
	if (sms_handle < 0) {
		/* not registered yet */
		return -EINVAL;
	}

	sms_unregister_listener(sms_handle);
	sms_handle = -1;

	return 0;
}

static int do_sms_send(const char *number, const char *message)
{
	int err;

	if (sms_handle < 0) {
		/* not registered yet */
		return -EINVAL;
	}

	err = sms_send_text(number, message);
	if (err) {
		LOG_ERR("SMS send error: %d", err);
	}

	return err;
}

SLM_AT_CMD_CUSTOM(xsms, "AT#XSMS", handle_at_sms);
static int handle_at_sms(enum at_cmd_type cmd_type, const struct at_param_list *param_list,
			 uint32_t)
{
	int err = -EINVAL;
	uint16_t op;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(param_list, 1, &op);
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
			err = util_string_get(param_list, 2, number, &size);
			if (err) {
				return err;
			}
			size = MAX_CONCATENATED_MESSAGE * SMS_MAX_PAYLOAD_LEN_CHARS;
			err = util_string_get(param_list, 3, message, &size);
			if (err) {
				return err;
			}
			err = do_sms_send(number, message);
		} else {
			LOG_WRN("Unknown SMS operation: %d", op);
		}
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
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

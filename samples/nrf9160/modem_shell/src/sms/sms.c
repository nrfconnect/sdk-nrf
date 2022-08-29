/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/kernel.h>

#include <modem/sms.h>

#include "sms.h"
#include "mosh_defines.h"
#include "mosh_print.h"

#define PAYLOAD_BUF_SIZE 160
#define SMS_HANDLE_NONE -1

static int sms_handle = SMS_HANDLE_NONE;
static int sms_recv_counter;

static void sms_callback(struct sms_data *const data, void *context)
{
	if (data == NULL) {
		mosh_error("SMS callback with NULL data\n");
	}

	if (data->type == SMS_TYPE_STATUS_REPORT) {
		mosh_print("SMS status report received");
		return;
	} else if (data->type == SMS_TYPE_DELIVER) {
		struct sms_deliver_header *header = &data->header.deliver;

		mosh_print(
			"Time:   %02d-%02d-%02d %02d:%02d:%02d%c%d",
			header->time.year, header->time.month,
			header->time.day, header->time.hour,
			header->time.minute, header->time.second,
			header->time.timezone >= 0 ? '+' : '-',
			header->time.timezone);

		mosh_print("Text:   '%s'", data->payload);
		mosh_print("Length: %d", data->payload_len);

		if (header->app_port.present) {
			mosh_print(
				"Application port addressing scheme: dest_port=%d, src_port=%d",
				header->app_port.dest_port,
				header->app_port.src_port);
		}
		if (header->concatenated.present) {
			mosh_print(
				"Concatenated short messages: ref_number=%d, msg %d/%d",
				header->concatenated.ref_number,
				header->concatenated.seq_number,
				header->concatenated.total_msgs);
		}

		sms_recv_counter++;
	} else {
		mosh_print("SMS protocol message with unknown type received");
	}
}

int sms_register(void)
{
	int handle;

	if (sms_handle != SMS_HANDLE_NONE) {
		return 0;
	}

	handle = sms_register_listener(sms_callback, NULL);
	if (handle) {
		mosh_error("sms_register_listener returned err: %d\n", handle);
		return handle;
	}
	mosh_print("SMS registered");

	sms_handle = handle;
	return 0;
}

int sms_unregister(void)
{
	sms_unregister_listener(sms_handle);
	if (sms_handle != SMS_HANDLE_NONE) {
		/* Only print if we've registered earlier. Otherwise this will cause a crash
		 * when called from other modules if shell is not set.
		 */
		mosh_print("SMS unregistered");
	}

	sms_handle = SMS_HANDLE_NONE;

	return 0;
}

/* Function name is not sms_send() because it's reserved by SMS library. */
int sms_send_msg(char *number, char *text)
{
	int ret;

	if (number == NULL || strlen(number) == 0) {
		mosh_error("Number not given");
		return -EINVAL;
	}
	if (text == NULL || strlen(text) == 0) {
		mosh_error("Text not given");
		return -EINVAL;
	}

	mosh_print("Sending SMS to number=%s, text='%s'", number, text);

	ret = sms_register();
	if (ret != 0) {
		return ret;
	}

	ret = sms_send_text(number, text);
	if (ret) {
		mosh_error("Sending SMS failed with error: %d", ret);
	}

	return ret;
}

int sms_recv(bool arg_receive_start)
{
	if (arg_receive_start) {
		sms_recv_counter = 0;
		mosh_print("SMS receive counter set to zero");
	} else {
		mosh_print("SMS receive counter = %d", sms_recv_counter);
	}

	return 0;
}

/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>

#include <download_client.h>

#include "mock/dl_coap.h"

struct override_return_values_s override_return_values;
struct default_values_s default_values;

void dl_coap_init(size_t file_size, size_t coap_request_send_len)
{
	memset(&override_return_values, 0, sizeof(override_return_values));
	default_values.file_size = file_size;
	default_values.coap_request_send_len = coap_request_send_len;
	default_values.coap_request_send_timeout = 4000;
}

int socket_send(const struct download_client *client, size_t len, int timeout);

int coap_block_init(struct download_client *client, size_t from)
{
	if (override_return_values.func_coap_block_init) {
		return ztest_get_return_value();
	}

	return 0;
}

int coap_get_recv_timeout(struct download_client *dl)
{
	if (override_return_values.func_coap_get_recv_timeout) {
		return ztest_get_return_value();
	}

	return 4000;
}

int coap_initiate_retransmission(struct download_client *dl)
{
	if (override_return_values.func_coap_initiate_retransmission) {
		return ztest_get_return_value();
	}

	return 0;
}

int coap_parse(struct download_client *client, size_t len)
{
	int ret = 0;

	if (client->file_size == 0) {
		client->file_size = default_values.file_size;
	}

	if (override_return_values.func_coap_parse) {
		ret = ztest_get_return_value();
	}

	if (!ret) {
		client->progress += len;
	}

	return ret;
}

int coap_request_send(struct download_client *client)
{
	int err = 0;

	err = socket_send(client, default_values.coap_request_send_len,
			  default_values.coap_request_send_timeout);
	if (err) {
		return err;
	}

	return 0;
}

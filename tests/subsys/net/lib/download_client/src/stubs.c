/*
 * Copyright (c) 2023 grandcentrix GmbH
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "stubs.h"

#include <download_client.h>

extern int socket_send(const struct download_client *client, size_t len, int timeout);

DEFINE_FAKE_VALUE_FUNC(int, coap_block_init, struct download_client *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, coap_get_recv_timeout, struct download_client *);
DEFINE_FAKE_VALUE_FUNC(int, coap_initiate_retransmission, struct download_client *);
DEFINE_FAKE_VALUE_FUNC(int, coap_parse, struct download_client *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, coap_request_send, struct download_client *);

DEFINE_FAKE_VALUE_FUNC(int, http_parse, const struct download_client *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, http_get_request_send, const struct download_client *);

int coap_get_recv_timeout_fake_default(struct download_client *dl)
{
	return 4000;
}

static size_t *mock_coap_initiate_retransmission_ret_values;
static size_t mock_coap_initiate_retransmission_ret_len;
static size_t mock_coap_initiate_retransmission_ret_cnt;

void test_set_ret_val_coap_initiate_retransmission(size_t *values, size_t len)
{
	mock_coap_initiate_retransmission_ret_values = values;
	mock_coap_initiate_retransmission_ret_len = len;
	mock_coap_initiate_retransmission_ret_cnt = 0;
}

int coap_initiate_retransmission_fake_default(struct download_client *dl)
{
	if (mock_coap_initiate_retransmission_ret_cnt < mock_coap_initiate_retransmission_ret_len) {
		return mock_coap_initiate_retransmission_ret_values
			[mock_coap_initiate_retransmission_ret_cnt++];
	}

	return 0;
}

static size_t *mock_coap_parse_ret_values;
static size_t mock_coap_parse_ret_len;
static size_t mock_coap_parse_ret_cnt;

void set_ret_val_coap_parse(size_t *values, size_t len)
{
	mock_coap_parse_ret_values = values;
	mock_coap_parse_ret_len = len;
	mock_coap_parse_ret_cnt = 0;
}

int coap_parse_fake_default(struct download_client *client, size_t len)
{
	int ret = 0;

	if (client->file_size == 0) {
		client->file_size = 75;
	}

	if (mock_coap_parse_ret_cnt < mock_coap_parse_ret_len) {
		ret = mock_coap_parse_ret_values[mock_coap_parse_ret_cnt++];
	}

	if (!ret) {
		client->progress += len;
	}

	return ret;
}

int coap_request_send_fake_default(struct download_client *client)
{
	int err = 0;

	err = socket_send(client, 20, 4000);
	if (err) {
		return err;
	}

	return 0;
}

void test_stubs_teardown(void)
{
	test_set_ret_val_coap_initiate_retransmission(NULL, 0);
	set_ret_val_coap_parse(NULL, 0);
}

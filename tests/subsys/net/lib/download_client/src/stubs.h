/*
 * Copyright (c) 2023 grandcentrix GmbH
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef STUBS_H
#define STUBS_H

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

DECLARE_FAKE_VALUE_FUNC(int, coap_block_init, struct download_client *, size_t);
DECLARE_FAKE_VALUE_FUNC(int, coap_get_recv_timeout, struct download_client *);
DECLARE_FAKE_VALUE_FUNC(int, coap_initiate_retransmission, struct download_client *);
DECLARE_FAKE_VALUE_FUNC(int, coap_parse, struct download_client *, size_t);
DECLARE_FAKE_VALUE_FUNC(int, coap_request_send, struct download_client *);

DECLARE_FAKE_VALUE_FUNC(int, http_parse, const struct download_client *, size_t);
DECLARE_FAKE_VALUE_FUNC(int, http_get_request_send, const struct download_client *);

int coap_get_recv_timeout_fake_default(struct download_client *dl);
void test_set_ret_val_coap_initiate_retransmission(size_t *values, size_t len);
int coap_initiate_retransmission_fake_default(struct download_client *dl);
void set_ret_val_coap_parse(size_t *values, size_t len);
int coap_parse_fake_default(struct download_client *client, size_t len);
int coap_request_send_fake_default(struct download_client *client);
void test_stubs_teardown(void);

#define DO_FOREACH_FAKE(FUNC)                                                                      \
	do {                                                                                       \
		FUNC(coap_block_init)                                                              \
		FUNC(coap_get_recv_timeout)                                                        \
		FUNC(coap_initiate_retransmission)                                                 \
		FUNC(coap_parse)                                                                   \
		FUNC(coap_request_send)                                                            \
		FUNC(http_parse)                                                                   \
		FUNC(http_get_request_send)                                                        \
	} while (0)

#endif /* STUBS_H */

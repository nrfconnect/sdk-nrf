/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DL_COAP_H_
#define _DL_COAP_H_

#include <zephyr/kernel.h>

struct override_return_values_s {
	bool func_coap_block_init;
	bool func_coap_get_recv_timeout;
	bool func_coap_initiate_retransmission;
	bool func_coap_parse;
};

struct default_values_s {
	size_t file_size;
	size_t coap_request_send_len;
	int coap_request_send_timeout;
};

extern struct override_return_values_s override_return_values;
extern struct default_values_s default_values;

void dl_coap_init(size_t file_size, size_t coap_request_send_len);

int coap_block_init(struct download_client *client, size_t from);
int coap_get_recv_timeout(struct download_client *dl);
int coap_initiate_retransmission(struct download_client *dl);
int coap_parse(struct download_client *client, size_t len);
int coap_request_send(struct download_client *client);

#endif /* _DL_COAP_H_ */

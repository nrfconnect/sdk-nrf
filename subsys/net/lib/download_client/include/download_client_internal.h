/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DOWNLOAD_CLIENT_INTERNAL_H
#define DOWNLOAD_CLIENT_INTERNAL_H

#include <net/download_client.h>

int url_parse_port(const char *url, uint16_t *port);
int url_parse_proto(const char *url, int *proto, int *type);
int url_parse_host(const char *url, char *host, size_t len);
int url_parse_file(const char *url, char *file, size_t len);
int http_parse(struct download_client *client, size_t len);
int http_get_request_send(struct download_client *client);

int coap_block_init(struct download_client *client, size_t from);
int coap_get_recv_timeout(struct download_client *dl);
int coap_initiate_retransmission(struct download_client *dl);
int coap_parse(struct download_client *client, size_t len);
int coap_request_send(struct download_client *client);

int socket_send(const struct download_client *client, size_t len, int timeout);

#endif /* DOWNLOAD_CLIENT_INTERNAL_H */

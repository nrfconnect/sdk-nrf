/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DOWNLOAD_CLIENT_INTERNAL_H
#define DOWNLOAD_CLIENT_INTERNAL_H

#define DLC_REQUEST_NEW_DATA 0
#define DLC_KEEP_READING 1

#include <net/download_client.h>

bool use_http(struct download_client *dlc);

int url_parse_port(const char *url, uint16_t *port);
int url_parse_proto(const char *url, int *proto, int *type);
int url_parse_host(const char *url, char *host, size_t len);
int url_parse_file(const char *url, char *file, size_t len);
int http_parse(struct download_client *client, size_t len);
int http_get_request_send(struct download_client *client);

int client_socket_host_lookup(const char * const hostname, uint8_t pdn_id, struct sockaddr *sa);
int client_socket_configure_and_connect(
	int *fd, int proto, int type, uint16_t port, struct sockaddr *remote_addr, //from dlc->sock
	const char *hostname, struct download_client_host_cfg *host_cfg);
int client_socket_close(int *fd);
int client_socket_send(int fd, void *buf, size_t len);
ssize_t client_socket_recv(int fd, void *buf, size_t len);
int client_socket_recv_timeout_set(int fd, int timeout_ms);
int client_socket_send_timeout_set(int fd, int timeout_ms);

#endif /* DOWNLOAD_CLIENT_INTERNAL_H */

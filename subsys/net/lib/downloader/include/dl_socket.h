/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DL_SOCKET_H
#define DL_SOCKET_H

#include <net/downloader.h>
#include <sys/types.h>

int dl_socket_configure_and_connect(
	int *fd, int proto, int type, uint16_t port, struct net_sockaddr *remote_addr,
	const char *hostname, struct downloader_host_cfg *dl_host_cfg);
int dl_socket_close(int *fd);
int dl_socket_send(int fd, void *buf, size_t len);
ssize_t dl_socket_recv(int fd, void *buf, size_t len);
int dl_socket_recv_timeout_set(int fd, uint32_t timeout_ms);
int dl_socket_send_timeout_set(int fd, uint32_t timeout_ms);

#endif /* DL_SOCKET_H */

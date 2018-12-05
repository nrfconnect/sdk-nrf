/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <net/socket.h>
#include <stdio.h>
#include <uart.h>
#include <string.h>

#define HTTP_HOST "google.com"
#define HTTP_PORT 80
#define RECV_BUF_SIZE 8192

char recv_buf[RECV_BUF_SIZE + 1];

int blocking_recv(int fd, u8_t *buf, u32_t size, u32_t flags)
{
	int err;

	do {
		err = recv(fd, buf, size, flags);
	} while (err < 0 && errno == EAGAIN);

	return err;
}

int blocking_send(int fd, u8_t *buf, u32_t size, u32_t flags)
{
	int err;

	do {
		err = send(fd, buf, size, flags);
	} while (err < 0 && errno == EAGAIN);

	return err;
}

int blocking_connect(int fd, struct sockaddr *local_addr, socklen_t len)
{
	int err;

	do {
		err = connect(fd, local_addr, len);
	} while (err < 0 && errno == EAGAIN);

	return err;
}

void app_http_start(void)
{
	struct sockaddr_in local_addr;
	struct addrinfo *res;

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(0);
	local_addr.sin_addr.s_addr = 0;

	printk("HTTP example\n\r");

	int err = getaddrinfo(HTTP_HOST, NULL, NULL, &res);

	printk("getaddrinfo err: %d\n\r", err);
	((struct sockaddr_in *)res->ai_addr)->sin_port = htons(HTTP_PORT);

	char send_buf[] =
		"GET / HTTP/1.1\r\nHost: www.google.com:80\r\nConnection: close\r\n\r\n";
	int send_data_len = strlen(send_buf);

	int client_fd = socket(AF_INET, SOCK_STREAM, 0);

	printk("client_fd: %d\n\r", client_fd);
	err = bind(client_fd, (struct sockaddr *)&local_addr,
		   sizeof(local_addr));
	printk("bind err: %d\n\r", err);
	err = blocking_connect(client_fd, (struct sockaddr *)res->ai_addr,
			       sizeof(struct sockaddr_in));
	printk("connect err: %d\n\r", err);

	int num_bytes = send(client_fd, send_buf, send_data_len, 0);

	printk("send err: %d\n\r", num_bytes);

	int tot_num_bytes = 0;

	do {
		/* TODO: make a proper timeout *
		 * Current solution will just hang 
		 * until remote side closes connection */
		num_bytes =
			blocking_recv(client_fd, recv_buf, RECV_BUF_SIZE, 0);
		tot_num_bytes += num_bytes;

		if (num_bytes <= 0) {
			break;
		}
		printk("%s", recv_buf);
	} while (num_bytes > 0);

	printk("\n\rFinished. Closing socket");
	err = close(client_fd);
}

int main(void)
{
	app_http_start();
	while (1) {
		;
	}
}

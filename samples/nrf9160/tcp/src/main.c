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

#define TCP_HOST "towel.blinkenlights.nl"
#define TCP_PORT 23
#define RECV_BUF_SIZE ((80 * 24) + 1)

char recv_buf[RECV_BUF_SIZE];

u32_t inet_addr(u8_t a, u8_t b, u8_t c, u8_t d)
{
	u32_t value = 0;

	value |= (u32_t)((((u8_t)(d)) << 24) & 0xFF000000);
	value |= (u32_t)((((u8_t)(c)) << 16) & 0x00FF0000);
	value |= (u32_t)((((u8_t)(b)) << 8) & 0x0000FF00);
	value |= (u32_t)((((u8_t)(a)) << 0) & 0x000000FF);

	return value;
}

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

void app_tcp_socket_start(void)
{
	struct addrinfo *res;

	printk("TCP example");

	int err = getaddrinfo(TCP_HOST, NULL, NULL, &res);

	printk("getaddrinfo err: %d\n\r", err);
	((struct sockaddr_in *)res->ai_addr)->sin_port = htons(TCP_PORT);
	struct sockaddr_in local_addr;

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(0);
	local_addr.sin_addr.s_addr = 0;

	int client_fd = socket(AF_INET, SOCK_STREAM, 0);

	printk("client_fd: %d\n\r", client_fd);
	err = bind(client_fd, (struct sockaddr *)&local_addr,
		   sizeof(local_addr));
	printk("bind err: %d\n\r", err);
	err = blocking_connect(client_fd, (struct sockaddr *)res->ai_addr,
			       sizeof(struct sockaddr_in));
	printk("connect err: %d\n\r", errno);

	size_t ret;

	do {
		ret = blocking_recv(client_fd, recv_buf, sizeof(recv_buf) - 1,
				    0);
		if (ret > 0 && ret < RECV_BUF_SIZE) {
			/* Ensure that string is zero terminated */
			recv_buf[ret] = 0;
			printk("%s", recv_buf);
		}
	} while (ret > 0);

	printk("\n\rfin");

	err = close(client_fd);
}

int main(void)
{
	app_tcp_socket_start();
}

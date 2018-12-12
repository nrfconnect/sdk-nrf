/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <net/socket.h>
#include <stdio.h>
#include <string.h>
#include <uart.h>
#include <zephyr.h>

#define HTTP_HOST "google.com"
#define HTTP_PORT 80
#define RECV_BUF_SIZE 1024

char recv_buf[RECV_BUF_SIZE + 1];

const char *at_commands[] = {
	"AT+CGMM", "AT+CGDCONT?", "AT%XCBAND=?", "AT+CGSN=1",
	"AT+CESQ",
	/* Add more here if needed */
};

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

void app_socket_start(void)
{
	int at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);

	printk("Starting simple AT socket application\n\r");

	if (at_socket_fd < 0) {
		printk("Socket err: %d, errno: %d\r\n", at_socket_fd, errno);
	}
	for (int i = 0; i < ARRAY_SIZE(at_commands); i++) {
		int bytes_written = send(at_socket_fd, at_commands[i],
					 strlen(at_commands[i]), 0);
		if (bytes_written > 0) {
			int r_bytes =
				blocking_recv(at_socket_fd, recv_buf,
					      sizeof(recv_buf), MSG_DONTWAIT);
			if (r_bytes > 0) {
				printk("%s", recv_buf);
			}
		}
	}
	printk("Closing socket\n\r");
	(void)close(at_socket_fd);
}

int main(void)
{
	app_socket_start();
	while (1) {
		;
	}
}

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

#define RECV_BUF_SIZE 1024

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

void app_socket_start(void)
{
	struct sockaddr_in remote_addr;
	struct sockaddr_in local_addr;

	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(21);
	/* ftp.uninett.no */
	remote_addr.sin_addr.s_addr = inet_addr(129, 240, 118, 47);

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(0);
	local_addr.sin_addr.s_addr = 0;

	int client_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (client_fd <= 0) {
		printk("client_fd: %d\r\n", client_fd);
		return;
	}

	int err = bind(client_fd, (struct sockaddr *)&local_addr,
		       sizeof(local_addr));
	if (err < 0) {
		printk("bind err: %d\r\n", errno);
		return;
	}

	if (blocking_connect(client_fd, (struct sockaddr *)&remote_addr,
			     sizeof(remote_addr)) < 0) {
		printk("connect err: %d\r\n", errno);
		return;
	}

	if (blocking_recv(client_fd, recv_buf, sizeof(recv_buf), 0) < 0) {
		printk("recv err: %d\r\n", errno);
		return;
	}

	if (blocking_send(client_fd, "USER anonymous\r\n",
			  strlen("USER anonymous\r\n"), 0) < 0) {
		printk("send err: %d\r\n", errno);
		return;
	}

	if (blocking_recv(client_fd, recv_buf, sizeof(recv_buf), 0) < 0) {
		printk("recv err: %d\r\n", errno);
		return;
	}

	if (blocking_send(client_fd, "PASS anonymous\r\n",
			  strlen("PASS anonymous\r\n"), 0) < 0) {
		printk("write err: %d\r\n", errno);
		return;
	}

	if (blocking_recv(client_fd, recv_buf, sizeof(recv_buf), 0) < 0) {
		printk("recv err: %d\r\n", errno);
		return;
	}

	if (blocking_send(client_fd, "EPSV\r\n", strlen("EPSV\r\n"), 0) < 0) {
		printk("write err: %d\r\n", errno);
		return;
	}

	if (blocking_recv(client_fd, recv_buf, sizeof(recv_buf), 0) < 0) {
		printk("recv err: %d\r\n", errno);
		return;
	}

	/* Parse port we get back, and open up a new socket with this port
	 * Expected string by FTP server:
	 * "229 Entering Extended Passive Mode (|||PORT_NUMBER|).\r\n"
	 */
	const char start_pattern[] = "(|||";
	const char end_pattern[] = "|)";

	char *start_ptr = strstr(recv_buf, start_pattern);
	char *stop_ptr = strstr(recv_buf, end_pattern);
	/* Calculate the length of the returned PORT to use */
	int string_size =
		((stop_ptr - start_ptr) - (sizeof(start_pattern) - 1));
	/* five chars max + \0 at the end */
	char port_num_ascii[6] = { 0 };

	memcpy(port_num_ascii, start_ptr + (sizeof(start_pattern) - 1),
	       string_size);

	/* Convert from ASCII to integer */
	int port_num = atoi(port_num_ascii);

	/* Create a new socket for DATA transfer */
	int data_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (data_fd <= 0) {
		printk("data_fd: %d\r\n", errno);
		return;
	}

	/* Update the PORT given by the PASV/EPSV command */
	remote_addr.sin_port = htons(port_num);
	/* Connect to the PORT before issuing the DATA transfer */
	err = blocking_connect(data_fd, (struct sockaddr *)&remote_addr,
			       sizeof(remote_addr));
	if (err < 0) {
		printk("connect err: %d\r\n", errno);
		return;
	}

	char send_buf[] = "RETR robots.txt\r\n";

	if (blocking_send(client_fd, send_buf, strlen(send_buf), 0) < 0) {
		printk("write err: %d\r\n", errno);
		return;
	}
	/* Clear buffer as we've reused it for previous command transfers */
	memset(recv_buf, 0, sizeof(recv_buf));
	/* Note: using data_fd for all data transfers! */
	if (blocking_recv(data_fd, recv_buf, RECV_BUF_SIZE, 0) < 0) {
		printk("recv err: %d\r\n", errno);
		return;
	}
	/* recv_buf shall hold: "User-agent: *\nDisallow: /\n" */
	printk("recv_buf:\n\r%s\n\r", recv_buf);

	(void)close(client_fd);
	(void)close(data_fd);
}

int main(void)
{
	printk("FTP example\n\r");
	app_socket_start();

	while (1) {
		;
	}
}

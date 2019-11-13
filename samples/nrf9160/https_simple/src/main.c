/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <net/socket.h>
#include <stdio.h>
#include <uart.h>
#include <string.h>

#define HTTP_HOST "google.com"
#define HTTP_PORT 443
#define RECV_BUF_SIZE 4096

char recv_buf[RECV_BUF_SIZE + 1];

void app_http_start(void)
{
	struct sockaddr_in local_addr;
	struct addrinfo *res;
	struct addrinfo hints;

	hints.ai_flags = 0;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(HTTP_PORT);
	local_addr.sin_addr.s_addr = 0;

	printk("HTTP example\n\r");

	int err = getaddrinfo(HTTP_HOST, NULL, &hints, &res);
	if (err) {
		printk("getaddrinfo errno %d\n", errno);
		/* No clean up needed, just return */
		return;
	}

	((struct sockaddr_in *)res->ai_addr)->sin_port = htons(HTTP_PORT);

	char send_buf[] =
		"GET / HTTP/1.1\r\n"
		"Host: www.google.com:443\r\n"
		"Connection: close\r\n\r\n";
	int send_data_len = strlen(send_buf);

	int client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);

	enum {
		NONE = 0,
		OPTIONAL = 1,
		REQUIRED = 2,
	};

	int verify = OPTIONAL;

	err = setsockopt(client_fd, SOL_TLS, TLS_PEER_VERIFY, &verify,
			 sizeof(verify));
	if (err) {
		printk("setsockopt err: %d\n", errno);
	}

	err = connect(client_fd, (struct sockaddr *)res->ai_addr,
		      sizeof(struct sockaddr_in));
	if (err > 0) {
		printk("connect err: %d\n", errno);
		goto clean_up;
	}

	int num_bytes = send(client_fd, send_buf, send_data_len, 0);
	if (num_bytes < 0) {
		printk("send errno: %d\n", errno);
		goto clean_up;
	}

	int tot_num_bytes = 0;

	do {
		/* TODO: make a proper timeout *
		 * Current solution will just hang 
		 * until remote side closes connection */
		num_bytes = recv(client_fd, recv_buf, RECV_BUF_SIZE, 0);
		tot_num_bytes += num_bytes;

		if (num_bytes <= 0) {
			printk("\nrecv errno: %d\n", errno);
			break;
		}
		printk("%s", recv_buf);
	} while (num_bytes > 0);

	printk("\n\rFinished. Closing socket");
clean_up:
	freeaddrinfo(res);
	err = close(client_fd);
}

int main(void)
{
	app_http_start();
	while (1) {
		k_cpu_idle();
	}
}

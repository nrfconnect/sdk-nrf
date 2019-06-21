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
#include <time.h>

#define NTP_HOST "ntp.uio.no"
#define NTP_PORT 123
#define RECV_BUF_SIZE 1024

/* Chapter 3: https://www.eecis.udel.edu/~mills/y2k.html 
 * Need to convert from 1900 -> 1970
 */
#define NTP_TO_UNIX_OFFSET 2208988800ULL

char recv_buf[RECV_BUF_SIZE];

/* From NTP RFCs https://www.ietf.org/rfc/rfc5905.txt */
struct ntp_format {
	u8_t flags;
	u8_t stratum; /* stratum */
	u8_t poll; /* poll interval */
	s8_t precision; /* precision */
	u32_t rootdelay; /* root delay */
	u32_t rootdisp; /* root dispersion */
	u32_t refid; /* reference ID */
	u32_t reftime_sec; /* reference time */
	u32_t reftime_frac; /* reference time */
	u32_t org_sec; /* origin timestamp */
	u32_t org_frac; /* origin timestamp */
	u32_t rec_sec; /* receive timestamp */
	u32_t rec_frac; /* receive timestamp */
	u32_t xmt_sec; /* transmit timestamp */
	u32_t xmt_frac; /* transmit timestamp */
};

int blocking_recv(int fd, u8_t *buf, u32_t size, u32_t flags)
{
	int err;

	do {
		err = recv(fd, buf, size, flags);
	} while (err < 0 && errno == EAGAIN);

	return err;
}

int blocking_recvfrom(int fd, void *buf, u32_t size, u32_t flags,
		      struct sockaddr *src_addr, socklen_t *addrlen)
{
	int err;

	do {
		err = recvfrom(fd, buf, size, flags, src_addr, addrlen);
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

void ntp_swap_endianess(struct ntp_format *ntp)
{
	if (ntp) {
		ntp->org_frac = htonl(ntp->org_frac);
		ntp->org_sec = htonl(ntp->org_sec);
		ntp->reftime_frac = htonl(ntp->reftime_frac);
		ntp->reftime_sec = htonl(ntp->reftime_sec);
		ntp->rec_frac = htonl(ntp->rec_frac);
		ntp->rec_sec = htonl(ntp->rec_sec);
		ntp->xmt_frac = htonl(ntp->xmt_frac);
		ntp->xmt_sec = htonl(ntp->xmt_sec);
	}
}

void ntp_print(struct ntp_format *ntp)
{
	if (ntp == NULL) {
		return;
	}

	printk("Flags: %u\n", ntp->flags);
	printk("Stratum: %u\n", ntp->stratum);
	printk("Poll: %u\n", ntp->poll);
	printk("Precision: %u\n", ntp->precision);
	printk("Root delay: %u\n", ntp->rootdelay);
	printk("Root dispersion: %u\n", ntp->rootdisp);
	printk("Ref ID: %u\n", ntp->refid);
	printk("Ref timestamp: %u", ntp->reftime_sec);
	printk("%u\n", ntp->reftime_frac);
	printk("Orig Timestamp: %u", ntp->org_sec);
	printk("%u\n", ntp->org_frac);
	printk("Receive timestamp: %u", ntp->rec_sec);
	printk("%u\n", ntp->rec_frac);
	printk("Transmit: %u", ntp->xmt_sec);
	printk("%u\n", ntp->xmt_frac);

	u32_t unix_time = ntp->xmt_sec - NTP_TO_UNIX_OFFSET;
	printk("Unix time: %d\n", unix_time);

	time_t time = unix_time;
	printf("Time: %s", ctime(&time));
}

void app_ntp_socket_start(void)
{
	struct addrinfo *res;
	struct ntp_format ntp = { 0 };
	socklen_t addrlen = sizeof(struct sockaddr_storage);

	ntp.flags = 0xe3;
	int err = getaddrinfo(NTP_HOST, NULL, NULL, &res);

	printk("getaddrinfo err: %d\n\r", err);
	((struct sockaddr_in *)res->ai_addr)->sin_port = htons(NTP_PORT);
	struct sockaddr_in local_addr;

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(0);
	local_addr.sin_addr.s_addr = 0;

	int client_fd = socket(AF_INET, SOCK_DGRAM, 0);
	printk("client_fd: %d\n\r", client_fd);

	err = bind(client_fd, (struct sockaddr *)&local_addr,
		   sizeof(local_addr));
	printk("bind err: %d\n\r", err);

	err = blocking_connect(client_fd, (struct sockaddr *)res->ai_addr,
			       sizeof(struct sockaddr_in));
	if (err < 0) {
		printk("connect err: %d\n\r", errno);
	}

	err = send(client_fd, &ntp, sizeof(struct ntp_format), 0);
	printk("sendto ret: %d\n\r", err);
	if (err < 0) {
		printk("sendto err: %d\n\r", errno);
	}

	err = blocking_recvfrom(client_fd, &ntp, sizeof(struct ntp_format), 0,
				(struct sockaddr *)res->ai_addr, &addrlen);
	if (err < 0) {
		printk("recvfrom err: %d\n\r", errno);
	}
	ntp_swap_endianess(&ntp);
	ntp_print(&ntp);
	freeaddrinfo(res);
	(void)close(client_fd);
}

int main(void)
{
	app_ntp_socket_start();
}

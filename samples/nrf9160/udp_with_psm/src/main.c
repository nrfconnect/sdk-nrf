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
#include <lte_lc.h>

#define HOST "ntp.uio.no"
#define PORT 123
#define RECV_BUF_SIZE 1024
#define SEND_BUF_SIZE 1024

u8_t recv_buf[RECV_BUF_SIZE];

/* As it is close to impossible to find
 * a proper UDP server, we just connect
 * to a ntp server and read the response
 */
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

void setup_psm(void)
{
	/*
	* GPRS Timer 3 value (octet 3)
	*
	* Bits 5 to 1 represent the binary coded timer value.
	*
	* Bits 6 to 8 defines the timer value unit for the GPRS timer as follows:
	* Bits 
	* 8 7 6
	* 0 0 0 value is incremented in multiples of 10 minutes 
	* 0 0 1 value is incremented in multiples of 1 hour 
	* 0 1 0 value is incremented in multiples of 10 hours
	* 0 1 1 value is incremented in multiples of 2 seconds
	* 1 0 0 value is incremented in multiples of 30 seconds
	* 1 0 1 value is incremented in multiples of 1 minute
	* 1 1 0 value is incremented in multiples of 320 hours (NOTE 1)
	* 1 1 1 value indicates that the timer is deactivated (NOTE 2).
	*/
	char psm_settings[] = CONFIG_LTE_PSM_REQ_RPTAU;
	printk("PSM bits: %c%c%c\n", psm_settings[0], psm_settings[1],
	       psm_settings[2]);
	printk("PSM Interval: %c%c%c%c%c\n", psm_settings[3], psm_settings[4],
	       psm_settings[5], psm_settings[6], psm_settings[7]);
	int err = lte_lc_psm_req(true);
	if (err < 0) {
		printk("Error setting PSM: %d Errno: %d\n", err, errno);
	}
}

void app_udp_start(void)
{
	struct addrinfo *res;
	socklen_t addrlen = sizeof(struct sockaddr_storage);

	int err = getaddrinfo(HOST, NULL, NULL, &res);
	if (err < 0) {
		printk("getaddrinfo err: %d\n\r", err);
		return;
	}
	((struct sockaddr_in *)res->ai_addr)->sin_port = htons(PORT);
	struct sockaddr_in local_addr;

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(0);
	local_addr.sin_addr.s_addr = 0;

	int client_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (client_fd < 0) {
		printk("client_fd: %d\n\r", client_fd);
		goto error;
	}

	err = bind(client_fd, (struct sockaddr *)&local_addr,
		   sizeof(local_addr));
	if (err < 0) {
		printk("bind err: %d errno: %d\n\r", err, errno);
		goto error;
	}

	err = connect(client_fd, (struct sockaddr *)res->ai_addr,
		      sizeof(struct sockaddr_in));
	if (err < 0) {
		printk("connect err: %d errno: %d\n\r", err, errno);
		goto error;
	}

	/* Just hard code the packet format */
	u8_t send_buf[sizeof(struct ntp_format)] = { 0xe3 };

	/*err = sendto(client_fd, send_buf, sizeof(send_buf), 0,
		     (struct sockaddr *)res->ai_addr, addrlen);*/
	err = send(client_fd, send_buf, sizeof(send_buf), 0);
	printk("sendto ret: %d\n\r", err);
	if (err < 0) {
		printk("sendto err: %d errno: %d\n\r", err, errno);
		goto error;
	}

	err = blocking_recvfrom(client_fd, recv_buf, sizeof(recv_buf), 0,
				(struct sockaddr *)res->ai_addr, &addrlen);
	if (err < 0) {
		printk("recvfrom err: %d errno: %d\n\r", err, errno);
		goto error;
	} else {
		printk("Got data back: ");
		for (int i = 0; i < err; i++) {
			printk("%x ", recv_buf[i]);
		}
		printk("\n");
	}

error:
	printk("Finished\n");
	(void)close(client_fd);
}

static volatile bool run_udp;

void app_timer_handler(struct k_timer *dummy)
{
	static u32_t minutes;

	minutes++;
	/* This shall match the PSM interval */
	if (minutes % 10 == 0) {
		run_udp = true;
	}
	printk("Elapsed time: %d\n", minutes);
}

K_TIMER_DEFINE(app_timer, app_timer_handler, NULL);

void timer_init(void)
{
	k_timer_start(&app_timer, K_MINUTES(1), K_MINUTES(1));
}

int main(void)
{
	if (!IS_ENABLED(CONFIG_AT_HOST_LIBRARY)) {
		/* Stop the UART RX for power consumption reasons */
		NRF_UARTE0_NS->TASKS_STOPRX = 1;
	}

	printk("UDP test with PSM\n");
	app_udp_start();
	setup_psm();
	timer_init();
	while (1) {
		k_sleep(500);
		if (run_udp == true) {
			printk("Run UDP\n");
			run_udp = false;
			app_udp_start();
		}
	}
	return 1;
}

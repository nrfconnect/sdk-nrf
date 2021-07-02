/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <net/socket.h>
#include <nrf_socket.h>

#define INVALID_SOCKET		-1
#define ICMP_MAX_URL		128
#define ICMP_MAX_LEN		512

#define ICMP			0x01
#define ICMP_ECHO_REQ		0x08
#define ICMP_ECHO_REP		0x00
#define IP_PROTOCOL_POS		0x09

#define PING_WORK_QUEUE_STACK_SIZE 2048
#define PING_WORK_QUEUE_PRIORITY 5

K_THREAD_STACK_DEFINE(ping_work_q_stack_area, PING_WORK_QUEUE_STACK_SIZE);

/**@ ICMP Ping command arguments */
static struct ping_argv_t {
	struct addrinfo *src;
	struct addrinfo *dest;
	int len;
	int waitms;
	int count;
	int interval;
} ping_argv;

static struct k_work_q my_work_q;
static struct k_work my_work;

static inline void setip(uint8_t *buffer, uint32_t ipaddr)
{
	buffer[0] = ipaddr & 0xFF;
	buffer[1] = (ipaddr >> 8) & 0xFF;
	buffer[2] = (ipaddr >> 16) & 0xFF;
	buffer[3] = ipaddr >> 24;
}

static uint16_t check_ics(const uint8_t *buffer, int len)
{
	const uint32_t *ptr32 = (const uint32_t *)buffer;
	uint32_t hcs = 0;
	const uint16_t *ptr16;

	for (int i = len / 4; i > 0; i--) {
		uint32_t s = *ptr32++;

		hcs += s;
		if (hcs < s) {
			hcs++;
		}
	}

	ptr16 = (const uint16_t *)ptr32;

	if (len & 2) {
		uint16_t s = *ptr16++;

		hcs += s;
		if (hcs < s) {
			hcs++;
		}
	}

	if (len & 1) {
		const uint8_t *ptr8 = (const uint8_t *)ptr16;
		uint8_t s = *ptr8;

		hcs += s;
		if (hcs < s) {
			hcs++;
		}
	}

	while (hcs > 0xFFFF) {
		hcs = (hcs & 0xFFFF) + (hcs >> 16);
	}

	return ~hcs;    /* One's complement */
}

static void calc_ics(uint8_t *buffer, int len, int hcs_pos)
{
	uint16_t *ptr_hcs = (uint16_t *)(buffer + hcs_pos);
	*ptr_hcs = 0;   /* Clear checksum before calculation */
	uint16_t hcs;

	hcs = check_ics(buffer, len);
	*ptr_hcs = hcs;
}

static uint32_t send_ping_wait_reply(void)
{
	static uint8_t seqnr;
	uint16_t total_length;
	uint8_t ip_buf[NET_IPV4_MTU];
	uint8_t *data = NULL;
	static int64_t start_t, delta_t;
	const uint8_t header_len = 20;
	int pllen, len;
	const uint16_t icmp_hdr_len = 8;
	struct sockaddr_in *sa;
	struct nrf_pollfd fds[1];
	int fd;
	int ret;
	int hcs;
	int plseqnr;

	/* Generate IPv4 ICMP EchoReq */
	total_length = ping_argv.len + header_len + icmp_hdr_len;
	memset(ip_buf, 0x00, header_len);

	/* IPv4 header */
	ip_buf[0] = (4 << 4) + (header_len / 4); /* Version & header length */
	ip_buf[1] = 0x00;                        /* Type of service */
	ip_buf[2] = total_length >> 8;           /* Total length */
	ip_buf[3] = total_length & 0xFF;         /* Total length */
	ip_buf[4] = 0x00;                        /* Identification */
	ip_buf[5] = 0x00;                        /* Identification */
	ip_buf[6] = 0x00;                        /* Flags & fragment offset */
	ip_buf[7] = 0x00;                        /* Flags & fragment offset */
	ip_buf[8] = 64;                          /* TTL */
	ip_buf[9] = ICMP;                        /* Protocol */
	/* ip_buf[10..11] = ICS, calculated later */

	sa = (struct sockaddr_in *)ping_argv.src->ai_addr;
	setip(ip_buf+12, sa->sin_addr.s_addr);     /* Source */
	sa = (struct sockaddr_in *)ping_argv.dest->ai_addr;
	setip(ip_buf+16, sa->sin_addr.s_addr);     /* Destination */

	calc_ics(ip_buf, header_len, 10);

	/* ICMP header */
	data = ip_buf + header_len;
	data[0] = ICMP_ECHO_REQ;                 /* Type (echo req) */
	data[1] = 0x00;                          /* Code */
	/* data[2..3] = checksum, calculated later */
	data[4] = 0x00;                         /* Identifier */
	data[5] = 0x00;                         /* Identifier */
	data[6] = seqnr >> 8;                   /* seqnr */
	data[7] = ++seqnr;                      /* seqr */

	/* Payload */
	for (int i = 8; i < total_length - header_len; i++) {
		data[i] = (i + seqnr) % 10 + '0';
	}

	/* ICMP CRC */
	calc_ics(data, total_length - header_len, 2);

	/* Send the ping */
	errno = 0;
	delta_t = 0;
	start_t = k_uptime_get();

	fd = nrf_socket(NRF_AF_PACKET, NRF_SOCK_RAW, 0);
	if (fd < 0) {
		printk("socket() failed: (%d)\n", -errno);
		return (uint32_t)delta_t;
	}

	ret = nrf_send(fd, ip_buf, total_length, 0);
	if (ret <= 0) {
		printk("nrf_send() failed: (%d)\n", -errno);
		goto close_end;
	}

	fds[0].fd = fd;
	fds[0].events = NRF_POLLIN;
	ret = nrf_poll(fds, 1, ping_argv.waitms);
	if (ret <= 0) {
		printk("nrf_poll() failed: (%d) (%d)\n", -errno, ret);
		goto close_end;
	}

	/* receive response */
	do {
		len = nrf_recv(fd, ip_buf, NET_IPV4_MTU, 0);
		if (len <= 0) {
			printk("nrf_recv() failed: (%d) (%d)\n", -errno, len);
			goto close_end;
		}
		if (len < header_len) {
			/* Data length error, ignore silently */
			printk("nrf_recv() wrong data (%d)\n", len);
			continue;
		}
		if (ip_buf[IP_PROTOCOL_POS] != ICMP) {
			/* Not ipv4 echo reply, ignore silently */
			continue;
		}
		break;
	} while (1);

	delta_t = k_uptime_delta(&start_t);

	/* Check ICMP HCS */
	hcs = check_ics(data, len - header_len);
	if (hcs != 0) {
		printk("HCS error %d\n", hcs);
		delta_t = 0;
		goto close_end;
	}
	/* Payload length */
	pllen = (ip_buf[2] << 8) + ip_buf[3];

	/* Check seqnr and length */
	plseqnr = data[7];
	if (plseqnr != seqnr) {
		printk("error sequence numbers %d %d\n", plseqnr, seqnr);
		delta_t = 0;
		goto close_end;
	}
	if (pllen != len) {
		printk("error length %d %d\n", pllen, len);
		delta_t = 0;
		goto close_end;
	}

	/* Result */
	printk("PING: %dms\n", (uint32_t)delta_t);

close_end:
	(void)nrf_close(fd);
	return (uint32_t)delta_t;
}

void ping_task(struct k_work *item)
{
	struct addrinfo *si = ping_argv.src;
	struct addrinfo *di = ping_argv.dest;
	uint32_t sum = 0;
	uint32_t count = 0;

	ARG_UNUSED(item);

	for (int i = 0; i < ping_argv.count; i++) {
		uint32_t ping_t = send_ping_wait_reply();

		if (ping_t > 0)  {
			count++;
			sum += ping_t;
		}
		k_sleep(K_MSEC(ping_argv.interval));
	}

	if (count > 0) {
		uint32_t avg = (sum + count/2) / count;
		printk("PING: average %dms\n", avg);
	}

	freeaddrinfo(si);
	freeaddrinfo(di);
}

int ping(const char *local, const char *remote, int count)
{
	int st;
	struct addrinfo *res;
	int addr_len;

	/* Check network connection status by checking local IP address */
	addr_len = strlen(local);
	if (addr_len == 0) {
		printk("LTE not connected yet\n");
		return -1;
	}
	st = getaddrinfo(local, NULL, NULL, &res);
	if (st != 0) {
		printk("getaddrinfo(src) error: %d\n", st);
		return -st;
	}
	ping_argv.src = res;

	/* Get destination */
	res = NULL;
	st = getaddrinfo(remote, NULL, NULL, &res);
	if (st != 0) {
		printk("getaddrinfo(dest) error: %d\n", st);
		freeaddrinfo(ping_argv.src);
		return -st;
	}
	ping_argv.dest = res;

	if (ping_argv.src->ai_family != ping_argv.dest->ai_family) {
		printk("Source/Destination address family error\n");
		freeaddrinfo(ping_argv.dest);
		freeaddrinfo(ping_argv.src);
		return -1;
	}

	ping_argv.len = 32;		/* default 32 bytes */
	if (IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT) ||
	    IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS)) {
		ping_argv.waitms = 30000;	/* NB-IoT default 30 seconds */
	} else {
		ping_argv.waitms = 5000;	/* M1 default 5 seconds */
	}
	ping_argv.interval = 1000;	/* default 1s */
	ping_argv.count = 1;		/* default 1, adjustable */
	if (count > 0) {
		ping_argv.count = count;
	}

	k_work_submit_to_queue(&my_work_q, &my_work);

	return 0;
}

void ping_init(void)
{
	k_work_queue_start(&my_work_q,
			   ping_work_q_stack_area,
			   K_THREAD_STACK_SIZEOF(ping_work_q_stack_area),
			   PING_WORK_QUEUE_PRIORITY,
			   NULL);
	k_work_init(&my_work, ping_task);
}

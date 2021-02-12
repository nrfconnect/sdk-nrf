/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <net/socket.h>
#include <nrf_socket.h>
#include <modem/modem_info.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_icmp.h"

LOG_MODULE_REGISTER(icmp, CONFIG_SLM_LOG_LEVEL);

#define INVALID_SOCKET		-1
#define ICMP_MAX_URL		128
#define ICMP_MAX_LEN		512

#define ICMP			0x01
#define ICMP_ECHO_REQ		0x08
#define ICMP_ECHO_REP		0x00
#define IP_PROTOCOL_POS		0x09

/*
 * Known limitation in this version
 * - IPv6 support
 */

/**@brief List of supported AT commands. */
enum slm_icmp_at_cmd_type {
	AT_ICMP_PING,
	AT_ICMP_MAX
};

/**@ ICMP Ping command arguments */
static struct ping_argv_t {
	struct addrinfo *src;
	struct addrinfo *dest;
	int len;
	int waitms;
	int count;
	int interval;
} ping_argv;

/* global functions defined in different files */
void rsp_send(const uint8_t *str, size_t len);

/* global variable defined in different files */
extern struct k_work_q slm_work_q;

/** forward declaration of cmd handlers **/
static int handle_at_icmp_ping(enum at_cmd_type cmd_type);

/**@brief SLM AT Command list type. */
static slm_at_cmd_list_t icmp_at_list[AT_ICMP_MAX] = {
	{AT_ICMP_PING, "AT#XPING", handle_at_icmp_ping},
};

static struct k_work my_work;

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_SLM_SOCKET_RX_MAX * 2];

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
		LOG_ERR("socket() failed: (%d)", -errno);
		return (uint32_t)delta_t;
	}

	ret = nrf_send(fd, ip_buf, total_length, 0);
	if (ret <= 0) {
		LOG_ERR("nrf_send() failed: (%d)", -errno);
		goto close_end;
	}

	fds[0].fd = fd;
	fds[0].events = NRF_POLLIN;
	ret = nrf_poll(fds, 1, ping_argv.waitms);
	if (ret <= 0) {
		LOG_ERR("nrf_poll() failed: (%d) (%d)", -errno, ret);
		sprintf(rsp_buf, "#XPING: \"timeout\"\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		goto close_end;
	}

	/* receive response */
	do {
		len = nrf_recv(fd, ip_buf, NET_IPV4_MTU, 0);
		if (len <= 0) {
			LOG_ERR("nrf_recv() failed: (%d) (%d)", -errno, len);
			goto close_end;
		}
		if (len < header_len) {
			/* Data length error, ignore silently */
			LOG_INF("nrf_recv() wrong data (%d)", len);
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
		LOG_WRN("HCS error %d", hcs);
		delta_t = 0;
		goto close_end;
	}
	/* Payload length */
	pllen = (ip_buf[2] << 8) + ip_buf[3];

	/* Check seqnr and length */
	plseqnr = data[7];
	if (plseqnr != seqnr) {
		LOG_WRN("error sequence numbers %d %d", plseqnr, seqnr);
		delta_t = 0;
		goto close_end;
	}
	if (pllen != len) {
		LOG_WRN("error length %d %d", pllen, len);
		delta_t = 0;
		goto close_end;
	}

	/* Result */
	sprintf(rsp_buf, "#XPING: %d.%03d\r\n",
		(uint32_t)(delta_t)/1000,
		(uint32_t)(delta_t)%1000);
	rsp_send(rsp_buf, strlen(rsp_buf));

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

	if (count > 1) {
		uint32_t avg = (sum + count/2) / count;
		int avg_s = avg / 1000;
		int avg_f = avg % 1000;

		sprintf(rsp_buf, "#XPING: \"average %d.%03d\"\r\n",
			avg_s, avg_f);
		rsp_send(rsp_buf, strlen(rsp_buf));
	}

	freeaddrinfo(si);
	freeaddrinfo(di);
	sprintf(rsp_buf, "\r\nOK\r\n");
	rsp_send(rsp_buf, strlen(rsp_buf));
}

static int ping_test_handler(const char *url, int length, int waittime,
			int count, int interval)
{
	int st;
	struct addrinfo *res;
	int addr_len;
	char ipv4_addr[NET_IPV4_ADDR_LEN];

	if (length > ICMP_MAX_LEN) {
		LOG_ERR("Payload size exceeds limit");
		return -1;
	}

	if (!util_get_ipv4_addr(ipv4_addr)) {
		LOG_ERR("Unable to obtain local IPv4 address");
		return -1;
	}

	/* Check network connection status by checking local IP address */
	addr_len = strlen(ipv4_addr);
	if (addr_len == 0) {
		LOG_ERR("LTE not connected yet");
		return -1;
	}
	st = getaddrinfo(ipv4_addr, NULL, NULL, &res);
	if (st != 0) {
		LOG_ERR("getaddrinfo(src) error: %d", st);
		return -st;
	}
	ping_argv.src = res;

	/* Get destination */
	res = NULL;
	st = getaddrinfo(url, NULL, NULL, &res);
	if (st != 0) {
		sprintf(rsp_buf, "\"resolve host error: %d\"\r\n", st);
		rsp_send(rsp_buf, strlen(rsp_buf));
		freeaddrinfo(ping_argv.src);
		return -st;
	}
	ping_argv.dest = res;

	if (ping_argv.src->ai_family != ping_argv.dest->ai_family) {
		LOG_ERR("Source/Destination address family error");
		freeaddrinfo(ping_argv.dest);
		freeaddrinfo(ping_argv.src);
		return -1;
	}

	ping_argv.len = length;
	ping_argv.waitms = waittime;
	ping_argv.count = 1;		/* default 1 */
	ping_argv.interval = 1000;	/* default 1s */
	if (count > 0) {
		ping_argv.count = count;
	}
	if (interval > 0) {
		ping_argv.interval = interval;
	}

	k_work_submit_to_queue(&slm_work_q, &my_work);
	return 0;
}

/**@brief handle AT#XPING commands
 *  AT#XPING=<url>,<length>,<timeout>[,<count>[,<interval>]]
 *  AT#XPING? READ command not supported
 *  AT#XPING=? TEST command not supported
 */
static int handle_at_icmp_ping(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char url[ICMP_MAX_URL];
	int size = ICMP_MAX_URL;
	uint16_t length, timeout, count, interval;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = util_string_get(&at_param_list, 1, url, &size);
		if (err < 0) {
			return err;
		}
		err = at_params_short_get(&at_param_list, 2, &length);
		if (err < 0) {
			return err;
		}
		err = at_params_short_get(&at_param_list, 3, &timeout);
		if (err < 0) {
			return err;
		}
		if (at_params_valid_count_get(&at_param_list) > 4) {
			err = at_params_short_get(&at_param_list, 4, &count);
			if (err < 0) {
				return err;
			};
		} else {
			count = 0;
		}
		if (at_params_valid_count_get(&at_param_list) > 5) {
			err = at_params_short_get(&at_param_list, 5, &interval);
			if (err < 0) {
				return err;
			};
		} else {
			interval = 0;
		}
		err = ping_test_handler(url, length, timeout, count, interval);
		break;

	default:
		break;
	}

	return err;
}

/**@brief API to handle TCP/IP AT commands
 */
int slm_at_icmp_parse(const char *at_cmd)
{
	int ret = -ENOENT;
	enum at_cmd_type type;

	for (int i = 0; i < AT_ICMP_MAX; i++) {
		if (slm_util_cmd_casecmp(at_cmd, icmp_at_list[i].string)) {
			ret = at_parser_params_from_str(at_cmd, NULL,
						&at_param_list);
			if (ret < 0) {
				LOG_ERR("Failed to parse AT command %d", ret);
				return -EINVAL;
			}
			type = at_parser_cmd_type_get(at_cmd);
			ret = icmp_at_list[i].handler(type);
			break;
		}
	}

	return ret;
}

/**@brief API to list ICMP AT commands
 */
void slm_at_icmp_clac(void)
{
	for (int i = 0; i < AT_ICMP_MAX; i++) {
		sprintf(rsp_buf, "%s\r\n", icmp_at_list[i].string);
		rsp_send(rsp_buf, strlen(rsp_buf));
	}
}

/**@brief API to initialize ICMP AT commands handler
 */
int slm_at_icmp_init(void)
{
	k_work_init(&my_work, ping_task);
	return 0;
}

/**@brief API to uninitialize ICMP AT commands handler
 */
int slm_at_icmp_uninit(void)
{
	return 0;
}

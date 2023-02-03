/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/net/socket.h>
#include <nrf_socket.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_icmp.h"

LOG_MODULE_REGISTER(slm_icmp, CONFIG_SLM_LOG_LEVEL);

#define ICMP_DEFAULT_LINK_MTU    1500
#define ICMP_HDR_LEN             8

/* Next header: */
#define IP_NEXT_HEADER_POS  6
#define IP_PROTOCOL_POS     9

#define ICMP_ECHO_REP       0
#define ICMP_ECHO_REQ       8
#define ICMP6_ECHO_REQ      128
#define ICMP6_ECHO_REP      129

/**@ ICMP Ping command arguments */
static struct ping_argv_t {
	struct addrinfo *src;
	struct addrinfo *dest;
	uint16_t len;
	uint16_t waitms;
	uint16_t count;
	uint16_t interval;
	uint16_t pdn;
} ping_argv;

/* global variable defined in different files */
extern struct k_work_q slm_work_q;

static struct k_work ping_work;

/* global variable defined in different files */
extern struct at_param_list at_param_list;

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
	static int64_t start_t;
	int64_t delta_t;
	int32_t timeout;
	static uint8_t seqnr;
	uint16_t total_length;
	uint8_t *buf = NULL;
	uint8_t *data = NULL;
	uint8_t rep = 0;
	uint8_t header_len = 0;
	struct addrinfo *si = ping_argv.src;
	const int alloc_size = ICMP_DEFAULT_LINK_MTU;
	struct pollfd fds[1];
	int dpllen, pllen, len;
	int fd;
	int plseqnr;
	int ret;
	const uint16_t icmp_hdr_len = ICMP_HDR_LEN;

	if (si->ai_family == AF_INET) {
		/* Generate IPv4 ICMP EchoReq */

		/* Ping header */
		header_len = NET_IPV4H_LEN;
		total_length = ping_argv.len + header_len + icmp_hdr_len;
		buf = calloc(1, alloc_size);
		if (buf == NULL) {
			LOG_ERR("No RAM memory available for sending ping.");
			return -1;
		}

		buf[0] = (4 << 4) + (header_len / 4);      /* Version & header length */
		/* buf[1] = 0; */                          /* Type of service */
		buf[2] = total_length >> 8;                /* Total length */
		buf[3] = total_length & 0xFF;              /* Total length */
		/* buf[4..5] = 0; */                       /* Identification */
		/* buf[6..7] = 0; */                       /* Flags & fragment offset */
		buf[8] = 64;                               /* TTL */
		buf[9] = IPPROTO_ICMP;                     /* Protocol */
		/* buf[10..11] */                          /* ICS, calculated later */

		struct sockaddr_in *sa = (struct sockaddr_in *)ping_argv.src->ai_addr;

		setip(buf + 12, sa->sin_addr.s_addr);      /* Source */
		sa = (struct sockaddr_in *)ping_argv.dest->ai_addr;
		setip(buf + 16, sa->sin_addr.s_addr);      /* Destination */

		calc_ics(buf, header_len, 10);

		/* ICMP header */
		data = buf + header_len;
		data[0] = ICMP_ECHO_REQ;                    /* Type (echo req) */
		/* data[1] = 0; */                          /* Code */
		/* data[2..3] */                            /* checksum, calculated later */
		/* data[4..5] = 0; */                       /* Identifier */
		/* data[6] = 0;  */                         /* seqnr >> 8 */
		data[7] = ++seqnr;

		/* Payload */
		for (int i = 8; i < total_length - header_len; i++) {
			data[i] = (i + seqnr) % 10 + '0';
		}

		/* ICMP CRC */
		calc_ics(data, total_length - header_len, 2);
		rep = ICMP_ECHO_REP;
	} else {
		/* Generate IPv6 ICMP EchoReq */

		/* ipv6 header */
		header_len = NET_IPV6H_LEN;

		uint16_t payload_length = ping_argv.len + icmp_hdr_len;

		total_length = payload_length + header_len;
		buf = calloc(1, alloc_size);
		if (buf == NULL) {
			LOG_ERR("No RAM memory available for sending ping.");
			return -1;
		}

		buf[0] = (6 << 4);              /* Version & traffic class 4 bits */
		/* buf[1..3] = 0; */            /* Traffic class 4 bits & flow label */
		buf[4] = payload_length >> 8;   /* Payload length */
		buf[5] = payload_length & 0xFF; /* Payload length */
		buf[6] = IPPROTO_ICMPV6;        /* Next header (58) */
		buf[7] = 64;                    /* Hop limit */

		struct sockaddr_in6 *sa = (struct sockaddr_in6 *)ping_argv.src->ai_addr;

		memcpy(buf + 8, sa->sin6_addr.s6_addr, 16); /* Source address */
		sa = (struct sockaddr_in6 *)ping_argv.dest->ai_addr;
		memcpy(buf + 24, sa->sin6_addr.s6_addr, 16); /* Destination address */

		/* ICMPv6 header */
		data = buf + header_len;
		data[0] = ICMP6_ECHO_REQ;        /* Type (echo req) */
		/* data[1] = 0; */               /* Code */
		/* data[2..3] */                 /* checksum, calculated later */
		/* data[4..5] = 0; */            /* Identifier */
		/* data[6] = 0; */               /* seqnr >> 8 */
		data[7] = ++seqnr;

		/* Payload */
		for (int i = 0; i < ping_argv.len; i++) {
			data[i + icmp_hdr_len] = (i + seqnr) % 10 + '0';
		}

		/* ICMPv6 CRC: https://tools.ietf.org/html/rfc4443#section-2.3
		 * for IPv6 pseudo header see: https://tools.ietf.org/html/rfc2460#section-8.1
		 */
		uint32_t hcs = check_ics(buf + 8, 32);   /* Pseudo header: source + dest */
		uint8_t tbuf[2];

		hcs += check_ics(buf + 4, 2);            /* Pseudo header: payload length */

		tbuf[0] = 0;
		tbuf[1] = buf[6];

		hcs += check_ics(tbuf, 2);                /* Pseudo header: Next header */
		hcs += check_ics(data, 2);                /* ICMP: Type & Code */
		hcs += check_ics(data + 4, 4 + ping_argv.len); /* ICMP: Header data + Data */
		while (hcs > 0xFFFF) {
			hcs = (hcs & 0xFFFF) + (hcs >> 16);
		}
		data[2] = hcs & 0xFF;
		data[3] = hcs >> 8;
		rep = ICMP6_ECHO_REP;
	}

	/* Send the ping */
	errno = 0;
	delta_t = 0;

	fd = socket(AF_PACKET, SOCK_RAW, 0);
	if (fd < 0) {
		LOG_ERR("socket() failed: (%d)", -errno);
		free(buf);
		return (uint32_t)delta_t;
	}

	/* Use non-primary PDN if specified, fail if cannot proceed
	 */
	if (ping_argv.pdn != 0) {
		size_t len;
		struct ifreq ifr = { 0 };

		snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "pdn%d", ping_argv.pdn);
		len = strlen(ifr.ifr_name);
		if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, len) < 0) {
			LOG_WRN("Unable to set socket SO_BINDTODEVICE, abort");
			goto close_end;
		}
		LOG_DBG("Use PDN: %d", ping_argv.pdn);
	} else {
		LOG_DBG("Use PDN: 0");
	}

	/* We have a blocking socket and we do not want to block for
	 * a long for sending. Thus, let's set the timeout:
	 */
	struct timeval tv = {
		.tv_sec = (ping_argv.waitms / 1000),
		.tv_usec = (ping_argv.waitms % 1000) * 1000,
	};

	if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (struct timeval *)&tv,
		       sizeof(struct timeval)) < 0) {
		LOG_WRN("Unable to set socket SO_SNDTIMEO, continue");
	}

	/* Just for sure, let's put the timeout for rcv as well
	 * (should not be needed for non-blocking socket):
	 */
	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv,
		       sizeof(struct timeval)) < 0) {
		LOG_WRN("Unable to set socket SO_RCVTIMEO, continue");
	}

	/* Include also a sending time to measured RTT: */
	start_t = k_uptime_get();
	timeout = ping_argv.waitms;

	ret = send(fd, buf, total_length, 0);
	if (ret <= 0) {
		LOG_ERR("send() failed: (%d)", -errno);
		goto close_end;
	}

	/* Now after send(), calculate how much there's still time left */
	delta_t = k_uptime_delta(&start_t);
	timeout = ping_argv.waitms - (int32_t)delta_t;

wait_for_data:
	fds[0].fd = fd;
	fds[0].events = POLLIN;

	do {
		if (timeout <= 0) {
			LOG_WRN("Pinging result: no ping response in given timeout msec");
			delta_t = 0;
			goto close_end;
		}

		ret = poll(fds, 1, timeout);
		if (ret == 0) {
			LOG_WRN("Pinging result: no ping response in given timeout msec");
			delta_t = 0;
			goto close_end;
		} else if (ret < 0) {
			LOG_ERR("poll() failed: (%d) (%d)", -errno, ret);
			delta_t = 0;
			goto close_end;
		}

		len = recv(fd, buf, alloc_size, 0);

		/* Calculate again, how much there's still time left */
		delta_t = k_uptime_delta(&start_t);
		timeout = ping_argv.waitms - (int32_t)delta_t;

		if (len <= 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				LOG_ERR("recv() failed with errno (%d) and return value (%d)",
					-errno, len);
				delta_t = 0;
				goto close_end;
			}
		}
		if (len < header_len) {
			/* Data length error, ignore "silently" */
			LOG_ERR("recv() wrong data (%d)", len);
		}

		if ((rep == ICMP_ECHO_REP && buf[IP_PROTOCOL_POS] == IPPROTO_ICMP) ||
		    (rep == ICMP6_ECHO_REP && buf[IP_NEXT_HEADER_POS] == IPPROTO_ICMPV6)) {
			/* if not ipv4/ipv6 echo reply, ignore silently,
			 * otherwise break the loop and go to parse the response
			 */
			break;
		}
	} while (true);

	if (rep == ICMP_ECHO_REP) {
		/* Check ICMP HCS */
		int hcs = check_ics(data, len - header_len);

		if (hcs != 0) {
			LOG_ERR("IPv4 HCS error, hcs: %d, len: %d\r\n", hcs, len);
			delta_t = 0;
			goto close_end;
		}
		pllen = (buf[2] << 8) + buf[3]; /* Raw socket payload length */
	} else {
		/* Check ICMP6 CRC */
		uint32_t hcs = check_ics(buf + 8, 32);   /* Pseudo header source + dest */
		uint8_t tbuf[2];

		hcs += check_ics(buf + 4, 2);   /* Pseudo header packet length */

		tbuf[0] = 0;
		tbuf[1] = buf[6];
		hcs += check_ics(tbuf, 2);              /* Pseudo header Next header */
		hcs += check_ics(data, 2);              /* Type & Code */
		hcs += check_ics(data + 4, len - header_len - 4); /* Header data + Data */

		while (hcs > 0xFFFF) {
			hcs = (hcs & 0xFFFF) + (hcs >> 16);
		}

		int plhcs = data[2] + (data[3] << 8);

		if (plhcs != hcs) {
			LOG_ERR("IPv6 HCS error: 0x%x 0x%x\r\n", plhcs, hcs);
			delta_t = 0;
			goto close_end;
		}
		/* Raw socket payload length */
		pllen = (buf[4] << 8) + buf[5] + header_len; /* Payload length - hdr */
	}

	/* Data payload length: */
	dpllen = pllen - header_len - icmp_hdr_len;

	/* Check seqnr and length */
	plseqnr = data[7];
	if (plseqnr != seqnr) {
		/* This is not the reply you are looking for */

		/* Wait for the next response if there' still time */
		if (timeout > 0) {
			goto wait_for_data;
		} else {
			delta_t = 0;
			goto close_end;
		}
	}
	if (pllen != len) {
		LOG_ERR("Expected length %d, got %d", len, pllen);
		delta_t = 0;
		goto close_end;
	}

	/* Result */
	rsp_send("#XPING: %d.%03d seconds\r\n",
		(uint32_t)(delta_t)/1000, (uint32_t)(delta_t)%1000);

close_end:
	(void)close(fd);
	free(buf);
	return (uint32_t)delta_t;
}

void ping_task(struct k_work *item)
{
	struct addrinfo *si = ping_argv.src;
	struct addrinfo *di = ping_argv.dest;
	uint32_t sum = 0;
	uint32_t count = 0;
	uint32_t rtt_min = 0xFFFFFFFF;
	uint32_t rtt_max = 0;
	uint32_t lost;

	ARG_UNUSED(item);

	for (int i = 0; i < ping_argv.count; i++) {
		uint32_t ping_t = send_ping_wait_reply();

		if (ping_t > 0)  {
			count++;
			sum += ping_t;
			rtt_max = MAX(rtt_max, ping_t);
			rtt_min = MIN(rtt_min, ping_t);
		}
		k_sleep(K_MSEC(ping_argv.interval));
	}

	lost = ping_argv.count - count;
	LOG_INF("Packets: Sent = %d, Received = %d, Lost = %d (%d%% loss)",
		ping_argv.count, count, lost, lost * 100 / ping_argv.count);

	if (count > 1) {
		uint32_t avg = (sum + count/2) / count;
		int avg_s = avg / 1000;
		int avg_f = avg % 1000;

		rsp_send("#XPING: average %d.%03d seconds\r\n",
			avg_s, avg_f);

		LOG_INF("Approximate round trip times in milli-seconds:\n"
			"    Minimum = %dms, Maximum = %dms, Average = %dms",
			rtt_min, rtt_max, sum / count);
	}

	freeaddrinfo(si);
	freeaddrinfo(di);
}

static int ping_test_handler(const char *target)
{
	int ret;
	struct addrinfo *res;

	ret = getaddrinfo(target, NULL, NULL, &res);
	if (ret != 0) {
		LOG_ERR("getaddrinfo(dest) error: %d", ret);
		rsp_send("\"%s\"\r\n", gai_strerror(ret));
		return -EAGAIN;
	}

	/* Use the first result to decide which address family to use */
	if (res->ai_family == AF_INET) {
		char ipv4_addr[NET_IPV4_ADDR_LEN] = {0};

		LOG_INF("Ping target's IPv4 address");
		util_get_ip_addr(ping_argv.pdn, ipv4_addr, NULL);
		if (strlen(ipv4_addr) == 0) {
			LOG_ERR("Unable to obtain local IPv4 address");
			freeaddrinfo(res);
			return -1;
		}

		ping_argv.dest = res;
		res = NULL;
		ret = getaddrinfo(ipv4_addr, NULL, NULL, &res);
		if (ret != 0) {
			LOG_ERR("getaddrinfo(src) error: %d", ret);
			freeaddrinfo(ping_argv.dest);
			return -ret;
		}
		ping_argv.src = res;
	} else if (res->ai_family == AF_INET6) {
		char ipv6_addr[NET_IPV6_ADDR_LEN] = {0};

		LOG_INF("Ping target's IPv6 address");
		util_get_ip_addr(ping_argv.pdn, NULL, ipv6_addr);
		if (strlen(ipv6_addr) == 0) {
			LOG_ERR("Unable to obtain local IPv6 address");
			freeaddrinfo(res);
			return -1;
		}

		ping_argv.dest = res;
		res = NULL;
		ret = getaddrinfo(ipv6_addr, NULL, NULL, &res);
		if (ret != 0) {
			LOG_ERR("getaddrinfo(src) error: %d", ret);
			freeaddrinfo(ping_argv.dest);
			return -ret;
		}
		ping_argv.src = res;
	} else {
		LOG_WRN("Address family not supported: %d", res->ai_family);
	}

	k_work_submit_to_queue(&slm_work_q, &ping_work);
	return 0;
}

/**@brief handle AT#XPING commands
 *  AT#XPING=<url>,<length>,<timeout>[,<count>[,<interval>[,<pdn>]]]
 *  AT#XPING? READ command not supported
 *  AT#XPING=? TEST command not supported
 */
int handle_at_icmp_ping(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char target[SLM_MAX_URL];
	int size = SLM_MAX_URL;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = util_string_get(&at_param_list, 1, target, &size);
		if (err < 0) {
			return err;
		}
		err = at_params_unsigned_short_get(&at_param_list, 2, &ping_argv.len);
		if (err < 0) {
			return err;
		}
		err = at_params_unsigned_short_get(&at_param_list, 3, &ping_argv.waitms);
		if (err < 0) {
			return err;
		}
		ping_argv.count = 1; /* default 1 */
		if (at_params_valid_count_get(&at_param_list) > 4) {
			err = at_params_unsigned_short_get(&at_param_list, 4, &ping_argv.count);
			if (err < 0) {
				return err;
			};
		}
		ping_argv.interval = 1000; /* default 1s */
		if (at_params_valid_count_get(&at_param_list) > 5) {
			err = at_params_unsigned_short_get(&at_param_list, 5, &ping_argv.interval);
			if (err < 0) {
				return err;
			};
		}
		ping_argv.pdn = 0; /* default 0 primary PDN */
		if (at_params_valid_count_get(&at_param_list) > 6) {
			err = at_params_unsigned_short_get(&at_param_list, 6, &ping_argv.pdn);
			if (err < 0) {
				return err;
			};
		}

		err = ping_test_handler(target);
		break;

	default:
		break;
	}

	return err;
}

/**@brief API to initialize ICMP AT commands handler
 */
int slm_at_icmp_init(void)
{
	k_work_init(&ping_work, ping_task);
	return 0;
}

/**@brief API to uninitialize ICMP AT commands handler
 */
int slm_at_icmp_uninit(void)
{
	return 0;
}

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#include <zephyr.h>

#include <modem/modem_info.h>

#include <fcntl.h>
#include <posix/unistd.h>
#include <posix/netdb.h>

#include <posix/poll.h>
#include <posix/sys/socket.h>

#include <posix/arpa/inet.h>

#include "utils/net_utils.h"
#include "link_api.h"
#include "mosh_defines.h"
#include "mosh_print.h"

#include "icmp_ping.h"

/* Protocol field: */
#define ICMP 1
#define ICMP6 6
#define ICMPV6 58

/* Next header: */
#define IP_NEXT_HEADER_POS 6
#define IP_PROTOCOL_POS 9

#define ICMP_ECHO_REP 0
#define ICMP_ECHO_REQ 8
#define ICMP6_ECHO_REQ 128
#define ICMP6_ECHO_REP 129

extern struct k_poll_signal mosh_signal;

enum ping_rai {
	PING_RAI_NONE,
	PING_RAI_ONGOING,
	PING_RAI_LAST_PACKET
};

/* ICMP Ping command arguments */
static struct icmp_ping_shell_cmd_argv ping_argv;

/*****************************************************************************/

static inline void setip(uint8_t *buffer, uint32_t ipaddr)
{
	buffer[0] = ipaddr & 0xFF;
	buffer[1] = (ipaddr >> 8) & 0xFF;
	buffer[2] = (ipaddr >> 16) & 0xFF;
	buffer[3] = ipaddr >> 24;
}

/*****************************************************************************/
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

	return ~hcs; /* One's complement */
}

/*****************************************************************************/

static void calc_ics(uint8_t *buffer, int len, int hcs_pos)
{
	uint16_t *ptr_hcs = (uint16_t *)(buffer + hcs_pos);

	*ptr_hcs = 0; /* Clear checksum before calculation */
	uint16_t hcs;

	hcs = check_ics(buffer, len);
	*ptr_hcs = hcs;
}

/*****************************************************************************/

static uint32_t send_ping_wait_reply(enum ping_rai rai)
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
		header_len = ICMP_IPV4_HDR_LEN;

		total_length = ping_argv.len + header_len + icmp_hdr_len;

		buf = calloc(1, alloc_size);

		if (buf == NULL) {
			mosh_error("No RAM memory available for sending ping.");
			return -1;
		}

		buf[0] = (4 << 4) +
			 (header_len / 4);      /* Version & header length */
		/* buf[1] = 0; */ /* Type of service */
		buf[2] = total_length >> 8;     /* Total length */
		buf[3] = total_length & 0xFF;   /* Total length */
		/* buf[4..5] = 0; */ /* Identification */
		/* buf[6..7] = 0; */ /* Flags & fragment offset */
		buf[8] = 64;    /* TTL */
		buf[9] = ICMP;  /* Protocol */
		/* buf[10..11] = ICS, calculated later */

		struct sockaddr_in *sa =
			(struct sockaddr_in *)ping_argv.src->ai_addr;
		setip(buf + 12, sa->sin_addr.s_addr);   /* Source */
		sa = (struct sockaddr_in *)ping_argv.dest->ai_addr;
		setip(buf + 16, sa->sin_addr.s_addr);   /* Destination */

		calc_ics(buf, header_len, 10);

		/* ICMP header */
		data = buf + header_len;
		data[0] = ICMP_ECHO_REQ; /* Type (echo req) */
		/* data[1] = 0; */ /* Code */
		/* data[2..3] = checksum, calculated later */
		/* data[4..5] = 0; */ /* Identifier */
		/* data[6] = 0;  */ /* seqnr >> 8 */
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
		header_len = ICMP_IPV6_HDR_LEN;
		uint16_t payload_length = ping_argv.len + icmp_hdr_len;

		total_length = payload_length + header_len;
		buf = calloc(1, alloc_size);

		if (buf == NULL) {
			mosh_error("No RAM memory available for sending ping.");
			return -1;
		}

		buf[0] = (6 << 4);              /* Version & traffic class 4 bits */
		/* buf[1..3] = 0; */ /* Traffic class 4 bits & flow label */
		buf[4] = payload_length >> 8;   /* Payload length */
		buf[5] = payload_length & 0xFF; /* Payload length */
		buf[6] = ICMPV6;                /* Next header (58) */
		buf[7] = 64;                    /* Hop limit */

		struct sockaddr_in6 *sa =
			(struct sockaddr_in6 *)ping_argv.src->ai_addr;
		memcpy(buf + 8, sa->sin6_addr.s6_addr, 16); /* Source address */

		sa = (struct sockaddr_in6 *)ping_argv.dest->ai_addr;
		memcpy(buf + 24, sa->sin6_addr.s6_addr,
		       16); /* Destination address */

		/* ICMPv6 header */
		data = buf + header_len;
		data[0] = ICMP6_ECHO_REQ; /* Type (echo req) */
		/* data[1] = 0; */ /* Code */
		/* data[2..3] = checksum, calculated later */
		/* data[4..5] = 0; */ /* Identifier */
		/* data[6] = 0; */ /* seqnr >> 8 */
		data[7] = ++seqnr;

		/* Payload */
		for (int i = 0; i < ping_argv.len; i++) {
			data[i + icmp_hdr_len] = (i + seqnr) % 10 + '0';
		}

		/* ICMPv6 CRC: https://tools.ietf.org/html/rfc4443#section-2.3
		 * for IPv6 pseudo header see: https://tools.ietf.org/html/rfc2460#section-8.1
		 */
		uint32_t hcs = check_ics(buf + 8,
					 32);   /* Pseudo header: source + dest */
		hcs += check_ics(buf + 4,
				 2);            /* Pseudo header: payload length */

		uint8_t tbuf[2];

		tbuf[0] = 0;
		tbuf[1] = buf[6];

		hcs += check_ics(tbuf, 2);      /* Pseudo header: Next header */
		hcs += check_ics(data, 2);      /* ICMP: Type & Code */
		hcs += check_ics(
			data + 4,
			4 + ping_argv.len); /* ICMP: Header data + Data */

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
		mosh_error("socket() failed: (%d)", -errno);
		free(buf);
		return (uint32_t)delta_t;
	}
	if (ping_argv.cid != MOSH_ARG_NOT_SET) {
		if (ping_argv.pdn_id_for_cid != MOSH_ARG_NOT_SET) {
			/* Binding a data socket with PDN ID: */
			ret = net_utils_socket_pdn_id_set(
				fd, ping_argv.pdn_id_for_cid);
			if (ret != 0) {
				mosh_error(
					"Cannot bind socket to PDN ID %d",
					ping_argv.pdn_id_for_cid);
				goto close_end;
			}
		}
	}

#ifdef SO_RCVTIMEO
#ifdef SO_SNDTIMEO
	/* We have a blocking socket and we do not want to block for
	 * a long for sending. Thus, let's set the timeout:
	 */

	struct timeval tv = {
		.tv_sec = (ping_argv.timeout / 1000),
		.tv_usec = (ping_argv.timeout % 1000) * 1000,
	};

	if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (struct timeval *)&tv,
		       sizeof(struct timeval)) < 0) {
		mosh_error("Unable to set socket SO_SNDTIMEO, continue");
	}

	/* Just for sure, let's put the timeout for rcv as well
	 * (should not be needed for non-blocking socket):
	 */
	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv,
		       sizeof(struct timeval)) < 0) {
		mosh_error("Unable to set socket SO_RCVTIMEO, continue");
	}
#endif
#endif
	if (rai != PING_RAI_NONE) {
		/* Set RAI option ONGOING except for the last packet for which we set ONE_RESP */
		int rai_option = SO_RAI_ONGOING;

		if (rai == PING_RAI_LAST_PACKET) {
			rai_option = SO_RAI_ONE_RESP;
		}
		ret = setsockopt(fd, SOL_SOCKET, rai_option, NULL, 0);
		if (ret) {
			mosh_error("setsockopt() for RAI failed with error %d", errno);
			goto close_end;
		}
	}

	/* Include also a sending time to measured RTT: */
	start_t = k_uptime_get();
	timeout = ping_argv.timeout;

	ret = send(fd, buf, total_length, 0);
	if (ret <= 0) {
		mosh_error("send() failed: (%d)", -errno);
		goto close_end;
	}

	/* Set a socket as non-blocking for receiving the response so that we
	 * can control a pinging timeout for receive better:
	 */
	int flags = fcntl(fd, F_GETFL, 0);

	fcntl(fd, F_SETFL, flags | (int)O_NONBLOCK);

	/* Now after send(), calculate how much there's still time left */
	delta_t = k_uptime_delta(&start_t);
	timeout = ping_argv.timeout - (int32_t)delta_t;

wait_for_data:
	fds[0].fd = fd;
	fds[0].events = POLLIN;

	do {
		if (timeout <= 0) {
			mosh_print(
				"Pinging %s results: no ping response in given timeout %d msec",
				ping_argv.target_name, ping_argv.timeout);
			delta_t = 0;
			goto close_end;
		}

		ret = poll(fds, 1, timeout);
		if (ret == 0) {
			mosh_print(
				"Pinging %s results: no response in given timeout %d msec",
				ping_argv.target_name, ping_argv.timeout);
			delta_t = 0;
			goto close_end;
		} else if (ret < 0) {
			mosh_error("poll() failed: (%d) (%d)", -errno, ret);
			delta_t = 0;
			goto close_end;
		}

		len = recv(fd, buf, alloc_size, 0);

		/* Calculate again, how much there's still time left */
		delta_t = k_uptime_delta(&start_t);
		timeout = ping_argv.timeout - (int32_t)delta_t;

		if (len <= 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				mosh_error(
					"recv() failed with errno (%d) and return value (%d)",
					-errno, len);
				delta_t = 0;
				goto close_end;
			}
		}
		if (len < header_len) {
			/* Data length error, ignore "silently" */
			mosh_error("recv() wrong data (%d)", len);
		}

		if ((rep == ICMP_ECHO_REP && buf[IP_PROTOCOL_POS] == ICMP) ||
		    (rep == ICMP6_ECHO_REP &&
		     buf[IP_NEXT_HEADER_POS] == ICMPV6)) {
			/* if not ipv4/ipv6 echo reply, ignore silently,
			 * otherwise break the loop and go to parse the response
			 */
			break;
		}
	} while (true);

	if (rai == PING_RAI_LAST_PACKET) {
		/* Set RAI option NO_DATA after last response has been received */
		ret = setsockopt(fd, SOL_SOCKET, SO_RAI_NO_DATA, NULL, 0);
		if (ret) {
			mosh_error("setsockopt() for SO_RAI_NO_DATA failed with error %d",
				    errno);
			goto close_end;
		}
	}

	if (rep == ICMP_ECHO_REP) {
		/* Check ICMP HCS */
		int hcs = check_ics(data, len - header_len);

		if (hcs != 0) {
			mosh_error("IPv4 HCS error, hcs: %d, len: %d\r\n", hcs, len);
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
		hcs += check_ics(data + 4,
				 len - header_len - 4); /* Header data + Data */

		while (hcs > 0xFFFF)
			hcs = (hcs & 0xFFFF) + (hcs >> 16);

		int plhcs = data[2] + (data[3] << 8);

		if (plhcs != hcs) {
			mosh_error("IPv6 HCS error: 0x%x 0x%x\r\n", plhcs, hcs);
			delta_t = 0;
			goto close_end;
		}
		/* Raw socket payload length */
		pllen = (buf[4] << 8) + buf[5] +
			header_len; /* Payload length - hdr */
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
		mosh_error("Expected length %d, got %d", len, pllen);
		delta_t = 0;
		goto close_end;
	}

	/* Result */
	mosh_print(
		"Pinging %s results: time=%d.%03dsecs, payload sent: %d, payload received %d",
		ping_argv.target_name, (uint32_t)(delta_t) / 1000,
		(uint32_t)(delta_t) % 1000, ping_argv.len, dpllen);

close_end:
	(void)close(fd);
	free(buf);
	return (uint32_t)delta_t;
}

/*****************************************************************************/

static void icmp_ping_tasks_execute(void)
{
	struct addrinfo *si = ping_argv.src;
	struct addrinfo *di = ping_argv.dest;
	uint32_t sum = 0;
	uint32_t count = 0;
	uint32_t rtt_min = 0xFFFFFFFF;
	uint32_t rtt_max = 0;
	int set, res;
	enum ping_rai rai;
	uint32_t ping_t;

	for (int i = 0; i < ping_argv.count; i++) {
		rai = PING_RAI_NONE;
		if (ping_argv.rai) {
			if (i == ping_argv.count - 1) {
				/* For last packet */
				rai = PING_RAI_LAST_PACKET;
			} else {
				rai = PING_RAI_ONGOING;
			}
		}
		ping_t = send_ping_wait_reply(rai);

		k_poll_signal_check(&mosh_signal, &set, &res);
		if (set && res == MOSH_SIGNAL_KILL) {
			k_poll_signal_reset(&mosh_signal);
			mosh_error("KILL signal received - exiting");
			break;
		}

		if (ping_t > 0) {
			count++;
			sum += ping_t;
			rtt_max = MAX(rtt_max, ping_t);
			rtt_min = MIN(rtt_min, ping_t);
		}
		k_sleep(K_MSEC(ping_argv.interval));
	}

	freeaddrinfo(si);
	freeaddrinfo(di);

	uint32_t lost = ping_argv.count - count;

	mosh_print("Ping statistics for %s:", ping_argv.target_name);
	mosh_print(
		"    Packets: Sent = %d, Received = %d, Lost = %d (%d%% loss)",
		ping_argv.count,
		count,
		lost,
		lost * 100 / ping_argv.count);

	if (count > 0) {
		mosh_print("Approximate round trip times in milli-seconds:");
		mosh_print(
			"    Minimum = %dms, Maximum = %dms, Average = %dms",
			rtt_min, rtt_max, sum / count);
	}
	mosh_print("Pinging DONE");
}

/*****************************************************************************/

int icmp_ping_start(struct icmp_ping_shell_cmd_argv *ping_args)
{
	int st = -1;
	struct addrinfo *res;
	char src_ipv_addr[NET_IPV6_ADDR_LEN];
	char portstr[6] = { 0 };
	char *service = NULL;
	bool set_flags = false;

	/* Copy args in local storage here: */
	memcpy(&ping_argv, ping_args, sizeof(struct icmp_ping_shell_cmd_argv));

	mosh_print("Initiating ping to: %s", ping_argv.target_name);

	/* Sets getaddrinfo hints by using current host address(es): */
	struct addrinfo hints = {
		.ai_family = AF_INET,
	};

	if (ping_argv.cid != MOSH_ARG_NOT_SET) {
		if (ping_argv.pdn_id_for_cid != MOSH_ARG_NOT_SET) {
			snprintf(portstr, 6, "%d", ping_argv.pdn_id_for_cid);
			set_flags = true;
			service = portstr;
			hints.ai_flags = AI_PDNSERV;
		}
	}

	inet_ntop(AF_INET, &(ping_argv.current_addr4), src_ipv_addr,
		  sizeof(src_ipv_addr));
	if (ping_argv.current_pdp_type == PDP_TYPE_IP4V6) {
		if (ping_argv.force_ipv6) {
			hints.ai_family = AF_INET6;
			inet_ntop(AF_INET6, &(ping_argv.current_addr6),
				  src_ipv_addr, sizeof(src_ipv_addr));
		}
	}
	if (ping_argv.current_pdp_type == PDP_TYPE_IPV6) {
		hints.ai_family = AF_INET6;
		inet_ntop(AF_INET6, &(ping_argv.current_addr6), src_ipv_addr,
			  sizeof(src_ipv_addr));
	}

	st = getaddrinfo(src_ipv_addr, service, &hints, &res);
	if (st != 0) {
		mosh_error("getaddrinfo(src) error: %d", st);
		return -st;
	}
	ping_argv.src = res;

	/* Get destination */
	res = NULL;

	st = getaddrinfo(ping_argv.target_name, service, &hints, &res);

	if (st != 0) {
		mosh_error("getaddrinfo(dest) error: %d", st);
		mosh_error("Cannot resolve remote host\r\n");
		freeaddrinfo(ping_argv.src);
		return -st;
	}
	ping_argv.dest = res;

	if (ping_argv.src->ai_family != ping_argv.dest->ai_family) {
		mosh_error("Source/Destination address family error");
		freeaddrinfo(ping_argv.dest);
		freeaddrinfo(ping_argv.src);
		return -1;
	}

	struct sockaddr *sa;

	sa = ping_argv.src->ai_addr;
	mosh_print("Source IP addr: %s", net_utils_sckt_addr_ntop(sa));
	sa = ping_argv.dest->ai_addr;
	mosh_print("Destination IP addr: %s", net_utils_sckt_addr_ntop(sa));

	/* Now we can check the max payload len for IPv6: */
	uint32_t ipv6_max_payload_len = ping_argv.mtu - ICMP_IPV6_HDR_LEN - ICMP_HDR_LEN;

	if (ping_argv.src->ai_family == AF_INET6 &&
	    ping_argv.len > ipv6_max_payload_len) {
		mosh_warn(
			"Payload size exceeds the link limits: MTU %d - headers %d = %d ",
			ping_argv.mtu, (ICMP_IPV4_HDR_LEN - ICMP_HDR_LEN),
			ipv6_max_payload_len);
		/* Continue still: */
	}

	icmp_ping_tasks_execute();
	return 0;
}

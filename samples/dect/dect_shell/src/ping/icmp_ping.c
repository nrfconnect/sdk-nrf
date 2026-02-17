/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <zephyr/kernel.h>

#include <zephyr/shell/shell.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/ethernet.h> /* just for ETH_P_ALL */
#include <zephyr/net/socket.h>

#include <fcntl.h>

#include "desh_print.h"
#include "desh_defines.h"

#include "icmp_ping.h"

/* Protocol field: */
#define ICMP   1
#define ICMP6  6
#define ICMPV6 58

/* Next header: */
#define IP_NEXT_HEADER_POS 6
#define IP_PROTOCOL_POS	   9

#define ICMP_ECHO_REP  0
#define ICMP_ECHO_REQ  8
#define ICMP6_ECHO_REQ 128
#define ICMP6_ECHO_REP 129

extern struct k_poll_signal desh_signal;

/* When IPv6 connection status was updated last time:
 * used to detect the cases when we need to re-read
 * iface addressing to update ping socket bindings in case of possible
 * changes.
 */
static int64_t ipv6_connected_status_updated_uptime;

static bool icmp_ping_current_conn_info_set(struct icmp_ping_shell_cmd_argv *ping_args,
					    struct icmp_ping_shell_cmd_argv *ping_argv);

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

static uint32_t send_ping_wait_reply(struct icmp_ping_shell_cmd_argv *ping_args)
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
	struct zsock_addrinfo *si = ping_args->src;
	const int alloc_size = ICMP_DEFAULT_LINK_MTU;
	struct zsock_pollfd fds[1];
	int dpllen, pllen, len;
	int fd;
	int plseqnr;
	int ret;
	const uint16_t icmp_hdr_len = ICMP_HDR_LEN;

	if (si->ai_family == AF_INET) {
		/* Generate IPv4 ICMP EchoReq */

		/* Ping header */
		header_len = ICMP_IPV4_HDR_LEN;

		total_length = ping_args->len + header_len + icmp_hdr_len;

		buf = calloc(1, alloc_size);

		if (buf == NULL) {
			desh_error("No RAM memory available for sending ping.");
			return -1;
		}

		buf[0] = (4 << 4) + (header_len / 4); /* Version & header length */
		/* buf[1] = 0; */		      /* Type of service */
		buf[2] = total_length >> 8;	      /* Total length */
		buf[3] = total_length & 0xFF;	      /* Total length */
		/* buf[4..5] = 0; */		      /* Identification */
		/* buf[6..7] = 0; */		      /* Flags & fragment offset */
		buf[8] = 64;			      /* TTL */
		buf[9] = ICMP;			      /* Protocol */
		/* buf[10..11] = ICS, calculated later */

		struct net_sockaddr_in *sa = (struct net_sockaddr_in *)ping_args->src->ai_addr;

		setip(buf + 12, sa->sin_addr.s_addr); /* Source */
		sa = (struct net_sockaddr_in *)ping_args->dest->ai_addr;
		setip(buf + 16, sa->sin_addr.s_addr); /* Destination */

		calc_ics(buf, header_len, 10);

		/* ICMP header */
		data = buf + header_len;
		data[0] = ICMP_ECHO_REQ; /* Type (echo req) */
		/* data[1] = 0; */	 /* Code */
		/* data[2..3] = checksum, calculated later */
		/* data[4..5] = 0; */ /* Identifier */
		/* data[6] = 0;  */   /* seqnr >> 8 */
		if (seqnr == UINT8_MAX) {
			seqnr = 0;
		}
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
		uint16_t payload_length = ping_args->len + icmp_hdr_len;

		total_length = payload_length + header_len;
		buf = calloc(1, alloc_size);

		if (buf == NULL) {
			desh_error("No RAM memory available for sending ping.");
			return -1;
		}

		buf[0] = (6 << 4);		/* Version & traffic class 4 bits */
		/* buf[1..3] = 0; */		/* Traffic class 4 bits & flow label */
		buf[4] = payload_length >> 8;	/* Payload length */
		buf[5] = payload_length & 0xFF; /* Payload length */
		buf[6] = ICMPV6;		/* Next header (58) */
		buf[7] = 64;			/* Hop limit */

		struct net_sockaddr_in6 *sa = (struct net_sockaddr_in6 *)ping_args->src->ai_addr;

		memcpy(buf + 8, sa->sin6_addr.s6_addr, 16); /* Source address */

		sa = (struct net_sockaddr_in6 *)ping_args->dest->ai_addr;
		memcpy(buf + 24, sa->sin6_addr.s6_addr, 16); /* Destination address */

		/* ICMPv6 header */
		data = buf + header_len;
		data[0] = ICMP6_ECHO_REQ; /* Type (echo req) */
		/* data[1] = 0; */	  /* Code */
		/* data[2..3] = checksum, calculated later */
		/* data[4..5] = 0; */ /* Identifier */
		/* data[6] = 0; */    /* seqnr >> 8 */
		data[7] = ++seqnr;

		/* Payload */
		for (int i = 0; i < ping_args->len; i++) {
			data[i + icmp_hdr_len] = (i + seqnr) % 10 + '0';
		}

		/* ICMPv6 CRC: https://tools.ietf.org/html/rfc4443#section-2.3
		 * for IPv6 pseudo header see: https://tools.ietf.org/html/rfc2460#section-8.1
		 */
		uint32_t hcs = check_ics(buf + 8, 32); /* Pseudo header: source + dest */

		hcs += check_ics(buf + 4, 2);	       /* Pseudo header: payload length */

		uint8_t tbuf[2];

		tbuf[0] = 0;
		tbuf[1] = buf[6];

		hcs += check_ics(tbuf, 2);			/* Pseudo header: Next header */
		hcs += check_ics(data, 2);			/* ICMP: Type & Code */
		hcs += check_ics(data + 4, 4 + ping_args->len); /* ICMP: Header data + Data */

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

	fd = zsock_socket(si->ai_family, SOCK_RAW, htons(IPPROTO_IP));
	if (fd < 0) {
		desh_error("socket() failed: (%d)", -errno);
		free(buf);
		return (uint32_t)delta_t;
	}
	/* Bind the socket to selected source net_if addr */
	net_socklen_t addrlen;

	if (si->ai_family == AF_INET) {
		/* IPv4 */
		struct net_sockaddr_in bind_addr;

		bind_addr.sin_family = AF_INET;
		bind_addr.sin_addr = ping_args->current_addr4;
		addrlen = sizeof(struct net_sockaddr_in);
		ret = zsock_bind(fd, (struct net_sockaddr *)&bind_addr, addrlen);
	} else {
		struct net_sockaddr_in6 bind_addr;

		/* IPv6 */
		bind_addr.sin6_family = AF_INET6;
		bind_addr.sin6_addr = ping_args->current_addr6;
		addrlen = sizeof(struct net_sockaddr_in6);
		ret = zsock_bind(fd, (struct net_sockaddr *)&bind_addr, addrlen);
	}

	if (ret < 0) {
		desh_error("Failed to bind a socket : %d", errno);
		(void)zsock_close(fd);
		free(buf);
		return ret;
	}
	/* Include also a sending time to measured RTT: */
	start_t = k_uptime_get();
	timeout = ping_args->timeout;

	ret = zsock_send(fd, buf, total_length, 0);
	if (ret <= 0) {
		desh_error("send() failed: (%d)", -errno);
		goto close_end;
	}

	/* Set a socket as non-blocking for receiving the response so that we
	 * can control a pinging timeout for receive better:
	 */
	int flags = zsock_fcntl(fd, F_GETFL, 0);

	zsock_fcntl(fd, F_SETFL, flags | (int)O_NONBLOCK);

	/* Now after send(), calculate how much there's still time left */
	delta_t = k_uptime_delta(&start_t);
	timeout = ping_args->timeout - (int32_t)delta_t;

wait_for_data:
	fds[0].fd = fd;
	fds[0].events = ZSOCK_POLLIN;
	fds[0].revents = 0;

	do {
		if (timeout <= 0) {
			desh_print("Pinging %s results: no ping response in given timeout %d msec "
				   "- all time used for sending",
				   ping_args->target_name, ping_args->timeout);
			delta_t = 0;
			goto close_end;
		}
		ret = zsock_poll(fds, 1, timeout);
		if (ret == 0) {
			desh_print("Pinging %s results: no response in given timeout %d msec "
				   "(timeout after TX %d)",
				   ping_args->target_name, ping_args->timeout, timeout);
			delta_t = 0;
			goto close_end;
		} else if (ret < 0) {
			desh_error("poll() failed: (%d) (%d)", -errno, ret);
			delta_t = 0;
			goto close_end;
		}
		len = zsock_recv(fd, buf, alloc_size, 0);

		/* Calculate again, how much there's still time left */
		delta_t = k_uptime_delta(&start_t);
		timeout = ping_args->timeout - (int32_t)delta_t;

		if (len <= 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				desh_error("recv() failed with errno (%d) and return value (%d)",
					   -errno, len);
				delta_t = 0;
				goto close_end;
			}
		}
		if (len < header_len) {
			/* Data length error, ignore "silently" */
			desh_error("recv() wrong data (%d)", len);
		}

		if ((rep == ICMP_ECHO_REP && buf[IP_PROTOCOL_POS] == ICMP) ||
		    (rep == ICMP6_ECHO_REP && buf[IP_NEXT_HEADER_POS] == ICMPV6)) {
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
			desh_error("IPv4 HCS error, hcs: %d, len: %d\r\n", hcs, len);
			delta_t = 0;
			goto close_end;
		}
		pllen = (buf[2] << 8) + buf[3]; /* Raw socket payload length */
	} else {
		/* Check ICMP6 CRC */
		uint32_t hcs = check_ics(buf + 8, 32); /* Pseudo header source + dest */
		uint8_t tbuf[2];

		hcs += check_ics(buf + 4, 2); /* Pseudo header packet length */

		tbuf[0] = 0;
		tbuf[1] = buf[6];
		hcs += check_ics(tbuf, 2);			  /* Pseudo header Next header */
		hcs += check_ics(data, 2);			  /* Type & Code */
		hcs += check_ics(data + 4, len - header_len - 4); /* Header data + Data */

		while (hcs > 0xFFFF) {
			hcs = (hcs & 0xFFFF) + (hcs >> 16);
		}
		int plhcs = data[2] + (data[3] << 8);

		if (plhcs != hcs) {
			desh_error("IPv6 HCS error: 0x%x 0x%x\r\n", plhcs, hcs);
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
		desh_error("Expected length %d, got %d", len, pllen);
		delta_t = 0;
		goto close_end;
	}

	/* Result */
	desh_print("Pinging %s results: time=%d.%03dsecs, payload sent: %d, payload received %d",
		   ping_args->target_name, (uint32_t)(delta_t) / 1000, (uint32_t)(delta_t) % 1000,
		   ping_args->len, dpllen);

close_end:
	(void)zsock_close(fd);
	free(buf);
	return (uint32_t)delta_t;
}

/*****************************************************************************/

int icmp_ping_start(struct icmp_ping_shell_cmd_argv *ping_args)
{
	struct icmp_ping_shell_cmd_argv current_ping_args;
	uint32_t sum = 0;
	uint32_t count = 0;
	uint32_t rtt_min = 0xFFFFFFFF;
	uint32_t rtt_max = 0;
	int set, res;
	uint32_t ping_t;
	int ret = 0;

	/* All good for ping_args, get the current connection info and start the ping */
	if (!icmp_ping_current_conn_info_set(ping_args, &current_ping_args)) {
		return -1;
	}

	for (int i = 0; i < current_ping_args.count; i++) {
		if (current_ping_args.conn_info_read_uptime <
			ipv6_connected_status_updated_uptime) {
			/* Connection status has changed since last read,
			 * re-read the connection info:
			 */
			desh_print("Re-reading conn info...");
			if (current_ping_args.dest) {
				zsock_freeaddrinfo(current_ping_args.dest);
			}
			current_ping_args.dest = NULL;
			if (current_ping_args.src) {
				zsock_freeaddrinfo(current_ping_args.src);
			}
			current_ping_args.src = NULL;
			if (!icmp_ping_current_conn_info_set(ping_args, &current_ping_args)) {
				desh_warn("Failed to re-read conn info - continue");
				k_sleep(K_MSEC(current_ping_args.interval));
				continue;
			}
		}
		ping_t = send_ping_wait_reply(&current_ping_args);

		k_poll_signal_check(current_ping_args.kill_signal, &set, &res);
		if (set) {
			k_poll_signal_reset(current_ping_args.kill_signal);
			desh_error("KILL signal received - exiting");
			break;
		}

		if (ping_t > 0) {
			count++;
			sum += ping_t;
			rtt_max = MAX(rtt_max, ping_t);
			rtt_min = MIN(rtt_min, ping_t);
		}
		k_sleep(K_MSEC(current_ping_args.interval));
	}

	uint32_t lost = current_ping_args.count - count;

	desh_print("Ping statistics for %s:", current_ping_args.target_name);
	desh_print("    Packets: Sent = %d, Received = %d, Lost = %d (%d%% loss)",
		   current_ping_args.count, count, lost, lost * 100 / current_ping_args.count);

	if (count > 0) {
		desh_print("Approximate round trip times in milli-seconds:");
		desh_print("    Minimum = %dms, Maximum = %dms, Average = %dms", rtt_min, rtt_max,
			   sum / count);
	}

	zsock_freeaddrinfo(current_ping_args.src);
	current_ping_args.src = NULL;
	zsock_freeaddrinfo(current_ping_args.dest);
	current_ping_args.dest = NULL;

	desh_print("Pinging DONE");

	return ret;
}

/*****************************************************************************/

void icmp_ping_cmd_defaults_set(struct icmp_ping_shell_cmd_argv *ping_args)
{
	memset(ping_args, 0, sizeof(struct icmp_ping_shell_cmd_argv));
	/* ping_args->dest = NULL; */
	ping_args->kill_signal = &desh_signal;
	ping_args->count = ICMP_PARAM_COUNT_DEFAULT;
	ping_args->interval = ICMP_PARAM_INTERVAL_DEFAULT;
	ping_args->timeout = ICMP_PARAM_TIMEOUT_DEFAULT;
	ping_args->len = ICMP_PARAM_LENGTH_DEFAULT;

	ping_args->ping_iface = net_if_get_by_index(
		net_if_get_by_name(CONFIG_DECT_MDM_DEVICE_NAME));
	if (!ping_args->ping_iface) {
		desh_error("%s: Interface %s not found", (__func__),
			CONFIG_DECT_MDM_DEVICE_NAME);
	}
}

/*****************************************************************************/

/**
 * @brief Read and set current connection parameters.
 *
 * @param ping_args In: Current ping parameters.
 * @param ping_argv Out: Ping parameters with updated connection parameters.
 *
 */
char *net_utils_sckt_addr_ntop(const struct net_sockaddr *addr)
{
	static char buf[NET_IPV6_ADDR_LEN];

	if (addr->sa_family == AF_INET6) {
		return zsock_inet_ntop(AF_INET6, &net_sin6(addr)->sin6_addr, buf, sizeof(buf));
	}

	if (addr->sa_family == AF_INET) {
		return zsock_inet_ntop(AF_INET, &net_sin(addr)->sin_addr, buf, sizeof(buf));
	}

	strcpy(buf, "Unknown AF");
	return buf;
}

static bool icmp_ping_current_conn_info_set(struct icmp_ping_shell_cmd_argv *ping_args,
					    struct icmp_ping_shell_cmd_argv *ping_argv)
{
	int st = -1;
	struct zsock_addrinfo *res;
	char src_ipv_addr[NET_IPV6_ADDR_LEN];
	char *service = NULL;
	const struct in6_addr *src;

	/* Now we can check the max payload len vs link MTU (IPv6 check later): */
	uint32_t ipv4_max_payload_len = ping_args->mtu - ICMP_IPV4_HDR_LEN - ICMP_HDR_LEN;

	if (!ping_args->force_ipv6 && ping_args->len > ipv4_max_payload_len) {
		desh_warn("Payload size exceeds the link limits: MTU %d - headers %d = %d ",
			  ping_args->force_ipv6, (ICMP_IPV4_HDR_LEN - ICMP_HDR_LEN),
			  ipv4_max_payload_len);
		/* Execute ping anyway */
	}

	/* Finally copy args in local storage here and start pinging */
	memcpy(ping_argv, ping_args, sizeof(struct icmp_ping_shell_cmd_argv));

	desh_print("Initiating ping to: %s", ping_argv->target_name);

	/* Sets getaddrinfo hints by using current host address(es): */
	struct zsock_addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_flags = 0;

	/* Get destination */
	res = NULL;

	st = zsock_getaddrinfo(ping_argv->target_name, service, &hints, &res);

	if (st != 0) {
		desh_error("getaddrinfo(dest) error: %d", st);
		desh_error("Cannot resolve remote host\r\n");
		zsock_freeaddrinfo(ping_argv->src);
		ping_argv->src = NULL;
		goto exit;
	}
	ping_argv->dest = res;

	struct net_sockaddr *dst_sa;
	struct net_sockaddr *src_sa;

	dst_sa = ping_argv->dest->ai_addr;

	src = net_if_ipv6_select_src_addr(ping_argv->ping_iface, &net_sin6(dst_sa)->sin6_addr);
	if (!src) {
		desh_error("No source address found for destination %s",
			   net_utils_sckt_addr_ntop(dst_sa));
		goto exit;
	}
	memcpy(&(ping_argv->current_addr6), src, sizeof(struct in6_addr));
	zsock_inet_ntop(AF_INET6, &(ping_argv->current_addr6), src_ipv_addr, sizeof(src_ipv_addr));

	st = zsock_getaddrinfo(src_ipv_addr, service, &hints, &res);
	if (st != 0) {
		desh_error("%s: getaddrinfo(src) error: %d", (__func__), st);
		goto exit;
	}
	ping_argv->src = res;
	src_sa = ping_argv->src->ai_addr;

	if (ping_argv->src->ai_family != ping_argv->dest->ai_family) {
		desh_error("Source/Destination address family error");
		zsock_freeaddrinfo(ping_argv->dest);
		ping_argv->dest = NULL;
		zsock_freeaddrinfo(ping_argv->src);
		ping_argv->src = NULL;
		goto exit;
	}

	desh_print("Source IP addr: %s", net_utils_sckt_addr_ntop(src_sa));
	desh_print("Destination IP addr: %s", net_utils_sckt_addr_ntop(dst_sa));

	/* Now we can check the max payload len for IPv6: */
	uint32_t ipv6_max_payload_len = ping_argv->mtu - ICMP_IPV6_HDR_LEN - ICMP_HDR_LEN;

	if (ping_argv->src->ai_family == AF_INET6 && ping_argv->len > ipv6_max_payload_len) {
		desh_warn("Payload size exceeds the link limits: MTU %d - headers %d = %d ",
			  ping_argv->mtu, (ICMP_IPV4_HDR_LEN - ICMP_HDR_LEN), ipv6_max_payload_len);
		/* Continue still: */
	}
	ping_argv->conn_info_read_uptime = k_uptime_get();
	return true;
exit:
	return false;
}
#if defined(CONFIG_NET_CONNECTION_MANAGER)
#define L4_EVENT_MASK (NET_EVENT_L4_IPV6_CONNECTED)
static struct net_mgmt_event_callback l4_cb;
static void l4_event_handler(struct net_mgmt_event_callback *cb, uint64_t event,
			     struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	switch (event) {
	case NET_EVENT_L4_IPV6_CONNECTED:
		ipv6_connected_status_updated_uptime = k_uptime_get();
		break;
	default:
		break;
	}
}
#endif

static int icmp_ping_init(void)
{
	ipv6_connected_status_updated_uptime = k_uptime_get();
#if defined(CONFIG_NET_CONNECTION_MANAGER)
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);
#endif
	return 0;
}

SYS_INIT(icmp_ping_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

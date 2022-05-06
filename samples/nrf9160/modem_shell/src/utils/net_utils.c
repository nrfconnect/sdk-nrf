/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/shell/shell.h>
#include <nrf_socket.h>

#include "net_utils.h"
#include "mosh_print.h"

int net_utils_socket_pdn_id_set(int fd, uint32_t pdn_id)
{
	int ret;
	size_t len;
	struct ifreq ifr = { 0 };

	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "pdn%d", pdn_id);
	len = strlen(ifr.ifr_name);

	ret = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, len);
	if (ret < 0) {
		mosh_error(
			"Failed to bind socket with PDN ID %d, error: %d, %s",
			pdn_id, ret, strerror(ret));
		return -EINVAL;
	}

	return 0;
}

char *net_utils_sckt_addr_ntop(const struct sockaddr *addr)
{
	static char buf[NET_IPV6_ADDR_LEN];

	if (addr->sa_family == AF_INET6) {
		return inet_ntop(AF_INET6, &net_sin6(addr)->sin6_addr, buf,
				 sizeof(buf));
	}

	if (addr->sa_family == AF_INET) {
		return inet_ntop(AF_INET, &net_sin(addr)->sin_addr, buf,
				 sizeof(buf));
	}

	strcpy(buf, "Unknown AF");
	return buf;
}

int net_utils_sa_family_from_ip_string(const char *src)
{
	char buf[INET6_ADDRSTRLEN];

	if (inet_pton(AF_INET, src, buf)) {
		return AF_INET;
	} else if (inet_pton(AF_INET6, src, buf)) {
		return AF_INET6;
	}
	return -1;
}

bool net_utils_ip_string_is_valid(const char *src)
{
	struct nrf_in6_addr in6_addr;

	/* Use nrf_inet_pton() because this has full IP address validation. */
	return (nrf_inet_pton(NRF_AF_INET, src, &in6_addr) == 1 ||
		nrf_inet_pton(NRF_AF_INET6, src, &in6_addr) == 1);
}

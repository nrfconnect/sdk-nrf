/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <nrf_modem_at.h>
#include <string.h>

#ifdef CONFIG_POSIX_API
#include <arpa/inet.h>
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ip_addr_helper, CONFIG_NRF_MODEM_LIB_NET_IF_LOG_LEVEL);

/* Structure that keeps track of the current IP addresses in use. */
static struct ip_addr_helper_data {
	struct in_addr ipv4_addr_current;
	struct in6_addr ipv6_addr_current;
	bool ipv4_added;
	bool ipv6_added;
} ctx;

/* @brief Function that obtains either the IPv4 or IPv6 address associated with
 *	  the default PDP context 0.
 *
 * @details Only a single IP family address is written to per call.
 *	    It's expected that only one of the input arguments points to a valid buffer when calling
 *	    this function. The other buffer must be set to NULL.
 *
 * @param [out] addr4 Pointer to a NULL terminated buffer that the current IPv4 address
 *		will be written to.
 * @param [out] addr6 Pointer to a NULL terminated buffer that the current IPv6 address
 *		will be written to.
 *
 * @returns 0 on success, negative value on failure.
 */
static int ip_addr_get(char *addr4, char *addr6)
{
	int ret;
	char tmp[sizeof(struct in6_addr)];
	char addr1[NET_IPV6_ADDR_LEN] = { 0 };
	char addr2[NET_IPV6_ADDR_LEN] = { 0 };

	/* Parse +CGPADDR: <cid>,<PDP_addr_1>,<PDP_addr_2>
	 * PDN type "IP": PDP_addr_1 is <IPv4>, max 16(INET_ADDRSTRLEN), '.' and digits
	 * PDN type "IPV6": PDP_addr_1 is <IPv6>, max 46(INET6_ADDRSTRLEN),':', digits, 'A'~'F'
	 * PDN type "IPV4V6": <IPv4>,<IPv6> or <IPV4> or <IPv6>
	 */
	ret = nrf_modem_at_scanf("AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"",
				 addr1, addr2);
	if (ret <= 0) {
		return -EFAULT;
	}

	/* inet_pton() is used to check the type of returned address(es). */
	if ((addr4 != NULL) && (inet_pton(AF_INET, addr1, tmp) == 1)) {
		strcpy(addr4, addr1);
		return strlen(addr4);
	}

	if ((addr6 != NULL) && (inet_pton(AF_INET6, addr1, tmp) == 1)) {
		strcpy(addr6, addr1);
		return strlen(addr6);
	}

	/* If two addresses are provided, the IPv6 address is in the second address argument. */
	if ((addr6 != NULL) && (ret > 1) && (inet_pton(AF_INET6, addr2, tmp) == 1)) {
		strcpy(addr6, addr2);
		return strlen(addr6);
	}

	return -ENODATA;
}

int lte_ipv4_addr_add(const struct net_if *iface)
{
	int len;
	char ipv4_addr[NET_IPV4_ADDR_LEN] = { 0 };
	struct sockaddr addr = { 0 };
	struct net_if_addr *ifaddr = NULL;

	if (iface == NULL) {
		return -EINVAL;
	}

	len = ip_addr_get(ipv4_addr, NULL);
	if (len < 0) {
		return len;
	}

	if (!net_ipaddr_parse(ipv4_addr, len, &addr)) {
		return -EFAULT;
	}

	ifaddr = net_if_ipv4_addr_add((struct net_if *)iface,
				      &net_sin(&addr)->sin_addr,
				      NET_ADDR_MANUAL,
				      0);
	if (!ifaddr) {
		return -ENODEV;
	}

	ctx.ipv4_addr_current = net_sin(&addr)->sin_addr;
	ctx.ipv4_added = true;

	return 0;
}

int lte_ipv6_addr_add(const struct net_if *iface)
{
	int len;
	char ipv6_addr[NET_IPV6_ADDR_LEN] = { 0 };
	struct sockaddr addr = { 0 };
	struct net_if_addr *ifaddr = NULL;

	if (iface == NULL) {
		return -EINVAL;
	}

	len = ip_addr_get(NULL, ipv6_addr);
	if (len < 0) {
		return len;
	}

	if (!net_ipaddr_parse(ipv6_addr, len, &addr)) {
		return -EFAULT;
	}

	ifaddr = net_if_ipv6_addr_add((struct net_if *)iface,
				      &net_sin6(&addr)->sin6_addr,
				      NET_ADDR_MANUAL,
				      0);
	if (!ifaddr) {
		return -ENODEV;
	}

	ctx.ipv6_addr_current = net_sin6(&addr)->sin6_addr;
	ctx.ipv6_added = true;

	return 0;
}

int lte_ipv4_addr_remove(const struct net_if *iface)
{
	if (iface == NULL) {
		return -EINVAL;
	}

	if (!ctx.ipv4_added) {
		LOG_DBG("No IPv4 address to remove");
		return 0;
	}

	if (!net_if_ipv4_addr_rm((struct net_if *)iface, &ctx.ipv4_addr_current)) {
		return -EFAULT;
	}

	ctx.ipv4_added = false;

	return 0;
}

int lte_ipv6_addr_remove(const struct net_if *iface)
{
	if (iface == NULL) {
		return -EINVAL;
	}

	if (!ctx.ipv6_added) {
		LOG_DBG("No IPv6 address to remove");
		return 0;
	}

	if (!net_if_ipv6_addr_rm((struct net_if *)iface, &ctx.ipv6_addr_current)) {
		return -EFAULT;
	}

	ctx.ipv6_added = false;

	return 0;
}

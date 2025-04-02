/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/util.h>
#include <nrf_socket.h>

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

/* The DNS address is just a string, we don't know if it's IPv4 or IPv6 address.
 * Check with inet_pton() to determine the right address family.
 */
void dns_fallback_set(void)
{
	int err;
	int valid;
	struct nrf_in_addr in_addr;
	struct nrf_in6_addr in6_addr;

	struct {
		void *addr;
		nrf_sa_family_t fam;
		uint16_t size;
	} dns[] = {
		{.addr = &in_addr, .fam = NRF_AF_INET, .size = sizeof(in_addr)},
		{.addr = &in6_addr, .fam = NRF_AF_INET6, .size = sizeof(in6_addr)},
	};

	for (size_t i = 0; i < ARRAY_SIZE(dns); i++) {
		valid = nrf_inet_pton(dns[i].fam, CONFIG_LTE_LC_DNS_FALLBACK_ADDRESS, dns[i].addr);
		if (!valid) {
			/* try next */
			continue;
		}
		err = nrf_setdnsaddr(dns[i].fam, dns[i].addr, dns[i].size);
		if (err) {
			LOG_ERR("Failed to set fallback DNS address, err %d", errno);
			return;
		}
		/* no need to try the next address family */
		break;
	}

	if (!valid) {
		LOG_ERR("%s is not a valid DNS server address", CONFIG_LTE_LC_DNS_FALLBACK_ADDRESS);
		return;
	}

	LOG_DBG("Fallback DNS address successfully set to %s", CONFIG_LTE_LC_DNS_FALLBACK_ADDRESS);
}

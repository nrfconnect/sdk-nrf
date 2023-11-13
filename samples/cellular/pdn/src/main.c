/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <modem/pdn.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <nrf_socket.h>

#define SIN(addr)  ((struct nrf_sockaddr_in *)(addr))
#define SIN6(addr) ((struct nrf_sockaddr_in6 *)(addr))

static const char * const fam_str[] = {
	[PDN_FAM_IPV4V6] = "IPV4V6",
	[PDN_FAM_IPV6] = "IPV6",
	[PDN_FAM_IPV4] = "IPV4",
};

static const char * const event_str[] = {
	[PDN_EVENT_CNEC_ESM] = "ESM",
	[PDN_EVENT_ACTIVATED] = "activated",
	[PDN_EVENT_DEACTIVATED] = "deactivated",
	[PDN_EVENT_IPV6_UP] = "IPv6 up",
	[PDN_EVENT_IPV6_DOWN] = "IPv6 down",
	[PDN_EVENT_NETWORK_DETACH] = "network detach",
};

static void snprintaddr(char *str, size_t size, struct nrf_sockaddr *addr)
{
	switch (addr->sa_family) {
	case NRF_AF_INET:
		nrf_inet_ntop(addr->sa_family, &SIN(addr)->sin_addr, str, size);
		break;
	case NRF_AF_INET6:
		nrf_inet_ntop(addr->sa_family, &SIN6(addr)->sin6_addr, str, size);
		break;
	default:
		snprintf(str, size, "Unknown family %d", addr->sa_family);
	}
}

static void ifaddrs_print(void)
{
	int err;

	struct nrf_ifaddrs *ifaddrs;
	struct nrf_ifaddrs *ifa;

	char buf[NRF_INET6_ADDRSTRLEN] = {0};

	err = nrf_getifaddrs(&ifaddrs);
	if (err) {
		printk("nrf_getifaddrs() failed, err %d\n", err);
		return;
	}

	printk("\nInterface addresses:\n");

	ifa = ifaddrs;
	while (ifa != NULL) {
		snprintaddr(buf, sizeof(buf), ifa->ifa_addr);
		printk("%s: (%s) %s\n",
		       ifa->ifa_name, net_family2str(ifa->ifa_addr->sa_family), buf);

		/* Netmask, broadaddr and dstaddr are not supported by the modem
		 * and will always be zero.
		 */

		ifa = ifa->ifa_next;
	}
	printk("\n");

	nrf_freeifaddrs(ifaddrs);
}

void pdn_event_handler(uint8_t cid, enum pdn_event event, int reason)
{
	switch (event) {
	case PDN_EVENT_CNEC_ESM:
		printk("Event: PDP context %d, %s\n", cid, pdn_esm_strerror(reason));
		break;
	default:
		printk("Event: PDP context %d %s\n", cid, event_str[event]);
		break;
	}
}

int main(void)
{
	int err;
	int esm;
	uint8_t cid;
	char apn[32];

	printk("PDN sample started\n");

	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem library initialization failed, error: %d\n", err);
		return 0;
	}

	/* Setup a callback for the default PDP context (zero).
	 * Do this before switching to function mode 1 (CFUN=1)
	 * to receive the first activation event.
	 */
	err = pdn_default_ctx_cb_reg(pdn_event_handler);
	if (err) {
		printk("pdn_default_ctx_cb_reg() failed, err %d\n", err);
		return 0;
	}

	err = lte_lc_connect();
	if (err) {
		return 0;
	}

	err = pdn_default_apn_get(apn, sizeof(apn));
	if (err) {
		printk("pdn_default_apn_get() failed, err %d\n", err);
		return 0;
	}

	printk("Default APN is %s\n", apn);

	/* Create a PDP context and assign an event handler to receive events */
	err = pdn_ctx_create(&cid, pdn_event_handler);
	if (err) {
		printk("pdn_ctx_create() failed, err %d\n", err);
		return 0;
	}

	printk("Created new PDP context %d\n", cid);

	/* Configure a PDP context with APN and Family */
	err = pdn_ctx_configure(cid, apn, PDN_FAM_IPV4V6, NULL);
	if (err) {
		printk("pdn_ctx_configure() failed, err %d\n", err);
		return 0;
	}

	printk("PDP context %d configured: APN %s, Family %s\n",
	       cid, apn, fam_str[PDN_FAM_IPV4V6]);

	/* Activate a PDN connection */
	err = pdn_activate(cid, &esm, NULL);
	if (err) {
		printk("pdn_activate() failed, err %d esm %d %s\n",
			err, esm, pdn_esm_strerror(err));
		return 0;
	}

	printk("PDP Context %d, PDN ID %d\n", 0, pdn_id_get(0));
	printk("PDP Context %d, PDN ID %d\n", cid, pdn_id_get(cid));

	ifaddrs_print();

	lte_lc_power_off();
	printk("Bye\n");

	return 0;
}

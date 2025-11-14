/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <nrf_socket.h>

#define SIN(addr)  ((struct nrf_sockaddr_in *)(addr))

static const char * const fam_str[] = {
	[LTE_LC_PDN_FAM_IPV4V6] = "IPV4V6",
	[LTE_LC_PDN_FAM_IPV6] = "IPV6",
	[LTE_LC_PDN_FAM_IPV4] = "IPV4",
};

static const char * const event_str[] = {
	[LTE_LC_EVT_PDN_ESM_ERROR] = "ESM",
	[LTE_LC_EVT_PDN_ACTIVATED] = "activated",
	[LTE_LC_EVT_PDN_DEACTIVATED] = "deactivated",
	[LTE_LC_EVT_PDN_IPV6_UP] = "IPv6 up",
	[LTE_LC_EVT_PDN_IPV6_DOWN] = "IPv6 down",
	[LTE_LC_EVT_PDN_NETWORK_DETACH] = "network detach",
	[LTE_LC_EVT_PDN_APN_RATE_CONTROL_ON] = "APN rate control on",
	[LTE_LC_EVT_PDN_APN_RATE_CONTROL_OFF] = "APN rate control off",
	[LTE_LC_EVT_PDN_CTX_DESTROYED] = "context destroyed",
};

static void snprintaddr(int fam, char *str, size_t size, void *addr)
{
	char *ret;

	ret = zsock_inet_ntop(fam, addr, str, size);
	if (!ret) {
		snprintf(str, size, "Unable to parse");
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
		snprintaddr(ifa->ifa_addr->sa_family, buf, sizeof(buf),
			    &SIN(ifa->ifa_addr)->sin_addr);
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

static void dynamic_info_print(uint32_t cid)
{
	int err;
	struct lte_lc_pdn_dynamic_info info;
	char buf[NRF_INET6_ADDRSTRLEN] = {0};

	err = lte_lc_pdn_dynamic_info_get(cid, &info);
	if (err) {
		printk("lte_lc_pdn_dynamic_info_get() failed, err %d\n", err);
		return;
	}

	printk("Dynamic info for cid %d:\n", cid);
	if (info.dns_addr4_primary.s_addr) {
		snprintaddr(AF_INET, buf, sizeof(buf), &info.dns_addr4_primary);
		printk("Primary IPv4 DNS address: %s\n", buf);
		snprintaddr(AF_INET, buf, sizeof(buf), &info.dns_addr4_primary);
		printk("Secondary IPv4 DNS address: %s\n", buf);
		snprintaddr(AF_INET6, buf, sizeof(buf), &info.dns_addr6_primary);
		printk("Primary IPv6 DNS address: %s\n", buf);
		snprintaddr(AF_INET6, buf, sizeof(buf), &info.dns_addr6_primary);
		printk("Secondary IPv6 DNS address: %s\n", buf);
		printk("IPv4 MTU: %d, IPv6 MTU: %d\n", info.ipv4_mtu, info.ipv6_mtu);
	}
}

void lte_lc_evt_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_PDN:
		switch (evt->pdn.type) {
		case LTE_LC_EVT_PDN_ESM_ERROR:
			printk("Event: PDP context %d, %s\n", evt->pdn.cid,
			       lte_lc_pdn_esm_strerror(evt->pdn.esm_err));
			break;
		default:
			printk("Event: PDP context %d %s\n", evt->pdn.cid,
							     event_str[evt->pdn.type]);
			break;
		}
		break;
	default:
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

	/* Setup a callback for LTE events.
	 * Do this before switching to function mode 1 (CFUN=1)
	 * to receive the first activation event.
	 */
	lte_lc_register_handler(lte_lc_evt_handler);

	err = lte_lc_connect();
	if (err) {
		return 0;
	}

	err = lte_lc_pdn_ctx_default_apn_get(apn, sizeof(apn));
	if (err) {
		printk("lte_lc_pdn_ctx_default_apn_get() failed, err %d\n", err);
		return 0;
	}

	printk("Default APN is %s\n", apn);

	/* Create a PDP context and assign an event handler to receive events */
	err = lte_lc_pdn_ctx_create(&cid);
	if (err) {
		printk("lte_lc_pdn_ctx_create() failed, err %d\n", err);
		return 0;
	}

	printk("Created new PDP context %d\n", cid);

	/* Configure a PDP context with APN and Family */
	err = lte_lc_pdn_ctx_configure(cid, apn, LTE_LC_PDN_FAM_IPV4V6, NULL);
	if (err) {
		printk("lte_lc_pdn_ctx_configure() failed, err %d\n", err);
		return 0;
	}

	printk("PDP context %d configured: APN %s, Family %s\n",
	       cid, apn, fam_str[LTE_LC_PDN_FAM_IPV4V6]);

	/* Activate a PDN connection */
	err = lte_lc_pdn_activate(cid, &esm, NULL);
	if (err) {
		printk("lte_lc_pdn_activate() failed, err %d esm %d %s\n",
			err, esm, lte_lc_pdn_esm_strerror(err));
		return 0;
	}

	printk("PDP Context %d, PDN ID %d\n", 0, lte_lc_pdn_id_get(0));
	printk("PDP Context %d, PDN ID %d\n", cid, lte_lc_pdn_id_get(cid));

	dynamic_info_print(0);
	dynamic_info_print(cid);

	ifaddrs_print();

	lte_lc_power_off();
	printk("Bye\n");

	return 0;
}

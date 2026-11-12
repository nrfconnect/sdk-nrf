/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <nrf_socket.h>

#define SIN(addr)  ((struct nrf_sockaddr_in *)(addr))

/* How long to wait for IPv6 to come up after a PDN connection has been activated. */
#define IPV6_UP_TIMEOUT K_SECONDS(3)

/* Given by the event handler when IPv6 is up for the default PDP context (CID 0), and for the
 * PDP context created by the sample.
 */
static K_SEM_DEFINE(ipv6_up_default_ctx_sem, 0, 1);
static K_SEM_DEFINE(ipv6_up_new_ctx_sem, 0, 1);

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
	[LTE_LC_EVT_PDN_SUSPENDED] = "suspended",
	[LTE_LC_EVT_PDN_RESUMED] = "resumed",
};

static const char *pdn_event_str(enum lte_lc_pdn_evt_type type)
{
	if (type >= ARRAY_SIZE(event_str) || event_str[type] == NULL) {
		return "unknown event";
	}

	return event_str[type];
}

static void snprintaddr(int fam, char *str, size_t size, void *addr)
{
	char *ret;

	ret = zsock_inet_ntop(fam, addr, str, size);
	if (!ret) {
		snprintk(str, size, "Unable to parse");
	}
}

/* Print an address, or that it is not available if it is the unspecified address. */
static void dynamic_info_addr_print(const char *name, int fam, void *addr)
{
	char buf[NET_INET6_ADDRSTRLEN];
	const bool unspecified = (fam == NET_AF_INET) ?
				 net_ipv4_is_addr_unspecified(addr) :
				 net_ipv6_is_addr_unspecified(addr);

	if (unspecified) {
		printk("%s: not available\n", name);

		return;
	}

	snprintaddr(fam, buf, sizeof(buf), addr);
	printk("%s: %s\n", name, buf);
}

static void dynamic_info_mtu_print(const char *name, uint32_t mtu)
{
	if (mtu == 0) {
		printk("%s: not available\n", name);
		return;
	}

	printk("%s: %u\n", name, mtu);
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

static void dynamic_info_print(uint8_t cid, bool ipv6_up)
{
	int err;
	struct lte_lc_pdn_dynamic_info info;

	err = lte_lc_pdn_dynamic_info_get(cid, &info);
	if (err) {
		printk("lte_lc_pdn_dynamic_info_get() failed, err %d\n", err);
		return;
	}

	printk("Dynamic info for cid %u:\n", cid);

	printk("APN: %s\n", info.apn);

	dynamic_info_addr_print("IPv4 address", NET_AF_INET, &info.ipv4_addr);
	dynamic_info_addr_print("Primary IPv4 DNS address", NET_AF_INET, &info.dns_addr4_primary);
	dynamic_info_addr_print("Secondary IPv4 DNS address", NET_AF_INET,
				&info.dns_addr4_secondary);
	dynamic_info_mtu_print("IPv4 MTU", info.ipv4_mtu);

	/* The IPv6 address is only assigned when IPv6 is up. */
	if (!ipv6_up) {
		printk("IPv6 is not up\n");

		return;
	}

	dynamic_info_addr_print("IPv6 address", NET_AF_INET6, &info.ipv6_addr);
	dynamic_info_addr_print("Primary IPv6 DNS address", NET_AF_INET6,
				&info.dns_addr6_primary);
	dynamic_info_addr_print("Secondary IPv6 DNS address", NET_AF_INET6,
				&info.dns_addr6_secondary);
	dynamic_info_mtu_print("IPv6 MTU", info.ipv6_mtu);
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
		case LTE_LC_EVT_PDN_IPV6_UP:
			printk("Event: PDP context %d %s\n", evt->pdn.cid,
							     pdn_event_str(evt->pdn.type));

			/* The sample creates one PDP context in addition to the default one. */
			if (evt->pdn.cid == 0) {
				k_sem_give(&ipv6_up_default_ctx_sem);
			} else {
				k_sem_give(&ipv6_up_new_ctx_sem);
			}
			break;
		default:
			printk("Event: PDP context %d %s\n", evt->pdn.cid,
							     pdn_event_str(evt->pdn.type));
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
	bool ipv6_up;
	char apn[LTE_LC_PDN_APN_LEN_MAX + 1];

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

	err = lte_lc_pdn_default_ctx_events_enable();
	if (err) {
		printk("lte_lc_pdn_default_ctx_events_enable() failed, err %d\n", err);
		return 0;
	}

	err = lte_lc_connect();
	if (err) {
		printk("lte_lc_connect() failed, err %d\n", err);
		return 0;
	}

	err = lte_lc_pdn_default_ctx_apn_get(apn, sizeof(apn));
	if (err) {
		printk("lte_lc_pdn_default_ctx_apn_get() failed, err %d\n", err);
		return 0;
	}

	printk("Default APN is %s\n", apn);

	/* Create a PDP context */
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
			err, esm, lte_lc_pdn_esm_strerror(esm));
		return 0;
	}

	printk("PDP Context %d, PDN ID %d\n", 0, lte_lc_pdn_id_get(0));
	printk("PDP Context %d, PDN ID %d\n", cid, lte_lc_pdn_id_get(cid));

	/* IPv6 comes up after the PDN connection has been activated. Wait for it, so that the
	 * IPv6 address can be printed. If the network does not support IPv6, the wait times out.
	 */
	ipv6_up = (k_sem_take(&ipv6_up_default_ctx_sem, IPV6_UP_TIMEOUT) == 0);
	dynamic_info_print(0, ipv6_up);

	ipv6_up = (k_sem_take(&ipv6_up_new_ctx_sem, IPV6_UP_TIMEOUT) == 0);
	dynamic_info_print(cid, ipv6_up);

	ifaddrs_print();

	lte_lc_power_off();
	printk("Bye\n");

	return 0;
}

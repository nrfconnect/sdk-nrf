/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Same pattern as samples/dect/dect_shell/src/mdns/dns_sd_advertise.c: registers a
 * DNS-SD service record via mdns_responder_set_ext_records() so Zephyr's mDNS
 * responder answers PTR/SRV/TXT browse queries for _dect-nr._udp.local. mDNS on
 * UDP 5353 is handled by the stack (mdns_responder).
 *
 * A UDP socket is bound to [::]:CONFIG_DECT_TETHER_IPV6_SAMPLE_MDNS_DNS_SD_PORT
 * to satisfy Zephyr's port_in_use() guard in dns_sd_handle_ptr_query(); without
 * it the SRV is not advertised. The bind is wildcard (any interface) and done
 * once at init -- no need to wait for dect0 specifically.
 *
 * Instance name follows net_hostname_get(); the record is refreshed on dect0
 * IPv6 address changes (so records track address changes after peer sync) and
 * on dect0 up/down transitions.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dns_sd.h>
#include <zephyr/net/hostname.h>
#include <zephyr/net/mdns_responder.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(bh6_mdns, CONFIG_DECT_TETHER_IPV6_SAMPLE_LOG_LEVEL);

#define BH6_MDNS_IFACE_NAME "dect0"

static char bh6_dns_sd_instance[DNS_SD_INSTANCE_MAX_SIZE + 1];

static const uint16_t bh6_dns_sd_port_be =
	sys_cpu_to_be16((uint16_t)CONFIG_DECT_TETHER_IPV6_SAMPLE_MDNS_DNS_SD_PORT);

static struct dns_sd_rec bh6_dns_sd_rec = {
	.instance = bh6_dns_sd_instance,
	.service = "_dect-nr",
	.proto = "_udp",
	.domain = "local",
	.text = DNS_SD_EMPTY_TXT,
	.text_size = 0,
	.port = &bh6_dns_sd_port_be,
};

static K_MUTEX_DEFINE(bh6_dns_sd_mutex);

static void bh6_dns_sd_refresh(void)
{
	const char *h = net_hostname_get();
	size_t n = strlen(h);

	if (n > DNS_SD_INSTANCE_MAX_SIZE) {
		n = DNS_SD_INSTANCE_MAX_SIZE;
	}
	if (n < DNS_SD_INSTANCE_MIN_SIZE) {
		LOG_WRN("DNS-SD: hostname too short for instance label, skip");
		return;
	}

	k_mutex_lock(&bh6_dns_sd_mutex, K_FOREVER);
	memcpy(bh6_dns_sd_instance, h, n);
	bh6_dns_sd_instance[n] = '\0';
	int ret = mdns_responder_set_ext_records(&bh6_dns_sd_rec, 1);

	LOG_INF("DNS-SD: refresh instance=%s.%s.%s.local ret=%d", bh6_dns_sd_instance,
		bh6_dns_sd_rec.service, bh6_dns_sd_rec.proto, ret);
	k_mutex_unlock(&bh6_dns_sd_mutex);
}

enum bh6_dns_sd_state {
	BH6_DNS_SD_STATE_WAITING_IFACE,
	BH6_DNS_SD_STATE_ADVERTISING,
};

static atomic_t bh6_dns_sd_state = ATOMIC_INIT(BH6_DNS_SD_STATE_WAITING_IFACE);

static struct net_if *bh6_mdns_net_if(void)
{
	int idx = net_if_get_by_name(BH6_MDNS_IFACE_NAME);

	if (idx < 0) {
		return NULL;
	}

	return net_if_get_by_index(idx);
}

static void bh6_dns_sd_evt(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			   struct net_if *iface)
{
	ARG_UNUSED(cb);

	if (iface != bh6_mdns_net_if()) {
		return;
	}

	switch (mgmt_event) {
	case NET_EVENT_IF_UP:
		if (atomic_cas(&bh6_dns_sd_state, BH6_DNS_SD_STATE_WAITING_IFACE,
			       BH6_DNS_SD_STATE_ADVERTISING)) {
			LOG_INF("DNS-SD: advertising _dect-nr._udp on %s, instance=%s (port %u)",
				BH6_MDNS_IFACE_NAME, net_hostname_get(),
				(unsigned int)CONFIG_DECT_TETHER_IPV6_SAMPLE_MDNS_DNS_SD_PORT);
		}
		bh6_dns_sd_refresh();
		break;
	case NET_EVENT_IF_DOWN:
		atomic_set(&bh6_dns_sd_state, BH6_DNS_SD_STATE_WAITING_IFACE);
		break;
	case NET_EVENT_IPV6_ADDR_ADD:
	case NET_EVENT_IPV6_ADDR_DEL:
		bh6_dns_sd_refresh();
		break;
	default:
		break;
	}
}

static struct net_mgmt_event_callback bh6_dns_sd_cb;

/* Placeholder listener: bind [::]:CONFIG_DECT_TETHER_IPV6_SAMPLE_MDNS_DNS_SD_PORT
 * and leave it open. Zephyr's dns_sd_handle_ptr_query() rejects SRV advertising
 * for a port that isn't in use; this bind satisfies that guard without spawning
 * a thread.
 */
static int bh6_dns_sd_placeholder_bind(void)
{
	struct sockaddr_in6 bind_addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(CONFIG_DECT_TETHER_IPV6_SAMPLE_MDNS_DNS_SD_PORT),
		.sin6_addr = in6addr_any,
	};
	int sock;

	sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("DNS-SD: placeholder socket failed (%d)", errno);
		return -errno;
	}

	if (zsock_bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
		LOG_ERR("DNS-SD: placeholder bind [::]:%u failed (%d)",
			(unsigned int)CONFIG_DECT_TETHER_IPV6_SAMPLE_MDNS_DNS_SD_PORT, errno);
		zsock_close(sock);
		return -errno;
	}

	LOG_INF("DNS-SD: placeholder listener bound UDP [::]:%u",
		(unsigned int)CONFIG_DECT_TETHER_IPV6_SAMPLE_MDNS_DNS_SD_PORT);
	return 0;
}

static int bh6_dns_sd_init(void)
{
	struct net_if *iface;
	int ret;

	net_mgmt_init_event_callback(&bh6_dns_sd_cb, bh6_dns_sd_evt,
				     NET_EVENT_IF_UP | NET_EVENT_IF_DOWN |
				     NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_ADDR_DEL);
	net_mgmt_add_event_callback(&bh6_dns_sd_cb);

	ret = bh6_dns_sd_placeholder_bind();
	if (ret < 0) {
		LOG_ERR("DNS-SD: placeholder bind failed (%d); SRV record will not be advertised",
			ret);
	}

	/* Register the DNS-SD record now so mdns_responder can answer as soon as
	 * dect0 is up and has an IPv6 address.
	 */
	bh6_dns_sd_refresh();

	/* If dect0 is already up before this init ran, flip state directly. */
	iface = bh6_mdns_net_if();
	if (iface != NULL && net_if_flag_is_set(iface, NET_IF_UP)) {
		atomic_set(&bh6_dns_sd_state, BH6_DNS_SD_STATE_ADVERTISING);
	}

	return 0;
}

SYS_INIT(bh6_dns_sd_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

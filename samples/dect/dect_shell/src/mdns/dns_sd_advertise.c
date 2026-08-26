/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * DNS-SD _dect-nr._udp on dect0.
 *
 * Registers a DNS-SD service record via mdns_responder_set_ext_records() so
 * Zephyr's mDNS responder answers PTR/SRV/TXT browse queries for
 * _dect-nr._udp.local.
 *
 * The SRV record points at CONFIG_SAMPLE_DESH_MDNS_DNS_SD_PORT (default 4700)
 * as a placeholder — no real _dect-nr._udp application service is
 * implemented yet. The TXT record carries "status=not-implemented" so
 * browsers can see this state.
 *
 * A UDP socket is bound to [::]:CONFIG_SAMPLE_DESH_MDNS_DNS_SD_PORT to
 * satisfy Zephyr's port_in_use() guard in dns_sd_handle_ptr_query(); without
 * it the SRV is not advertised.
 *
 * Instance name follows net_hostname_get(); the record is refreshed on
 * hostname changes (from the "hostname" shell cmd) and dect0 IPv6 address
 * changes. State flips to "advertising" once dect0 comes up.
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

LOG_MODULE_REGISTER(dect_shell_dns_sd, LOG_LEVEL_INF);

/* DECT L2 net_if name from Zephyr (see "net iface" in shell). */
#define DECT_SHELL_MDNS_IFACE_NAME "dect0"

static char dect_shell_dns_sd_instance[DNS_SD_INSTANCE_MAX_SIZE + 1];

static const uint16_t dect_shell_dns_sd_port_be =
	sys_cpu_to_be16((uint16_t)CONFIG_SAMPLE_DESH_MDNS_DNS_SD_PORT);

/* DNS-SD TXT: one <length><key=value> entry per RFC 6763 §6. The leading
 * octal escape \026 is the length byte (22 = strlen("status=not-implemented")).
 * "status=not-implemented" flags to browsers that the SRV-advertised port
 * is a placeholder; replace/extend once a real _dect-nr._udp service exists.
 */
static const char dect_shell_dns_sd_txt[] = "\026status=not-implemented";

static struct dns_sd_rec dect_shell_dns_sd_rec = {
	.instance = dect_shell_dns_sd_instance,
	.service = "_dect-nr",
	.proto = "_udp",
	.domain = "local",
	.text = dect_shell_dns_sd_txt,
	.text_size = sizeof(dect_shell_dns_sd_txt) - 1,
	.port = &dect_shell_dns_sd_port_be,
};

static K_MUTEX_DEFINE(dect_shell_dns_sd_mutex);

void dect_shell_dns_sd_refresh(void)
{
	const char *h = net_hostname_get();
	size_t n = strlen(h);

	if (n > DNS_SD_INSTANCE_MAX_SIZE) {
		n = DNS_SD_INSTANCE_MAX_SIZE;
	}
	if (n < DNS_SD_INSTANCE_MIN_SIZE) {
		LOG_WRN("DNS-SD: hostname too short for service instance, skip");
		return;
	}

	k_mutex_lock(&dect_shell_dns_sd_mutex, K_FOREVER);
	memcpy(dect_shell_dns_sd_instance, h, n);
	dect_shell_dns_sd_instance[n] = '\0';
	int ret = mdns_responder_set_ext_records(&dect_shell_dns_sd_rec, 1);

	LOG_INF("DNS-SD: refresh instance=%s.%s.%s.local ret=%d",
		dect_shell_dns_sd_instance, dect_shell_dns_sd_rec.service,
		dect_shell_dns_sd_rec.proto, ret);
	k_mutex_unlock(&dect_shell_dns_sd_mutex);
}

enum dect_shell_dns_sd_state {
	DECT_SHELL_DNS_SD_STATE_WAITING_IFACE,
	DECT_SHELL_DNS_SD_STATE_ADVERTISING,
};

static atomic_t dect_shell_dns_sd_state = ATOMIC_INIT(DECT_SHELL_DNS_SD_STATE_WAITING_IFACE);

const char *dect_shell_dns_sd_state_str(void)
{
	switch ((enum dect_shell_dns_sd_state)atomic_get(&dect_shell_dns_sd_state)) {
	case DECT_SHELL_DNS_SD_STATE_ADVERTISING:
		return "advertising";
	case DECT_SHELL_DNS_SD_STATE_WAITING_IFACE:
	default:
		return "waiting for " DECT_SHELL_MDNS_IFACE_NAME " to come up";
	}
}

bool dect_shell_dns_sd_is_advertising(void)
{
	return atomic_get(&dect_shell_dns_sd_state) == DECT_SHELL_DNS_SD_STATE_ADVERTISING;
}

static struct net_if *dect_shell_mdns_net_if(void)
{
	int idx = net_if_get_by_name(DECT_SHELL_MDNS_IFACE_NAME);

	if (idx < 0) {
		return NULL;
	}
	return net_if_get_by_index(idx);
}

static void dect_shell_dns_sd_evt(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				  struct net_if *iface)
{
	ARG_UNUSED(cb);

	if (iface != dect_shell_mdns_net_if()) {
		return;
	}

	switch (mgmt_event) {
	case NET_EVENT_IF_UP:
		if (atomic_cas(&dect_shell_dns_sd_state,
			       DECT_SHELL_DNS_SD_STATE_WAITING_IFACE,
			       DECT_SHELL_DNS_SD_STATE_ADVERTISING)) {
			LOG_INF("DNS-SD: advertising _dect-nr._udp on %s, instance=%s (port %u)",
				DECT_SHELL_MDNS_IFACE_NAME, net_hostname_get(),
				(unsigned int)CONFIG_SAMPLE_DESH_MDNS_DNS_SD_PORT);
		}
		dect_shell_dns_sd_refresh();
		break;
	case NET_EVENT_IF_DOWN:
		atomic_set(&dect_shell_dns_sd_state, DECT_SHELL_DNS_SD_STATE_WAITING_IFACE);
		break;
	case NET_EVENT_IPV6_ADDR_ADD:
	case NET_EVENT_IPV6_ADDR_DEL:
		dect_shell_dns_sd_refresh();
		break;
	default:
		break;
	}
}

static struct net_mgmt_event_callback dect_shell_dns_sd_cb;

/* Placeholder listener: bind [::]:CONFIG_SAMPLE_DESH_MDNS_DNS_SD_PORT and
 * leave it open. Zephyr's dns_sd_handle_ptr_query() rejects SRV advertising
 * for a port that isn't in use; this bind satisfies that guard without
 * spawning a thread. Replace with a real service handler when one exists.
 */
static int dect_shell_dns_sd_placeholder_bind(void)
{
	struct sockaddr_in6 bind_addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(CONFIG_SAMPLE_DESH_MDNS_DNS_SD_PORT),
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
			(unsigned int)CONFIG_SAMPLE_DESH_MDNS_DNS_SD_PORT, errno);
		zsock_close(sock);
		return -errno;
	}

	LOG_INF("DNS-SD: placeholder listener bound UDP [::]:%u",
		(unsigned int)CONFIG_SAMPLE_DESH_MDNS_DNS_SD_PORT);
	return 0;
}

static int dect_shell_dns_sd_init(void)
{
	struct net_if *iface;
	int ret;

	net_mgmt_init_event_callback(&dect_shell_dns_sd_cb, dect_shell_dns_sd_evt,
				     NET_EVENT_IF_UP | NET_EVENT_IF_DOWN |
				     NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_ADDR_DEL);
	net_mgmt_add_event_callback(&dect_shell_dns_sd_cb);

	ret = dect_shell_dns_sd_placeholder_bind();
	if (ret < 0) {
		LOG_ERR("DNS-SD: placeholder bind failed (%d); SRV record will not be advertised",
			ret);
	}

	/* Register the DNS-SD record now so mdns_responder can answer as soon
	 * as dect0 is up and has an IPv6 address.
	 */
	dect_shell_dns_sd_refresh();

	/* If dect0 is already up before this init ran, flip state directly. */
	iface = dect_shell_mdns_net_if();
	if (iface != NULL && net_if_flag_is_set(iface, NET_IF_UP)) {
		atomic_set(&dect_shell_dns_sd_state, DECT_SHELL_DNS_SD_STATE_ADVERTISING);
	}

	return 0;
}

SYS_INIT(dect_shell_dns_sd_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * ``dect discover``: browse _dect-nr._udp peers over DECT NR+ (DNS-SD PTR), then
 * resolve IPv6 addresses (AAAA) per peer. Long RD ID comes from each address
 * via dect_utils.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/hostname.h>
#include <zephyr/net/net_ip.h>

#include <net/dect/dect_utils.h>

#include "desh_print.h"

#if defined(CONFIG_SAMPLE_DESH_MDNS_DNS_SD_ADVERTISE)
/* dns_sd_advertise.c — local _dect-nr._udp advertise status, for "dect discover" to report. */
const char *dect_shell_dns_sd_state_str(void);
bool dect_shell_dns_sd_is_advertising(void);
#endif

#define DECT_DISCOVER_MAX_HOSTS 24

/* Column widths for discover table; IPv6 fits longest textual address (39 chars). */
#define DECT_DISCOVER_ROW_MAX 112
#define DECT_DISCOVER_HOST_W  24
#define DECT_DISCOVER_KIND_W  4
#define DECT_DISCOVER_IPV6_W  39

#define SUFFIX_DECT_NR	"._dect-nr._udp.local"

/* Browse: cap DNS budget at 5 s (+ 2.5 s slack).
 * AAAA: full CONFIG_NET_SOCKETS_DNS_TIMEOUT + 6 s slack.
 */
#define DISCOVER_PTR_DNS_BUDGET_MS   5000
#define DISCOVER_PTR_SLACK_MS	     2500U
#define DISCOVER_AAAA_SLACK_MS	     6000U
#define DISCOVER_WAIT_TICK_MS	     2000U
#define DISCOVER_PTR_BROWSE_PASSES   2U
#define DISCOVER_PTR_QUERY_ATTEMPTS  2U

/* .local queries use DNS id 0 (RFC 6762); pass name+type to cancel the right slot. */
static int discover_wait_done(struct dns_resolve_context *ctx, uint16_t dns_id, struct k_sem *sem,
			      const char *qname, enum dns_query_type qtype,
			      int32_t dns_budget_ms, uint32_t slack_ms)
{
	uint32_t wait_total;
	uint32_t remaining;

	if (dns_budget_ms < 1000) {
		dns_budget_ms = CONFIG_NET_SOCKETS_DNS_TIMEOUT;
	}

	wait_total = (uint32_t)dns_budget_ms + slack_ms;
	remaining = wait_total;

	while (remaining > 0U) {
		uint32_t step = remaining > DISCOVER_WAIT_TICK_MS ?
			DISCOVER_WAIT_TICK_MS : remaining;

		if (k_sem_take(sem, K_MSEC(step)) == 0) {
			return 0;
		}
		remaining -= step;
	}

	desh_warn("discover: query completion timeout (%u ms), cancel id %u", wait_total,
		  (unsigned int)dns_id);
	if (qname != NULL) {
		(void)dns_resolve_cancel_with_name(ctx, dns_id, qname, qtype);
	} else {
		(void)dns_resolve_cancel(ctx, dns_id);
	}
	(void)k_sem_take(sem, K_MSEC(1000));
	return -ETIMEDOUT;
}

static void discover_print_sep_row(void)
{
	char line[DECT_DISCOVER_ROW_MAX + 2];

	line[0] = ' ';
	memset(line + 1, '-', DECT_DISCOVER_ROW_MAX);
	line[DECT_DISCOVER_ROW_MAX + 1] = '\0';
	desh_print("%s", line);
}

struct discover_ctx {
	struct k_sem wait;
	char hosts[DECT_DISCOVER_MAX_HOSTS][CONFIG_DNS_RESOLVER_MAX_NAME_LEN + 1];
	size_t n_hosts;
	char cur_host[CONFIG_DNS_RESOLVER_MAX_NAME_LEN + 1];
	size_t row_index; /* 1-based, matches resolve order */
	bool got_aaaa;
};

static const char *discover_ipv6_kind(const struct net_in6_addr *a)
{
	if (net_ipv6_is_ll_addr(a)) {
		return "LL";
	}
	if (net_ipv6_is_ula_addr(a)) {
		return "ULA";
	}
	if (net_ipv6_is_global_addr(a)) {
		return "GUA";
	}
	return "?";
}

static void str_ascii_tolower_inplace(char *s)
{
	for (; *s != '\0'; s++) {
		if (*s >= 'A' && *s <= 'Z') {
			*s = (char)(*s - 'A' + 'a');
		}
	}
}

static bool dns_ascii_eq_ci(const char *a, const char *b)
{
	for (; *a && *b; a++, b++) {
		char x = *a;
		char y = *b;

		if (x >= 'A' && x <= 'Z') {
			x += 32;
		}
		if (y >= 'A' && y <= 'Z') {
			y += 32;
		}
		if (x != y) {
			return false;
		}
	}

	return *a == *b;
}

static bool name_ends_with_ci(const char *name, const char *suf)
{
	size_t ln = strlen(name);
	size_t ls = strlen(suf);

	if (ls > ln) {
		return false;
	}

	return dns_ascii_eq_ci(name + ln - ls, suf);
}

static int host_add(struct discover_ctx *d, const char *hostlocal)
{
	if (d->n_hosts >= DECT_DISCOVER_MAX_HOSTS) {
		return -ENOMEM;
	}

	for (size_t i = 0; i < d->n_hosts; i++) {
		if (strcmp(d->hosts[i], hostlocal) == 0) {
			return 0;
		}
	}

	strncpy(d->hosts[d->n_hosts], hostlocal, CONFIG_DNS_RESOLVER_MAX_NAME_LEN);
	d->hosts[d->n_hosts][CONFIG_DNS_RESOLVER_MAX_NAME_LEN] = '\0';
	d->n_hosts++;

	return 0;
}

static int ptr_instance_to_hostlocal(const char *canon, char *out, size_t outlen)
{
	const char *suf = NULL;

	if (name_ends_with_ci(canon, SUFFIX_DECT_NR)) {
		suf = SUFFIX_DECT_NR;
	} else {
		return -ENOENT;
	}

	size_t clen = strlen(canon);
	size_t slen = strlen(suf);

	if (clen <= slen) {
		return -EINVAL;
	}

	size_t label_len = clen - slen;

	if (label_len + sizeof(".local") > outlen) {
		return -E2BIG;
	}

	memcpy(out, canon, label_len);
	memcpy(out + label_len, ".local", sizeof(".local"));
	/* Lowercase: resolver hashes lowercase QNAMEs; needed for cancel on timeout. */
	str_ascii_tolower_inplace(out);

	return 0;
}

static bool discover_host_is_self_local(const char *hostlocal)
{
	char expect[CONFIG_DNS_RESOLVER_MAX_NAME_LEN + 1];
	int n;

	n = snprintf(expect, sizeof(expect), "%s.local", net_hostname_get());
	if (n <= 0 || (size_t)n >= sizeof(expect)) {
		return false;
	}

	return dns_ascii_eq_ci(hostlocal, expect);
}

static void ptr_cb(enum dns_resolve_status status, struct dns_addrinfo *info, void *user_data)
{
	struct discover_ctx *d = user_data;

	if (status == DNS_EAI_INPROGRESS) {
		if (info != NULL && info->ai_family == NET_AF_LOCAL) {
			char hl[CONFIG_DNS_RESOLVER_MAX_NAME_LEN + 1];

			if (ptr_instance_to_hostlocal(info->ai_canonname, hl, sizeof(hl)) == 0) {
				if (host_add(d, hl) == -ENOMEM) {
					desh_warn("discover: host list full (%d)",
						  DECT_DISCOVER_MAX_HOSTS);
				}
			}
		}
		return;
	}

	/* Resolver also ends with DNS_EAI_FAIL / DNS_EAI_NODATA etc. (see resolve.c
	 * dispatcher_cb quit path); only ALLDONE/CANCELED would leave us stuck.
	 */
	k_sem_give(&d->wait);
}

static int32_t discover_ptr_dns_budget_ms(void)
{
	int32_t b = CONFIG_NET_SOCKETS_DNS_TIMEOUT;

	if (b > (int32_t)DISCOVER_PTR_DNS_BUDGET_MS) {
		b = (int32_t)DISCOVER_PTR_DNS_BUDGET_MS;
	}

	return b;
}

static int discover_run_ptr(struct dns_resolve_context *ctx, struct discover_ctx *d,
			    const char *service)
{
	uint16_t dns_id;
	int32_t ptr_budget = discover_ptr_dns_budget_ms();
	int ret = 0;

	for (unsigned int attempt = 1; attempt <= DISCOVER_PTR_QUERY_ATTEMPTS; attempt++) {
		k_sem_init(&d->wait, 0, 1);
		ret = dns_resolve_service(ctx, service, &dns_id, ptr_cb, d, ptr_budget);
		if (ret == -EAGAIN) {
			desh_warn("discover: resolver busy (-EAGAIN), retry PTR %s (%u/%u)",
				  service, attempt, DISCOVER_PTR_QUERY_ATTEMPTS);
			k_sleep(K_MSEC(250));
			k_sem_init(&d->wait, 0, 1);
			ret = dns_resolve_service(ctx, service, &dns_id, ptr_cb, d, ptr_budget);
		}
		if (ret < 0) {
			return ret;
		}

		ret = discover_wait_done(ctx, dns_id, &d->wait, service, DNS_QUERY_TYPE_PTR,
					 ptr_budget, DISCOVER_PTR_SLACK_MS);
		if (ret == 0) {
			return 0;
		}

		if (attempt < DISCOVER_PTR_QUERY_ATTEMPTS) {
			desh_warn("discover: PTR %s timed out (%u/%u), retrying", service, attempt,
				  DISCOVER_PTR_QUERY_ATTEMPTS);
		}
	}

	return ret;
}

static void aaaa_cb(enum dns_resolve_status status, struct dns_addrinfo *info, void *user_data)
{
	struct discover_ctx *d = user_data;

	if (status == DNS_EAI_INPROGRESS) {
		if (info != NULL && info->ai_family == NET_AF_INET6) {
			struct net_in6_addr *addr = &net_sin6(&info->ai_addr)->sin6_addr;
			char ip[NET_IPV6_ADDR_LEN];
			uint32_t rd = dect_utils_lib_long_rd_id_from_ipv6_addr(addr);

			d->got_aaaa = true;
			net_addr_ntop(NET_AF_INET6, addr, ip, sizeof(ip));
			desh_print(" %2u | %-*.*s | %-*.*s | %-*.*s | %u (0x%08x)",
				   (unsigned int)d->row_index, DECT_DISCOVER_HOST_W,
				   DECT_DISCOVER_HOST_W, d->cur_host, DECT_DISCOVER_KIND_W,
				   DECT_DISCOVER_KIND_W, discover_ipv6_kind(addr),
				   DECT_DISCOVER_IPV6_W, DECT_DISCOVER_IPV6_W, ip,
				   (unsigned int)rd, (unsigned int)rd);
		}
		return;
	}

	if (status == DNS_EAI_ALLDONE) {
		if (!d->got_aaaa) {
			desh_warn("discover: no AAAA for %.60s", d->cur_host);
		}
	} else if (status == DNS_EAI_CANCELED) {
		if (!d->got_aaaa) {
			desh_warn("discover: timeout for %.60s", d->cur_host);
		}
	} else if (!d->got_aaaa) {
		desh_warn("discover: AAAA for %.60s ended (%d)", d->cur_host, status);
	}

	k_sem_give(&d->wait);
}

static void dect_shell_discover_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct discover_ctx d = {0};
	struct dns_resolve_context *ctx = dns_resolve_get_default();
	int32_t t = CONFIG_NET_SOCKETS_DNS_TIMEOUT;
	uint16_t dns_id;
	int ret;

	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	desh_print("");
	desh_print("dect discover: browse _dect-nr._udp (PTR), then AAAA for IPv6 addresses");
	desh_print("  this device: %s.local", net_hostname_get());
#if defined(CONFIG_SAMPLE_DESH_MDNS_DNS_SD_ADVERTISE)
	desh_print("  local _dect-nr._udp advertise: %s", dect_shell_dns_sd_state_str());
	if (!dect_shell_dns_sd_is_advertising()) {
		desh_warn("  this device is not yet advertising itself; peers may not see it");
	}
#endif
	desh_print("  (Browse may take up to ~%u s.)",
		   (unsigned int)(((uint32_t)discover_ptr_dns_budget_ms() + DISCOVER_PTR_SLACK_MS +
			       999U) /
			      1000U));

	for (unsigned int pass = 1; pass <= DISCOVER_PTR_BROWSE_PASSES; pass++) {
		desh_print("  browse _dect-nr._udp.local (%u/%u)", pass,
			   DISCOVER_PTR_BROWSE_PASSES);
		ret = discover_run_ptr(ctx, &d, "_dect-nr._udp.local");
		if (ret < 0) {
			desh_error("discover: PTR _dect-nr failed (%u/%u, %d)", pass,
				   DISCOVER_PTR_BROWSE_PASSES, ret);
			desh_print("");
			return;
		}
		if (d.n_hosts > 0) {
			break;
		}
		if (pass < DISCOVER_PTR_BROWSE_PASSES) {
			desh_print("  no peers yet (%u/%u), retrying browse", pass,
				   DISCOVER_PTR_BROWSE_PASSES);
		}
	}

	if (d.n_hosts == 0) {
		desh_print("  no _dect-nr._udp peers found (%u/%u browse passes)",
			   DISCOVER_PTR_BROWSE_PASSES, DISCOVER_PTR_BROWSE_PASSES);
		desh_print("");
		return;
	}

	desh_print("  Browse done, %zu peer(s) — AAAA for IPv6 addresses", d.n_hosts);
	desh_print("  IPv6 addresses (%zu peer(s)):", d.n_hosts);
	desh_print(" %2s | %-*s | %-*s | %-*s | long_rd_id", "#", DECT_DISCOVER_HOST_W, "host",
		   DECT_DISCOVER_KIND_W, "kind", DECT_DISCOVER_IPV6_W, "ipv6 (mDNS)");
	discover_print_sep_row();

	for (size_t i = 0; i < d.n_hosts; i++) {
		d.got_aaaa = false;
		d.row_index = i + 1U;
		strncpy(d.cur_host, d.hosts[i], sizeof(d.cur_host));
		d.cur_host[sizeof(d.cur_host) - 1] = '\0';

		k_sem_init(&d.wait, 0, 1);
		/* Unqualified name hits dns_resolve_name_internal() hostname shortcut (no mDNS). */
		const char *aaaa_qname = discover_host_is_self_local(
			d.cur_host) ? net_hostname_get() : d.cur_host;

		ret = dns_resolve_name(
			ctx, aaaa_qname, DNS_QUERY_TYPE_AAAA, &dns_id, aaaa_cb, &d, t);
		if (ret < 0) {
			desh_error("discover: AAAA for %.60s failed (%d)", d.cur_host, ret);
			continue;
		}
		ret = discover_wait_done(ctx, dns_id, &d.wait, aaaa_qname, DNS_QUERY_TYPE_AAAA, t,
					 DISCOVER_AAAA_SLACK_MS);
		if (ret < 0) {
			desh_warn("discover: AAAA for %.60s timed out", d.cur_host);
		}
	}

	desh_print("");
	desh_print("dect discover: finished (%zu peer(s))", d.n_hosts);
	desh_print("");
}

SHELL_SUBCMD_ADD((dect), discover, NULL,
		 "Browse _dect-nr._udp peers on mDNS; print IPv6 (LL/ULA/GUA) and long RD ID. "
		 "Usage: dect discover\n",
		 dect_shell_discover_cmd, 1, 0);

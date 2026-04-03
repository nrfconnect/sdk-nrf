/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Mock PPP L2 and modem net_if for DECT L2 sink when CONFIG_MODEM_CELLULAR is set.
 * Provides _net_l2_PPP (so sink links), and wrapper implementations so
 * net_if_get_first_by_type(&_net_l2_PPP) returns a mock net_if and
 * pm_device_action_run is a no-op. Sink code is unchanged.
 * Wrap symbols are always defined so linker --wrap can be used unconditionally.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/atomic.h>

#if defined(CONFIG_MODEM_CELLULAR)

/* Stub PPP L2 so sink's net_if_get_first_by_type(&NET_L2_GET_NAME(PPP)) links.
 * enable = NULL so net_if_up() takes the "L2 does not support enable" path.
 */
const struct net_l2 _net_l2_PPP = {
	.recv = NULL,
	.send = NULL,
	.enable = NULL,
	.get_flags = NULL,
	.alloc = NULL,
};

/* Mock net_if_dev for the fake PPP modem interface (rest zero-initialized) */
static struct net_if_dev mock_ppp_if_dev = {
	.dev = NULL, /* Set in init from DEVICE_DT_GET(DT_ALIAS(modem)) */
	.l2 = &_net_l2_PPP,
	.l2_data = NULL,
};

#if defined(CONFIG_NET_IPV6)
/* IPv6 config with one global unicast 2001:db8::1 so sink's NET_EVENT_IPV6_ROUTER_ADD
 * handler can iterate iface->config.ip.ipv6->unicast and find a matching prefix.
 */
static struct net_if_ipv6 mock_ppp_ipv6;
#endif

/* Mock net_if returned when sink asks for PPP interface (not in net_if section,
 * so conn_mgr does not track it; test_dect_ft_conn_mgr_connect skips PPP conn_mgr
 * assertions when net_if_get_by_iface(ppp_if) < 0).
 */
static struct net_if mock_ppp_if = {
	.if_dev = &mock_ppp_if_dev,
};

static bool mock_ppp_if_inited;

static void mock_ppp_if_init(void)
{
	if (mock_ppp_if_inited) {
		return;
	}
	mock_ppp_if_inited = true;
	k_mutex_init(&mock_ppp_if.lock);
	k_mutex_init(&mock_ppp_if.tx_lock);
	mock_ppp_if_dev.dev = DEVICE_DT_GET(DT_ALIAS(modem));

#if defined(CONFIG_NET_IPV6)
	/* Real config.ip.ipv6 so sink ROUTER_ADD handler does not dereference NULL */
	mock_ppp_if.config.ip.ipv6 = &mock_ppp_ipv6;
	/* One unicast: 2001:db8::1 (global), same as test's ADDR_ADD/ROUTER_ADD */
	static const uint8_t global[] = {
		0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
	};
	memcpy(mock_ppp_ipv6.unicast[0].address.in6_addr.s6_addr, global, sizeof(global));
	mock_ppp_ipv6.unicast[0].address.family = NET_AF_INET6;
	mock_ppp_ipv6.unicast[0].is_used = 1U;
	/* Refcount 1 so sink IF_DOWN net_if_ipv6_addr_rm() can unref and clear is_used */
	atomic_set(&mock_ppp_ipv6.unicast[0].atomic_ref, 1);
#endif
}

#endif /* CONFIG_MODEM_CELLULAR */

#if defined(CONFIG_MODEM_CELLULAR)
/**
 * Return the mock PPP net_if used as sink prefix iface (for tests).
 * Caller may use it to simulate NET_EVENT_IF_UP / NET_EVENT_IPV6_ADDR_ADD.
 */
struct net_if *dect_test_get_mock_ppp_net_if(void)
{
	mock_ppp_if_init();
	return &mock_ppp_if;
}

#if defined(CONFIG_NET_IPV6)
/**
 * Return the number of IPv6 unicast addresses currently in use on the mock PPP iface.
 * Used by test_dect_ft_sink_down to verify IF_DOWN flushed all addresses (expect 0).
 */
int dect_test_mock_ppp_ipv6_unicast_used_count(void)
{
	int n = 0;

	for (int i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (mock_ppp_ipv6.unicast[i].is_used &&
		    mock_ppp_ipv6.unicast[i].address.family == NET_AF_INET6) {
			n++;
		}
	}
	return n;
}

/**
 * Restore the mock PPP IPv6 unicast address (2001:db8::1) after it was cleared by
 * sink IF_DOWN. Used by test_dect_ft_conn_mgr_connect to re-establish the sink
 * so cluster_configure gets a global prefix and L4 event can fire.
 */
void dect_test_mock_ppp_restore_ipv6_unicast(void)
{
	static const uint8_t global[] = {
		0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
	};

	memcpy(mock_ppp_ipv6.unicast[0].address.in6_addr.s6_addr, global, sizeof(global));
	mock_ppp_ipv6.unicast[0].address.family = NET_AF_INET6;
	mock_ppp_ipv6.unicast[0].is_used = 1U;
	atomic_set(&mock_ppp_ipv6.unicast[0].atomic_ref, 1);
}
#endif /* CONFIG_NET_IPV6 */
#endif /* CONFIG_MODEM_CELLULAR */

/*
 * Wrapper: when sink asks for PPP interface, return our mock; otherwise call real.
 * Always defined so linker --wrap=net_if_get_first_by_type is valid.
 */
extern struct net_if *__real_net_if_get_first_by_type(const struct net_l2 *l2);

struct net_if *__wrap_net_if_get_first_by_type(const struct net_l2 *l2)
{
#if defined(CONFIG_MODEM_CELLULAR)
	if (l2 == &_net_l2_PPP) {
		mock_ppp_if_init();
		return &mock_ppp_if;
	}
#endif
	return __real_net_if_get_first_by_type(l2);
}

/* Wrapper for net_if_start_rs so tests can verify sink starts RS after ROUTER_DEL */
extern void __real_net_if_start_rs(struct net_if *iface);

int test_net_if_start_rs_call_count;

void __wrap_net_if_start_rs(struct net_if *iface)
{
	test_net_if_start_rs_call_count++;
	__real_net_if_start_rs(iface);
}

#if defined(CONFIG_MODEM_CELLULAR)
/*
 * Stub for pm_device_action_run so sink links when CONFIG_PM_DEVICE is not enabled.
 * Provide the symbol directly (no linker wrap needed).
 */
int pm_device_action_run(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);
	return 0;
}
#endif /* CONFIG_MODEM_CELLULAR */

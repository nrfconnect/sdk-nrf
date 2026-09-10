/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * AF_PACKET taps (SOCK_DGRAM): forward IPv6 mDNS (UDP 5353, 40-byte IPv6 header,
 * no extension headers) between the host Ethernet and first DECT interface,
 * without IID NAT.
 *
 * Limitation (initial scope): Ethernet -> DECT only forwards **queries** to
 * ff02::fb whose IPv6 **source** is ULA or GUA. fe80:: sources are dropped.
 * DECT -> Ethernet forwards multicast mDNS to ff02::fb and **unicast** mDNS
 * whose **destination** is ULA or GUA (typical unicast reply to a DHCPv6 host).
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>

#include <net/dect/dect_net_l2.h>

#include "dect_tether_ipv6_int.h"

LOG_MODULE_DECLARE(dect_tether_ipv6, CONFIG_DECT_TETHER_IPV6_LOG_LEVEL);

#define BH6_MCAST_OFF_SRC	 8
#define BH6_MCAST_OFF_DST	 24
#define BH6_MCAST_OFF_NEXTHDR	 6
#define BH6_MCAST_IPV6_HDR_MIN 40
#define BH6_MCAST_IPV6_UDP_MIN 48U
#define BH6_MCAST_UDP_OFF	 40
#define BH6_MCAST_MDNS_PORT	 5353
#define BH6_MCAST_POLL_MS	 1000
#define BH6_MCAST_BUF_SIZE	 1500

#define BH6_MCAST_STACK_SIZE \
	CONFIG_DECT_TETHER_IPV6_MDNS_FORWARD_STACK_SIZE
#define BH6_MCAST_PRIO K_PRIO_COOP(CONFIG_DECT_TETHER_IPV6_MDNS_FORWARD_THREAD_PRIO)

struct bh6_mdns_fwd_ctx {
	atomic_t running;
	int fd_eth;
	int fd_dect;
	struct net_if *iface_eth;
	struct net_if *iface_dect;
	struct k_thread thread;
	k_tid_t tid;
};

static struct bh6_mdns_fwd_ctx bh6_mdns_fwd;
K_THREAD_STACK_DEFINE(bh6_mdns_fwd_stack, BH6_MCAST_STACK_SIZE);
/* RX buffer is large; keep off the thread stack (stack size Kconfig stays small). */
static uint8_t bh6_mdns_fwd_rx_buf[BH6_MCAST_BUF_SIZE];

static struct net_if *bh6_mcast_iface_dect(void)
{
	return net_if_get_first_by_type(&NET_L2_GET_NAME(DECT));
}

static const uint8_t bh6_mdns_mcast_dst[16] = {
	0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb,
};

static bool bh6_mdns_is_mdns_udp(const uint8_t *buf, size_t len)
{
	uint16_t src_port;
	uint16_t dst_port;

	if (len < BH6_MCAST_IPV6_UDP_MIN) {
		return false;
	}
	if ((buf[0] & 0xf0U) != 0x60U) {
		return false;
	}
	if (buf[BH6_MCAST_OFF_NEXTHDR] != NET_IPPROTO_UDP) {
		return false;
	}

	src_port = (uint16_t)((buf[BH6_MCAST_UDP_OFF] << 8) | buf[BH6_MCAST_UDP_OFF + 1]);
	dst_port = (uint16_t)((buf[BH6_MCAST_UDP_OFF + 2] << 8) | buf[BH6_MCAST_UDP_OFF + 3]);

	return src_port == BH6_MCAST_MDNS_PORT || dst_port == BH6_MCAST_MDNS_PORT;
}

static bool bh6_mdns_eth_query_to_mcast(const uint8_t *buf, size_t len)
{
	if (!bh6_mdns_is_mdns_udp(buf, len)) {
		return false;
	}
	return memcmp(buf + BH6_MCAST_OFF_DST, bh6_mdns_mcast_dst, 16) == 0;
}

static bool bh6_addr_is_ula_or_gua_raw(const uint8_t *a16)
{
	struct net_in6_addr a;

	memcpy(&a, a16, sizeof(a));
	return net_ipv6_is_ula_addr(&a) || net_ipv6_is_global_addr(&a);
}

static bool bh6_is_ll(const uint8_t *a16)
{
	struct net_in6_addr a;

	memcpy(&a, a16, sizeof(a));
	return net_ipv6_is_ll_addr(&a);
}

static bool bh6_is_unspecified(const uint8_t *a16)
{
	struct net_in6_addr a;

	memcpy(&a, a16, sizeof(a));
	return net_ipv6_is_addr_unspecified(&a);
}

/* Ethernet -> DECT: mDNS query to ff02::fb, source must be ULA/GUA (not link-local). */
static bool bh6_mdns_eth_to_dect_ok(const uint8_t *buf, size_t len)
{
	const uint8_t *src = buf + BH6_MCAST_OFF_SRC;

	if (!bh6_mdns_eth_query_to_mcast(buf, len)) {
		return false;
	}
	if (net_ipv6_is_addr_mcast_raw(src) || bh6_is_unspecified(src)) {
		return false;
	}
	if (bh6_is_ll(src)) {
		LOG_DBG("bh6_mdns_fwd: skip eth->dect (link-local src)");
		return false;
	}
	if (!bh6_addr_is_ula_or_gua_raw(src)) {
		LOG_DBG("bh6_mdns_fwd: skip eth->dect (src not ULA/GUA)");
		return false;
	}
	return true;
}

/* DECT -> Ethernet: mDNS to ff02::fb, or unicast mDNS to ULA/GUA (not fe80). */
static bool bh6_mdns_dect_to_eth_ok(const uint8_t *buf, size_t len)
{
	const uint8_t *dst = buf + BH6_MCAST_OFF_DST;

	if (!bh6_mdns_is_mdns_udp(buf, len)) {
		return false;
	}
	if (memcmp(dst, bh6_mdns_mcast_dst, 16) == 0) {
		return true;
	}
	if (net_ipv6_is_addr_mcast_raw(dst) || bh6_is_unspecified(dst)) {
		return false;
	}
	if (bh6_is_ll(dst)) {
		LOG_DBG("bh6_mdns_fwd: skip dect->eth (link-local dst)");
		return false;
	}
	if (!bh6_addr_is_ula_or_gua_raw(dst)) {
		LOG_DBG("bh6_mdns_fwd: skip dect->eth (dst not ULA/GUA)");
		return false;
	}
	return true;
}

static int bh6_mcast_tx_netif(struct net_if *iface, const uint8_t *buf, size_t len)
{
	struct net_pkt *pkt;
	enum net_verdict v;
	const k_timeout_t tmo = K_MSEC(100);

	if (iface == NULL || len == 0U) {
		return -EINVAL;
	}

	pkt = net_pkt_alloc_with_buffer(iface, len, NET_AF_INET6, NET_IPPROTO_RAW, tmo);
	if (pkt == NULL) {
		LOG_WRN("bh6_mdns_fwd: TX alloc failed len=%zu if=%d", len,
			net_if_get_by_iface(iface));
		return -ENOMEM;
	}
	if (net_pkt_write(pkt, buf, len) < 0) {
		net_pkt_unref(pkt);
		return -EIO;
	}

#if defined(CONFIG_NET_L2_ETHERNET)
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_IPV6);
	}
#endif

	net_pkt_cursor_init(pkt);
	v = net_if_send_data(iface, pkt);
	if (v == NET_OK) {
		return 0;
	}
	if (v == NET_CONTINUE) {
		return 0;
	}

	net_pkt_unref(pkt);
	LOG_WRN("bh6_mdns_fwd: TX drop len=%zu if=%d", len, net_if_get_by_iface(iface));
	return -EIO;
}

static void bh6_mdns_fwd_close_sockets(void)
{
	if (bh6_mdns_fwd.fd_eth >= 0) {
		(void)zsock_close(bh6_mdns_fwd.fd_eth);
		bh6_mdns_fwd.fd_eth = -1;
	}
	if (bh6_mdns_fwd.fd_dect >= 0) {
		(void)zsock_close(bh6_mdns_fwd.fd_dect);
		bh6_mdns_fwd.fd_dect = -1;
	}
}

static int bh6_mdns_fwd_open_sockets(void)
{
	int ret;
	struct sockaddr_ll sa;

	bh6_mdns_fwd.fd_eth = zsock_socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL));
	if (bh6_mdns_fwd.fd_eth < 0) {
		LOG_ERR("bh6_mdns_fwd: eth socket failed (%d)", errno);
		return -errno;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sll_family = AF_PACKET;
	sa.sll_ifindex = net_if_get_by_iface(bh6_mdns_fwd.iface_eth);
	sa.sll_protocol = htons(ETH_P_ALL);
	ret = zsock_bind(bh6_mdns_fwd.fd_eth, (const struct sockaddr *)&sa, sizeof(sa));
	if (ret < 0) {
		LOG_ERR("bh6_mdns_fwd: eth bind failed (%d)", errno);
		(void)zsock_close(bh6_mdns_fwd.fd_eth);
		bh6_mdns_fwd.fd_eth = -1;
		return -errno;
	}

	bh6_mdns_fwd.fd_dect = zsock_socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL));
	if (bh6_mdns_fwd.fd_dect < 0) {
		ret = -errno;
		LOG_ERR("bh6_mdns_fwd: dect socket failed (%d)", errno);
		goto err_eth;
	}

	sa.sll_ifindex = net_if_get_by_iface(bh6_mdns_fwd.iface_dect);
	ret = zsock_bind(bh6_mdns_fwd.fd_dect, (const struct sockaddr *)&sa, sizeof(sa));
	if (ret < 0) {
		ret = -errno;
		LOG_ERR("bh6_mdns_fwd: dect bind failed (%d)", errno);
		goto err_dect;
	}

	return 0;

err_dect:
	(void)zsock_close(bh6_mdns_fwd.fd_dect);
	bh6_mdns_fwd.fd_dect = -1;
err_eth:
	(void)zsock_close(bh6_mdns_fwd.fd_eth);
	bh6_mdns_fwd.fd_eth = -1;
	return ret;
}

static void bh6_mdns_fwd_thread(void *a, void *b, void *c)
{
	struct zsock_pollfd fds[2];
	int ret;
	uint8_t *buf = bh6_mdns_fwd_rx_buf;

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	LOG_INF("bh6_mdns_fwd: thread eth if=%d dect if=%d (NAT=0, ULA/GUA filter)",
		net_if_get_by_iface(bh6_mdns_fwd.iface_eth),
		net_if_get_by_iface(bh6_mdns_fwd.iface_dect));

	while (atomic_get(&bh6_mdns_fwd.running) != 0) {
		int nfds = 0;

		if (bh6_mdns_fwd.fd_eth >= 0) {
			fds[nfds].fd = bh6_mdns_fwd.fd_eth;
			fds[nfds].events = ZSOCK_POLLIN;
			fds[nfds].revents = 0;
			nfds++;
		}
		if (bh6_mdns_fwd.fd_dect >= 0) {
			fds[nfds].fd = bh6_mdns_fwd.fd_dect;
			fds[nfds].events = ZSOCK_POLLIN;
			fds[nfds].revents = 0;
			nfds++;
		}
		if (nfds == 0) {
			if (atomic_get(&bh6_mdns_fwd.running) == 0) {
				break;
			}
			k_sleep(K_MSEC(100));
			continue;
		}

		ret = zsock_poll(fds, nfds, BH6_MCAST_POLL_MS);
		if (ret < 0) {
			if (atomic_get(&bh6_mdns_fwd.running) == 0) {
				break;
			}
			LOG_WRN("bh6_mdns_fwd: poll err %d", errno);
			k_sleep(K_MSEC(100));
			continue;
		}
		if (ret == 0) {
			continue;
		}

		for (int i = 0; i < nfds; i++) {
			if ((fds[i].revents & ZSOCK_POLLIN) == 0) {
				continue;
			}
			for (;;) {
				ssize_t len;
				bool from_eth = fds[i].fd == bh6_mdns_fwd.fd_eth;
				struct net_if *dst;

				len = zsock_recv(fds[i].fd, buf, BH6_MCAST_BUF_SIZE,
						 ZSOCK_MSG_DONTWAIT);
				if (len <= 0) {
					if (len == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
						LOG_WRN("bh6_mdns_fwd: recv err fd=%d (%d)",
							fds[i].fd, errno);
					}
					break;
				}

				if (from_eth) {
					if (!bh6_mdns_eth_to_dect_ok(buf, (size_t)len)) {
						continue;
					}
					dst = bh6_mdns_fwd.iface_dect;
				} else {
					if (!bh6_mdns_dect_to_eth_ok(buf, (size_t)len)) {
						continue;
					}
					dst = bh6_mdns_fwd.iface_eth;
				}

				if (bh6_mcast_tx_netif(dst, buf, (size_t)len) == 0) {
					LOG_DBG("bh6_mdns_fwd: fwd len=%zd %s->%s", len,
						from_eth ? "eth" : "dect",
						from_eth ? "dect" : "eth");
				}
			}
		}
	}

	LOG_INF("bh6_mdns_fwd: thread exit");
}

void dect_tether_ipv6_mdns_fwd_stop(void)
{
	if (!atomic_cas(&bh6_mdns_fwd.running, 1, 0)) {
		return;
	}

	/* Close AF_PACKET sockets so poll()/recv() in the thread returns promptly. */
	bh6_mdns_fwd_close_sockets();

	if (bh6_mdns_fwd.tid != NULL) {
		if (k_thread_join(bh6_mdns_fwd.tid, K_SECONDS(2)) != 0) {
			LOG_WRN("bh6_mdns_fwd: thread join timeout");
		}
		bh6_mdns_fwd.tid = NULL;
	}

	LOG_INF("bh6_mdns_fwd: stopped");
}

int dect_tether_ipv6_mdns_fwd_start(void)
{
	int ret;

	bh6_mdns_fwd.iface_eth = dect_tether_ipv6_host_eth_iface();
	bh6_mdns_fwd.iface_dect = bh6_mcast_iface_dect();

	if (bh6_mdns_fwd.iface_eth == NULL) {
		LOG_WRN("bh6_mdns_fwd: no Ethernet interface");
		return -ENODEV;
	}
	if (bh6_mdns_fwd.iface_dect == NULL) {
		LOG_WRN("bh6_mdns_fwd: no DECT interface");
		return -ENODEV;
	}

	if (!atomic_cas(&bh6_mdns_fwd.running, 0, 1)) {
		LOG_WRN("bh6_mdns_fwd: already running");
		return -EALREADY;
	}

	bh6_mdns_fwd.fd_eth = -1;
	bh6_mdns_fwd.fd_dect = -1;
	bh6_mdns_fwd.tid = NULL;

	ret = bh6_mdns_fwd_open_sockets();
	if (ret != 0) {
		atomic_set(&bh6_mdns_fwd.running, 0);
		return ret;
	}

	bh6_mdns_fwd.tid = k_thread_create(&bh6_mdns_fwd.thread, bh6_mdns_fwd_stack,
					   K_THREAD_STACK_SIZEOF(bh6_mdns_fwd_stack),
					   bh6_mdns_fwd_thread, NULL, NULL, NULL, BH6_MCAST_PRIO,
					   0, K_NO_WAIT);
	if (bh6_mdns_fwd.tid == NULL) {
		atomic_set(&bh6_mdns_fwd.running, 0);
		bh6_mdns_fwd_close_sockets();
		return -ENOMEM;
	}
	k_thread_name_set(bh6_mdns_fwd.tid, "dect_bh6_mdns_fwd");

	LOG_INF("bh6_mdns_fwd: started eth=%d dect=%d (UDP/%u, no NAT)",
		net_if_get_by_iface(bh6_mdns_fwd.iface_eth),
		net_if_get_by_iface(bh6_mdns_fwd.iface_dect), BH6_MCAST_MDNS_PORT);
	return 0;
}

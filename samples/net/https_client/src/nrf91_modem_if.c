/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/posix/arpa/inet.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <errno.h>
#include <nrf_modem_at.h>
#include <nrf_socket.h>
#include <zephyr/logging/log.h>

#include "lte_ip_addr_helper.h"

LOG_MODULE_REGISTER(nrf91_modem_if, LOG_LEVEL_INF);

#define NO_MDM_SCKT 				(-1)
#define NRF91_MTU_DEFAULT			1500

#define NRF91_MODEM_DATA_UL_BUFFER_SIZE		1280
#define NRF91_MODEM_DATA_DL_BUFFER_SIZE 	1280

#define NRF91_MODEM_DATA_DL_THREAD_STACK_SIZE	1536
#define NRF91_MODEM_DATA_RCV_THREAD_PRIORITY	K_HIGHEST_APPLICATION_THREAD_PRIO

struct nrf91_modem_ctx {
	uint8_t mac_addr[6];
	struct net_if *iface;
	bool default_pdp_active;
	int mdm_skct_id;
};

static struct nrf91_modem_ctx nrf91_modem_ctx;

static char send_buffer[NRF91_MODEM_DATA_UL_BUFFER_SIZE];
static char receive_buffer[NRF91_MODEM_DATA_DL_BUFFER_SIZE];

static struct k_work_delayable mdm_socket_work;
static K_SEM_DEFINE(mdm_socket_sem, 0, 1);

static K_MUTEX_DEFINE(nr91_modem_ctx_lock);
static K_MUTEX_DEFINE(send_buf_lock);

/******************************************************************************/

static uint16_t pdn_mtu(void)
{
	struct lte_lc_pdn_dynamic_info info;
	int err;

	err = lte_lc_pdn_dynamic_info_get(0, &info);
	if ((err != 0) || (info.ipv4_mtu == 0)) {
		LOG_WRN("MTU query failed, error: %d, MTU: %d", err, info.ipv4_mtu);

		info.ipv4_mtu = NET_IPV4_MTU;
	}

	LOG_DBG("Network MTU: %d", info.ipv4_mtu);

	return (uint16_t)info.ipv4_mtu;
}

#if !defined(CONFIG_DNS_SERVER_IP_ADDRESSES)
static int util_get_dns_servers(int cid, struct in_addr *dns1, struct in_addr *dns2)
{
	int ret;
	char response[512];
	char dns1_str[NET_IPV4_ADDR_LEN] = { 0 };
	char dns2_str[NET_IPV4_ADDR_LEN] = { 0 };

	ret = nrf_modem_at_cmd(response, sizeof(response), "AT+CGCONTRDP=%d", cid);
	if (ret) {
		LOG_ERR("AT+CGCONTRDP failed: %d", ret);

		return -EIO;
	}

	ret = nrf_modem_at_scanf(response,
				 "+CGCONTRDP: %*d,%*d,%*[^,],%*[^,],%*[^,],%*[^,],"
				 "\"%15[.0-9]\",\"%15[.0-9]\"",
				 dns1_str, dns2_str);

	if (ret >= 1 && strlen(dns1_str) > 0) {
		if (inet_pton(AF_INET, dns1_str, dns1) == 1) {
			LOG_DBG("Primary DNS: %s", dns1_str);
		} else {
			return -EINVAL;
		}
	} else {
		return -ENOENT;
	}

	if (ret >= 2 && strlen(dns2_str) > 0) {
		inet_pton(AF_INET, dns2_str, dns2);
		LOG_DBG("Secondary DNS: %s", dns2_str);
	}

	return 0;
}

static void configure_dns_from_modem(int cid)
{
	int ret;
	struct in_addr dns1 = { 0 };
	struct in_addr dns2 = { 0 };
	struct dns_resolve_context *dnsctx;
	struct sockaddr_in dns_server1 = {
		.sin_family = AF_INET,
		.sin_port = htons(53),
		.sin_addr = dns1
	};
	struct sockaddr_in dns_server2 = {
		.sin_family = AF_INET,
		.sin_port = htons(53),
		.sin_addr = dns2
	};
	const struct sockaddr *dns_servers[] = {
		(struct sockaddr *)&dns_server1,
		(struct sockaddr *)&dns_server2,
		NULL
	};

	ret = util_get_dns_servers(cid, &dns1, &dns2);
	if (ret != 0) {
		return;
	}

	dnsctx = dns_resolve_get_default();

	ret = dns_resolve_reconfigure(dnsctx, NULL, dns_servers, DNS_SOURCE_MANUAL);
	if (ret < 0) {
		LOG_ERR("DNS reconfigure failed: %d", ret);
	}
}
#endif /* CONFIG_DNS_SERVER_IP_ADDRESSES */

static void on_pdn_activated(void)
{
	k_mutex_lock(&nr91_modem_ctx_lock, K_FOREVER);

	nrf91_modem_ctx.default_pdp_active = true;

	k_mutex_unlock(&nr91_modem_ctx_lock);

	k_work_schedule(&mdm_socket_work, K_SECONDS(2));
}

static void on_pdn_deactivated(void)
{
	k_mutex_lock(&nr91_modem_ctx_lock, K_FOREVER);

	nrf91_modem_ctx.default_pdp_active = false;

	k_mutex_unlock(&nr91_modem_ctx_lock);

	k_work_schedule(&mdm_socket_work, K_NO_WAIT);
}

#if CONFIG_NET_IPV6
static void on_pdn_ipv6_up(void)
{
	int ret;

	LOG_DBG("PDN IPv6 up");

	ret = lte_ipv6_addr_add(nrf91_modem_ctx.iface);
	if (ret && ret != -ENODATA) {
		LOG_ERR("lte_ipv6_addr_add failed: %d", ret);
	}
}

static void on_pdn_ipv6_down(void)
{
	int ret;

	LOG_DBG("PDN IPv6 down");

	ret = lte_ipv6_addr_remove(nrf91_modem_ctx.iface);
	if (ret) {
		LOG_ERR("lte_ipv6_addr_remove failed: %d", ret);
	}
}
#endif /* CONFIG_NET_IPV6 */

static void nrf91_lte_evt_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_PDN:
		if (evt->pdn.cid != 0) {
			break;
		}

		LOG_DBG("PDN event type: %d, cid: %d", evt->pdn.type, evt->pdn.cid);

		switch (evt->pdn.type) {
		case LTE_LC_EVT_PDN_ACTIVATED:
			on_pdn_activated();

			break;
		case LTE_LC_EVT_PDN_NETWORK_DETACH:
			__fallthrough;
		case LTE_LC_EVT_PDN_DEACTIVATED:
			on_pdn_deactivated();

			break;
#if CONFIG_NET_IPV6
		case LTE_LC_EVT_PDN_IPV6_UP:
			on_pdn_ipv6_up();

			break;
		case LTE_LC_EVT_PDN_IPV6_DOWN:
			on_pdn_ipv6_down();

			break;
#endif
		case LTE_LC_EVT_PDN_CTX_DESTROYED:
			LOG_DBG("PDN context destroyed");

			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void nrf91_socket_create(void)
{
	int ret;

	ret = nrf_socket(NRF_AF_PACKET, NRF_SOCK_RAW, 0);
	if (ret < 0) {
		LOG_ERR("nrf_socket failed %d", ret);

		ret = NO_MDM_SCKT;

		k_sem_reset(&mdm_socket_sem);
	} else {
		k_sem_give(&mdm_socket_sem);
	}

	k_mutex_lock(&nr91_modem_ctx_lock, K_FOREVER);

	nrf91_modem_ctx.mdm_skct_id = ret;

	k_mutex_unlock(&nr91_modem_ctx_lock);
}

static void nrf91_socket_close(void)
{
	int ret;

	if (!nrf_modem_is_initialized()) {
		LOG_WRN("Modem not initialized, skipping socket close");

		return;
	}

	k_sem_reset(&mdm_socket_sem);
	nrf_close(nrf91_modem_ctx.mdm_skct_id);

	k_mutex_lock(&nr91_modem_ctx_lock, K_FOREVER);

	nrf91_modem_ctx.mdm_skct_id = NO_MDM_SCKT;

	k_mutex_unlock(&nr91_modem_ctx_lock);

#if CONFIG_NET_IPV4
	ret = lte_ipv4_addr_remove(nrf91_modem_ctx.iface);
	if (ret) {
		LOG_ERR("lte_ipv4_addr_remove failed: %d", ret);
	}
#endif
#if CONFIG_NET_IPV6
	ret = lte_ipv6_addr_remove(nrf91_modem_ctx.iface);
	if (ret) {
		LOG_ERR("lte_ipv6_addr_remove failed: %d", ret);
	}
#endif

	net_if_dormant_on(nrf91_modem_ctx.iface);
}

static void nrf91_mdm_socket_worker(struct k_work *unused)
{
	bool pdp_active;
	int socket_id;
	int ret;

	ARG_UNUSED(unused);

	k_mutex_lock(&nr91_modem_ctx_lock, K_FOREVER);

	pdp_active = nrf91_modem_ctx.default_pdp_active;
	socket_id = nrf91_modem_ctx.mdm_skct_id;

	k_mutex_unlock(&nr91_modem_ctx_lock);

	LOG_DBG("Socket worker running, pdp_active: %d, socket: %d", pdp_active, socket_id);

	if (!pdp_active) {
		nrf91_socket_close();

		return;
	}

	if (socket_id != NO_MDM_SCKT) {
		LOG_DBG("Modem socket already created");

		return;
	}

	LOG_DBG("Creating modem socket");
	nrf91_socket_create();

	k_mutex_lock(&nr91_modem_ctx_lock, K_FOREVER);

	socket_id = nrf91_modem_ctx.mdm_skct_id;

	k_mutex_unlock(&nr91_modem_ctx_lock);

	if (socket_id == NO_MDM_SCKT) {
		LOG_ERR("Socket creation failed");

		return;
	}

#if CONFIG_NET_IPV4
	ret = lte_ipv4_addr_add(nrf91_modem_ctx.iface);
	if (ret && ret != -ENODATA) {
		LOG_ERR("lte_ipv4_addr_add failed: %d", ret);

		return;
	}

	if (ret == -ENODATA) {
		LOG_WRN("No IPv4 address from network");
	}
#endif /* CONFIG_NET_IPV4 */

#if CONFIG_NET_IPV6
	ret = lte_ipv6_addr_add(nrf91_modem_ctx.iface);
	if (ret && ret != -ENODATA) {
		LOG_ERR("lte_ipv6_addr_add failed: %d", ret);

		return;
	}
#endif /* CONFIG_NET_IPV6 */

	net_if_set_mtu(nrf91_modem_ctx.iface, pdn_mtu());
	net_if_flag_set(nrf91_modem_ctx.iface, NET_IF_UP);
	net_if_dormant_off(nrf91_modem_ctx.iface);
	net_if_set_default(nrf91_modem_ctx.iface);
	net_mgmt_event_notify(NET_EVENT_L4_CONNECTED, nrf91_modem_ctx.iface);

#if defined(CONFIG_DNS_RESOLVER) && !defined(CONFIG_DNS_SERVER_IP_ADDRESSES)
	configure_dns_from_modem(0);
#endif
}

static uint8_t *util_fake_dev_get_mac(struct nrf91_modem_ctx *ctx)
{
	if (ctx->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		ctx->mac_addr[0] = 0x00;
		ctx->mac_addr[1] = 0x00;
		ctx->mac_addr[2] = 0x5E;
		ctx->mac_addr[3] = 0x00;
		ctx->mac_addr[4] = 0x53;
		ctx->mac_addr[5] = sys_rand32_get();
	}

	return ctx->mac_addr;
}

static void nrf91_iface_init(struct net_if *iface)
{
	struct nrf91_modem_ctx *ctx = net_if_get_device(iface)->data;
	uint8_t *mac = util_fake_dev_get_mac(ctx);

	ctx->iface = iface;

	k_mutex_lock(&nr91_modem_ctx_lock, K_FOREVER);

	ctx->default_pdp_active = false;
	ctx->mdm_skct_id = NO_MDM_SCKT;

	k_mutex_unlock(&nr91_modem_ctx_lock);

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);
	net_if_dormant_on(iface);

	LOG_DBG("Interface initialized in dormant state");
}

static int nrf91_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	k_work_init_delayable(&mdm_socket_work, nrf91_mdm_socket_worker);
	lte_lc_register_handler(nrf91_lte_evt_handler);

	return 0;
}

static int nrf91_modem_send(const struct device *dev, struct net_pkt *pkt)
{
	int ret, data_len;

	k_mutex_lock(&send_buf_lock, K_FOREVER);

	data_len = net_pkt_get_len(pkt);

	ret = net_pkt_read(pkt, send_buffer, data_len);
	if (ret < 0) {
		LOG_ERR("Cannot read packet: %d", ret);
	} else {
		int socket_id;

		k_mutex_lock(&nr91_modem_ctx_lock, K_FOREVER);

		socket_id = ((struct nrf91_modem_ctx *)dev->data)->mdm_skct_id;

		k_mutex_unlock(&nr91_modem_ctx_lock);

		if (socket_id < 0) {
			LOG_ERR("No modem socket for send");

			ret = -ENETDOWN;
		} else {
			ret = nrf_send(socket_id, send_buffer, data_len, 0);
			if (ret < 0) {
				LOG_ERR("send() failed: (%d), data len: %d", -errno, data_len);
			} else if (ret != data_len) {
				LOG_ERR("Only partially sent: %d of %d", ret, data_len);
			}
		}
	}

	net_pkt_unref(pkt);
	k_mutex_unlock(&send_buf_lock);

	return ret;
}

static void nrf91_modem_recv_thread(void)
{
	struct net_pkt *rcv_pkt;
	int ret;
	int recv_data_len;
	int socket_id;

	while (true) {
		uint8_t ip_version;
		sa_family_t family;

		k_mutex_lock(&nr91_modem_ctx_lock, K_FOREVER);

		socket_id = nrf91_modem_ctx.mdm_skct_id;

		k_mutex_unlock(&nr91_modem_ctx_lock);

		if (socket_id < 0) {
			k_sem_take(&mdm_socket_sem, K_FOREVER);

			continue;
		}

		recv_data_len = nrf_recv(socket_id, receive_buffer, sizeof(receive_buffer), 0);
		if (recv_data_len > 0) {
			ip_version = (receive_buffer[0] >> 4) & 0x0F;
			if (ip_version == 4) {
				family = AF_INET;
			} else if (ip_version == 6) {
				family = AF_INET6;
			} else {
				LOG_ERR("Invalid IP version: %d", ip_version);

				continue;
			}

			rcv_pkt = net_pkt_rx_alloc_with_buffer(nrf91_modem_ctx.iface, recv_data_len,
							       family, 0, K_NO_WAIT);
			if (!rcv_pkt) {
				LOG_ERR("Cannot allocate rcv packet");
				k_sleep(K_MSEC(200));

				continue;
			}

			if (net_pkt_write(rcv_pkt, (uint8_t *)receive_buffer, recv_data_len)) {
				LOG_ERR("Cannot write pkt - dropped packet");
				net_pkt_unref(rcv_pkt);
			} else {
				ret = net_recv_data(nrf91_modem_ctx.iface, rcv_pkt);
				if (ret < 0) {
					LOG_ERR("Packet dropped by NET stack: %d", ret);
					net_pkt_unref(rcv_pkt);
				}
			}
		} else {
			LOG_WRN("nrf_recv() failed, error: %d", -errno);
		}
	}
}

K_THREAD_DEFINE(nrf91_modem_dl_data_thread, NRF91_MODEM_DATA_DL_THREAD_STACK_SIZE,
		nrf91_modem_recv_thread, NULL, NULL, NULL,
		NRF91_MODEM_DATA_RCV_THREAD_PRIORITY, 0, 0);

/******************************************************************************/

static struct dummy_api nrf91_modem_if_api = {
	.iface_api.init = nrf91_iface_init,
	.send = nrf91_modem_send,
};

/* Set up a dummy L2 per Zephyr requirement, as the real L2 lives in the modem */
#define NRF91_L2_LAYER		DUMMY_L2
#define NRF91_L2_CTX_TYPE	NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(nrf91_modem_if, "nrf91_modem_if_dev",
		nrf91_init, NULL,
		&nrf91_modem_ctx, NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&nrf91_modem_if_api, NRF91_L2_LAYER, NRF91_L2_CTX_TYPE,
		NRF91_MTU_DEFAULT);

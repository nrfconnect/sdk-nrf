/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */

/* nRF91 usage without offloading TCP/IP stack to modem,
 * i.e. using Zephyr native TCP/IP stack.
 */

#include <zephyr/kernel.h>

#include <zephyr/random/random.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

#include <zephyr/posix/arpa/inet.h>

#include <nrf_socket.h>

#include <modem/pdn.h>
#include <nrf_modem_at.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>

#include "link_shell_pdn.h"
#include "net_utils.h"
#include "mosh_print.h"

#define NO_MDM_SCKT -1

#define NRF91_MTU 1500

K_SEM_DEFINE(mdm_socket_sem, 0, 1);

/* Work for creating modem data socket (uses system workq): */
static struct k_work_delayable mdm_socket_work;

struct nrf91_non_offload_dev_context {
	uint8_t mac_addr[6];
	struct net_if *iface;
	struct in_addr ipv4_addr;
	bool default_pdp_active;
	int mdm_skct_id;
};

static struct nrf91_non_offload_dev_context nrf91_non_offload_iface_data;

/******************************************************************************/

static void util_get_ip_addr(int cid, char *addr4, char *addr6)
{
	int ret;
	char cmd[128];
	char tmp[sizeof(struct in6_addr)];
	char addr1[NET_IPV6_ADDR_LEN] = { 0 };
	char addr2[NET_IPV6_ADDR_LEN] = { 0 };

	sprintf(cmd, "AT+CGPADDR=%d", cid);

	/** parse +CGPADDR: <cid>,<PDP_addr_1>,<PDP_addr_2>
	 * PDN type "IP": PDP_addr_1 is <IPv4>, max 16(INET_ADDRSTRLEN), '.' and digits
	 * PDN type "IPV6": PDP_addr_1 is <IPv6>, max 46(INET6_ADDRSTRLEN),':', digits, 'A'~'F'
	 * PDN type "IPV4V6": <IPv4>,<IPv6>, or <IPV4> or <IPv6>
	 */
	ret = nrf_modem_at_scanf(cmd, "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"", addr1,
				 addr2);
	if (ret <= 0) {
		return;
	}

	/* parse 1st IP string */
	if (addr4 != NULL && inet_pton(AF_INET, addr1, tmp) == 1) {
		strcpy(addr4, addr1);
	} else if (addr6 != NULL && inet_pton(AF_INET6, addr1, tmp) == 1) {
		strcpy(addr6, addr1);
		return;
	}

	/* parse second IP string (IPv6 only) */
	if (addr6 == NULL) {
		return;
	}
	if (ret > 1 && inet_pton(AF_INET6, addr2, tmp) == 1) {
		strcpy(addr6, addr2);
	}
}

/******************************************************************************/

struct pdn_conn_dyn_params {
	struct in_addr dns_addr4_primary;
	struct in_addr dns_addr4_secondary;
	struct in6_addr dns_addr6_primary;
	struct in6_addr dns_addr6_secondary;
	uint32_t ipv4_mtu;
	uint32_t ipv6_mtu;
};
#define AT_CMD_PDP_CONTEXT_READ_DYN_INFO "AT+CGCONTRDP=%d"
#define AT_CMD_PDP_CONTEXT_READ_DYN_INFO_PARAM_COUNT 20
#define AT_CMD_PDP_CONTEXT_READ_DYN_INFO_DNS_ADDR_PRIMARY_INDEX 6
#define AT_CMD_PDP_CONTEXT_READ_DYN_INFO_DNS_ADDR_SECONDARY_INDEX 7
#define AT_CMD_PDP_CONTEXT_READ_DYN_INFO_MTU_INDEX 12

static int util_get_pdn_conn_dyn_params(int cid, struct pdn_conn_dyn_params *ret_dyn_info)
{
	struct at_param_list param_list = { 0 };
	size_t param_str_len;
	char *next_param_str;
	char at_cmd_str[16];
	char at_cmd_response_str[512];
	char dns_addr_str[128];
	char *at_ptr = at_cmd_response_str;
	int ret, family, iterator;
	struct in_addr *addr;
	struct in6_addr *addr6;
	bool resp_continues = true;
	uint32_t mtu;
	int family_for_mtu;

	sprintf(at_cmd_str, AT_CMD_PDP_CONTEXT_READ_DYN_INFO, cid);

	ret = nrf_modem_at_cmd(at_cmd_response_str, sizeof(at_cmd_response_str), at_cmd_str);
	if (ret) {
		mosh_error("Cannot get PDP conn dyn params, ret: %d", ret);
		return false;
	}

	/* Parse the response */
	ret = at_params_list_init(&param_list, AT_CMD_PDP_CONTEXT_READ_DYN_INFO_PARAM_COUNT);
	if (ret) {
		mosh_error("Could not init AT params list, error: %d\n", ret);
		return ret;
	}

	iterator = 0;
	ret_dyn_info->ipv4_mtu = NRF91_MTU;
	ret_dyn_info->ipv6_mtu = NRF91_MTU;
	while (resp_continues) {
		resp_continues = false;
		ret = at_parser_max_params_from_str(at_ptr, &next_param_str, &param_list,
						    AT_CMD_PDP_CONTEXT_READ_DYN_INFO_PARAM_COUNT);
		if (ret == -EAGAIN) {
			resp_continues = true;
		} else if (ret == -E2BIG) {
			mosh_error("E2BIG, error: %d\n", ret);
		} else if (ret != 0) {
			mosh_error("Could not parse AT response for %s, error: %d\n", at_cmd_str,
				   ret);
			goto clean_exit;
		}

		/* Read primary DNS address */
		param_str_len = sizeof(dns_addr_str);
		ret = at_params_string_get(&param_list,
					   AT_CMD_PDP_CONTEXT_READ_DYN_INFO_DNS_ADDR_PRIMARY_INDEX,
					   dns_addr_str, &param_str_len);
		if (ret) {
			mosh_error("Could not parse dns str for cid %d, err: %d", cid, ret);
			goto clean_exit;
		}
		dns_addr_str[param_str_len] = '\0';

		family = net_utils_sa_family_from_ip_string(dns_addr_str);
		if (family == AF_INET) {
			addr = &(ret_dyn_info->dns_addr4_primary);
			(void)inet_pton(AF_INET, dns_addr_str, addr);
		} else if (family == AF_INET6) {
			addr6 = &(ret_dyn_info->dns_addr6_primary);
			(void)inet_pton(AF_INET6, dns_addr_str, addr6);
		}
		/* Primary DNS address used for checking IP family for MTU */
		family_for_mtu = family;

		/* Read secondary DNS address */
		param_str_len = sizeof(dns_addr_str);

		ret = at_params_string_get(
			&param_list, AT_CMD_PDP_CONTEXT_READ_DYN_INFO_DNS_ADDR_SECONDARY_INDEX,
			dns_addr_str, &param_str_len);
		if (ret) {
			mosh_error("Could not parse dns str, err: %d", ret);
			goto clean_exit;
		}
		dns_addr_str[param_str_len] = '\0';

		family = net_utils_sa_family_from_ip_string(dns_addr_str);
		if (family == AF_INET) {
			addr = &(ret_dyn_info->dns_addr4_secondary);
			(void)inet_pton(AF_INET, dns_addr_str, addr);
		} else if (family == AF_INET6) {
			addr6 = &(ret_dyn_info->dns_addr6_secondary);
			(void)inet_pton(AF_INET6, dns_addr_str, addr6);
		}

		/* Read link MTU if exists: */
		ret = at_params_int_get(&param_list,
					AT_CMD_PDP_CONTEXT_READ_DYN_INFO_MTU_INDEX,
					&mtu);
		if (ret) {
			/* Don't care if it fails and default MTUs have been set */
			ret = 0;
		} else {
			/* Note: if no primary DNS address in response,
			 * then family cannot be known for MTU from AT response and
			 * we use defaults for both families that have been set earlier.
			 */
			if (family_for_mtu == AF_INET) {
				ret_dyn_info->ipv4_mtu = mtu;
			} else if (family_for_mtu == AF_INET6) {
				ret_dyn_info->ipv6_mtu = mtu;
			}
		}

		if (resp_continues) {
			at_ptr = next_param_str;
			iterator++;
		}
	}

clean_exit:
	at_params_list_free(&param_list);

	return ret;
}

/******************************************************************************/

static void nrf91_non_offload_pdn_event_handler(uint8_t cid, enum pdn_event event, int reason)
{
	/* Only default PDN context is supported */
	if (cid == 0) {
		if (event == PDN_EVENT_ACTIVATED) {
			nrf91_non_offload_iface_data.default_pdp_active = true;
			k_work_schedule(&mdm_socket_work, K_SECONDS(2));
		} else if (event == PDN_EVENT_DEACTIVATED) {
			nrf91_non_offload_iface_data.default_pdp_active = false;
			k_work_schedule(&mdm_socket_work, K_NO_WAIT);
		}
	}
}

/******************************************************************************/

static void nrf91_non_offload_socket_create(void)
{
	int ret;

	ret = nrf_socket(NRF_AF_PACKET, NRF_SOCK_RAW, 0);
	if (ret < 0) {
		mosh_error("nrf_socket failed %d\n", ret);
		ret = NO_MDM_SCKT;
		k_sem_reset(&mdm_socket_sem);
	} else {
		k_sem_give(&mdm_socket_sem);
	}
	nrf91_non_offload_iface_data.mdm_skct_id = ret;
}

static void nrf91_non_offload_socket_close(void)
{
	int ret;

	k_sem_reset(&mdm_socket_sem);
	nrf_close(nrf91_non_offload_iface_data.mdm_skct_id);
	nrf91_non_offload_iface_data.mdm_skct_id = NO_MDM_SCKT;
	ret = net_if_ipv4_addr_rm(nrf91_non_offload_iface_data.iface,
				  &nrf91_non_offload_iface_data.ipv4_addr);
	if (!ret) {
		mosh_error("Cannot remove IPv4 address from interface %d",
			net_if_get_by_iface(nrf91_non_offload_iface_data.iface));
	}
}

static void nrf91_non_offload_mdm_socket_worker(struct k_work *unused)
{
	ARG_UNUSED(unused);
	if (nrf91_non_offload_iface_data.default_pdp_active) {
		if (nrf91_non_offload_iface_data.mdm_skct_id == NO_MDM_SCKT) {
			char ipv4_addr[NET_IPV4_ADDR_LEN] = { 0 };
			char ipv6_addr[NET_IPV6_ADDR_LEN] = { 0 };
			struct sockaddr addr;
			struct net_if_addr *ifaddr;
			struct pdn_conn_dyn_params pdn_dyn_info;
			int len, ret;

			nrf91_non_offload_socket_create();
			util_get_ip_addr(0, ipv4_addr, ipv6_addr);
			if (IS_ENABLED(CONFIG_NET_IPV4)) {
				len = strlen(ipv4_addr);
				if (len == 0) {
					mosh_error("Unable to obtain local IPv4 address");
					return;
				}
				ret = net_ipaddr_parse(ipv4_addr, len, &addr);
				if (!ret) {
					mosh_error("Unable to parse IPv4 address");
					return;
				}
				ifaddr = net_if_ipv4_addr_add(nrf91_non_offload_iface_data.iface,
							      &net_sin(&addr)->sin_addr,
							      NET_ADDR_MANUAL, 0);
				if (!ifaddr) {
					mosh_error("Cannot add %s to interface", ipv4_addr);
					return;
				}
				nrf91_non_offload_iface_data.ipv4_addr = net_sin(&addr)->sin_addr;
			}
			/* TODO: IPv6 */

			ret = util_get_pdn_conn_dyn_params(0, &pdn_dyn_info);
			if (ret) {
				mosh_warn("Couldn't get PDN dyn info");
			} else {
				/* Set LTE link MTU also to non-offloading net if */
				net_if_set_mtu(nrf91_non_offload_iface_data.iface,
					       pdn_dyn_info.ipv4_mtu);

#if defined(CONFIG_DNS_RESOLVER) && !defined(CONFIG_DNS_SERVER_IP_ADDRESSES)
				/* Dynamic DNS servers and set them on zephyr stack */
				struct dns_resolve_context *dnsctx;
				struct sockaddr_in dns1 = {
					.sin_family = AF_INET,
					.sin_port = htons(53),
					.sin_addr = pdn_dyn_info.dns_addr4_primary
				};
				struct sockaddr_in dns2 = {
					.sin_family = AF_INET,
					.sin_port = htons(53),
					.sin_addr = pdn_dyn_info.dns_addr4_secondary
				};
				const struct sockaddr *dns_servers[] = {
					(struct sockaddr *) &dns1,
					(struct sockaddr *) &dns2,
					NULL
				};

				dnsctx = dns_resolve_get_default();
				ret = dns_resolve_reconfigure(dnsctx, NULL, dns_servers);
				if (ret < 0) {
					mosh_error("Could not set DNS servers");
				}
#endif
			}

			/* TODO: support for multiple net interfaces within non-offloading device,
			 * I.e. each for created PDN context
			 */
		}
	} else {
		nrf91_non_offload_socket_close();
	}
}

/******************************************************************************/

static uint8_t *util_fake_dev_get_mac(struct nrf91_non_offload_dev_context *ctx)
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

static void nrf91_non_offload_iface_init(struct net_if *iface)
{
	struct nrf91_non_offload_dev_context *ctx = net_if_get_device(iface)->data;
	uint8_t *mac = util_fake_dev_get_mac(ctx);

	ctx->iface = iface;
	ctx->default_pdp_active = false;
	ctx->mdm_skct_id = NO_MDM_SCKT;

	/* The mac address is not really used but net if expects to find one. */
	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);
}

static int nrf91_non_offload_init(const struct device *arg)
{
	int err = 0;

	ARG_UNUSED(arg);

	k_work_init_delayable(&mdm_socket_work, nrf91_non_offload_mdm_socket_worker);

	err = link_shell_pdn_event_forward_cb_set(nrf91_non_offload_pdn_event_handler);
	if (err) {
		mosh_error("link_shell_pdn_event_forward_cb_set() failed, err %d\n", err);
		return -1;
	}
	return 0;
}

/******************************************************************************/

static K_MUTEX_DEFINE(send_buf_lock);

#define NRF91_MODEM_DATA_UL_BUFFER_SIZE 1500
static char send_buffer[NRF91_MODEM_DATA_UL_BUFFER_SIZE];

static int nrf91_non_offload_iface_send(const struct device *dev, struct net_pkt *pkt)
{
	int ret, data_len;

	k_mutex_lock(&send_buf_lock, K_FOREVER);
	data_len = net_pkt_get_len(pkt);
	ret = net_pkt_read(pkt, send_buffer, data_len);
	if (ret < 0) {
		mosh_error("%s: cannot read packet: %d, from pkt %p\n", __func__, ret, pkt);
	} else {
		struct nrf91_non_offload_dev_context *ctx = dev->data;

		ret = nrf_send(ctx->mdm_skct_id, send_buffer, data_len, 0);
		if (ret < 0) {
			mosh_error("%s: send() failed: (%d), data len: %d\n", __func__, -errno,
			       data_len);
		} else if (ret != data_len) {
			mosh_error("%s: only partially sent, only %d of original %d was sent",
				__func__,
				ret, data_len);
		}
	}
	net_pkt_unref(pkt);
	k_mutex_unlock(&send_buf_lock);

	return ret;
}

/******************************************************************************/

#define NRF91_MODEM_DATA_DL_BUFFER_SIZE 1500

static char receive_buffer[NRF91_MODEM_DATA_DL_BUFFER_SIZE];

static void nrf91_non_offload_modem_dl_data_thread_handler(void)
{
	struct net_pkt *rcv_pkt;

	int ret = 0;
	int recv_data_len = 0;

	while (true) {
		if (nrf91_non_offload_iface_data.mdm_skct_id < 0) {
			/* Wait for sockets to be created */
			k_sem_take(&mdm_socket_sem, K_FOREVER);
			continue;
		}

		recv_data_len = nrf_recv(nrf91_non_offload_iface_data.mdm_skct_id,
					 receive_buffer, sizeof(receive_buffer), 0);
		if (recv_data_len > 0) {
			rcv_pkt = net_pkt_rx_alloc_with_buffer(
					nrf91_non_offload_iface_data.iface, recv_data_len,
					AF_UNSPEC, 0, K_NO_WAIT);
			if (!rcv_pkt) {
				mosh_error("%s: cannot allocate rcv packet\n", (__func__));
				k_sleep(K_MSEC(200));
				continue;
			}
			if (net_pkt_write(rcv_pkt, (uint8_t *)receive_buffer,
			    recv_data_len)) {
				mosh_error("%s: cannot write pkt %p - dropped packet\n",
				(__func__), rcv_pkt);
				net_pkt_unref(rcv_pkt);
			} else {
				ret = net_recv_data(
					nrf91_non_offload_iface_data.iface,
					rcv_pkt);
				if (ret < 0) {
					mosh_error(
						"%s: received packet dropped by NET stack, ret %d",
						(__func__), ret);
					net_pkt_unref(rcv_pkt);
				}
			}
		} else {
			/* nrf_recv() failed, LTE connection might have dropped.
			 * Let other threads run and to update possible mdm_socket_sem.
			 */
			k_sleep(K_MSEC(200));
		}
	}
}

#define NRF91_MODEM_DATA_DL_THREAD_STACK_SIZE 2048
#define NRF91_MODEM_DATA_RCV_THREAD_PRIORITY K_PRIO_COOP(10) /* -6 */

K_THREAD_DEFINE(nrf91_modem_dl_data_thread, NRF91_MODEM_DATA_DL_THREAD_STACK_SIZE,
		nrf91_non_offload_modem_dl_data_thread_handler, NULL, NULL, NULL,
		NRF91_MODEM_DATA_RCV_THREAD_PRIORITY, 0, 0);

/******************************************************************************/

static struct dummy_api nrf91_non_offload_if_api = {
	.iface_api.init = nrf91_non_offload_iface_init,
	.send = nrf91_non_offload_iface_send,
};

/* No L2 */
#define NRF91_L2_LAYER DUMMY_L2
#define NRF91_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(nrf91_non_offload, "nrf91_nrf_non_offloading_dev",
		nrf91_non_offload_init, NULL,
		&nrf91_non_offload_iface_data, NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&nrf91_non_offload_if_api, NRF91_L2_LAYER, NRF91_L2_CTX_TYPE, NRF91_MTU);

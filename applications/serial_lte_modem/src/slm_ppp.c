/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "slm_ppp.h"
#include "slm_at_host.h"
#include "slm_util.h"
#if defined(CONFIG_SLM_CMUX)
#include "slm_cmux.h"
#endif
#include <modem/lte_lc.h>
#include <modem/pdn.h>
#include <zephyr/modem/ppp.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ppp.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/random/random.h>
#include <assert.h>

LOG_MODULE_REGISTER(slm_ppp, CONFIG_SLM_LOG_LEVEL);

/* This keeps track of whether the user is registered to the CGEV notifications.
 * We need them to know when to start/stop the PPP link, but that should not
 * influence what the user receives, so we do the filtering based on this.
 */
bool slm_fwd_cgev_notifs;

#if defined(CONFIG_SLM_CMUX)
BUILD_ASSERT(!DT_NODE_EXISTS(DT_CHOSEN(ncs_slm_ppp_uart)),
	"When CMUX is enabled PPP is usable only through it so it cannot have its own UART.");
#else
static const struct device *ppp_uart_dev = DEVICE_DT_GET(DT_CHOSEN(ncs_slm_ppp_uart));
#endif
static struct net_if *ppp_iface;

static uint8_t ppp_data_buf[1500];
static struct sockaddr_ll ppp_zephyr_dst_addr;

static struct k_thread ppp_data_passing_thread_id;
static K_THREAD_STACK_DEFINE(ppp_data_passing_thread_stack, KB(2));
static void ppp_data_passing_thread(void*, void*, void*);

static void ppp_controller(struct k_work *work);
enum ppp_action {
	PPP_START,
	PPP_RESTART,
	PPP_STOP
};
struct ppp_work {
	struct k_work work;
	enum ppp_action action;
};
static struct ppp_work ppp_start_work = {
	.work = Z_WORK_INITIALIZER(ppp_controller),
	.action = PPP_START
};
static struct ppp_work ppp_restart_work = {
	.work = Z_WORK_INITIALIZER(ppp_controller),
	.action = PPP_RESTART
};
static struct ppp_work ppp_stop_work = {
	.work = Z_WORK_INITIALIZER(ppp_controller),
	.action = PPP_STOP
};

static bool ppp_peer_connected;

enum ppp_states {
	PPP_STATE_STOPPED,
	PPP_STATE_STARTING,
	PPP_STATE_RUNNING,
	PPP_STATE_STOPPING
};
static atomic_t ppp_state;

MODEM_PPP_DEFINE(ppp_module, NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		 sizeof(ppp_data_buf), sizeof(ppp_data_buf));

static struct modem_pipe *ppp_pipe;

/* We use only the default PDP context. */
enum { PDP_CID = 0 };

enum {
	ZEPHYR_FD_IDX, /* Raw Zephyr socket to pass data to/from the PPP link. */
	MODEM_FD_IDX, /* Raw modem socket to pass data to/from the LTE link. */
	PPP_FDS_COUNT
};
const char *const ppp_socket_names[PPP_FDS_COUNT] = {
	[ZEPHYR_FD_IDX] = "Zephyr",
	[MODEM_FD_IDX] = "modem"
};
static int ppp_fds[PPP_FDS_COUNT] = { -1, -1 };

static bool open_ppp_sockets(void)
{
	int ret;

	ppp_fds[ZEPHYR_FD_IDX] = socket(AF_PACKET, SOCK_RAW | SOCK_NATIVE,
				        htons(IPPROTO_RAW));
	if (ppp_fds[ZEPHYR_FD_IDX] < 0) {
		LOG_ERR("Zephyr socket creation failed (%d).", errno);
		return false;
	}

	ppp_zephyr_dst_addr = (struct sockaddr_ll){
		.sll_family = AF_PACKET,
		.sll_ifindex = net_if_get_by_iface(ppp_iface)
	};
	ret = bind(ppp_fds[ZEPHYR_FD_IDX],
		   (const struct sockaddr *)&ppp_zephyr_dst_addr, sizeof(ppp_zephyr_dst_addr));
	if (ret < 0) {
		LOG_ERR("Failed to bind Zephyr socket (%d).", errno);
		return false;
	}

	ppp_fds[MODEM_FD_IDX] = socket(AF_PACKET, SOCK_RAW, 0);
	if (ppp_fds[MODEM_FD_IDX] < 0) {
		LOG_ERR("Modem socket creation failed (%d).", errno);
		return false;
	}

	return true;
}

static void close_ppp_sockets(void)
{
	for (size_t i = 0; i != ARRAY_SIZE(ppp_fds); ++i) {
		if (ppp_fds[i] < 0) {
			continue;
		}
		if (close(ppp_fds[i])) {
			LOG_WRN("Failed to close %s socket (%d).",
				ppp_socket_names[i], errno);
		}
		ppp_fds[i] = -1;
	}
}

static bool configure_ppp_link_ip_addresses(struct ppp_context *ctx)
{
	static uint8_t ppp_ll_addr[PPP_INTERFACE_IDENTIFIER_LEN];
	uint8_t ll_addr_len;
	char addr4[INET_ADDRSTRLEN];
	char addr6[INET6_ADDRSTRLEN];

	util_get_ip_addr(PDP_CID, addr4, addr6);

	if (*addr4) {
		if (inet_pton(AF_INET, addr4, &ctx->ipcp.my_options.address) != 1) {
			return false;
		}
	} else if (!*addr6) {
		LOG_ERR("No connectivity.");
		return false;
	}

	if (*addr6) {
		struct in6_addr in6;

		if (inet_pton(AF_INET6, addr6, &in6) != 1) {
			return false;
		}
		/* The interface identifier is the last 64 bits of the IPv6 address. */
		BUILD_ASSERT(sizeof(in6) >= sizeof(ppp_ll_addr));
		ll_addr_len = sizeof(ppp_ll_addr);
		memcpy(ppp_ll_addr, (uint8_t *)(&in6 + 1) - ll_addr_len, ll_addr_len);
	} else {
		/* 00-00-5E-00-53-xx as per RFC 7042, as zephyr/drivers/net/ppp.c does. */
		ll_addr_len = 6;
		ppp_ll_addr[0] = 0x00;
		ppp_ll_addr[1] = 0x00;
		ppp_ll_addr[2] = 0x5E;
		ppp_ll_addr[3] = 0x00;
		ppp_ll_addr[4] = 0x53;
		ppp_ll_addr[5] = sys_rand32_get();
	}
	net_if_set_link_addr(ppp_iface, ppp_ll_addr, ll_addr_len, NET_LINK_UNKNOWN);

	return true;
}

static bool ppp_is_running(void)
{
	return (atomic_get(&ppp_state) == PPP_STATE_RUNNING);
}

static void send_status_notification(void)
{
	rsp_send("\r\n#XPPP: %u,%u\r\n", ppp_is_running(), ppp_peer_connected);
}

static int ppp_start_failure(int ret)
{
	close_ppp_sockets();
	net_if_down(ppp_iface);
	return ret;
}

static int ppp_start_internal(void)
{
	int ret;
	unsigned int mtu;
	struct ppp_context *const ctx = net_if_l2_data(ppp_iface);

	if (!configure_ppp_link_ip_addresses(ctx)) {
		return -EADDRNOTAVAIL;
	}

	ret = pdn_dynamic_params_get(PDP_CID, &ctx->ipcp.my_options.dns1_address,
				     &ctx->ipcp.my_options.dns2_address, &mtu);
	if (ret) {
		return ret;
	}

	if (mtu) {
		/* Set the PPP MTU to that of the LTE link. */
		mtu = MIN(mtu, sizeof(ppp_data_buf));
	} else {
		LOG_DBG("Could not retrieve MTU, using fallback value.");
		mtu = CONFIG_SLM_PPP_FALLBACK_MTU;
		BUILD_ASSERT(sizeof(ppp_data_buf) >= CONFIG_SLM_PPP_FALLBACK_MTU);
	}

	net_if_set_mtu(ppp_iface, mtu);
	LOG_DBG("MTU set to %u.", mtu);

	ret = net_if_up(ppp_iface);
	if (ret) {
		LOG_ERR("Failed to bring PPP interface up (%d).", ret);
		return ret;
	}

	if (!open_ppp_sockets()) {
		return ppp_start_failure(-ENOTCONN);
	}

#if defined(CONFIG_SLM_CMUX)
	ppp_pipe = slm_cmux_reserve(CMUX_PPP_CHANNEL);
	/* The pipe opening is managed by CMUX. */
#endif

	modem_ppp_attach(&ppp_module, ppp_pipe);

#if !defined(CONFIG_SLM_CMUX)
	ret = modem_pipe_open(ppp_pipe);
	if (ret) {
		LOG_ERR("Failed to open PPP pipe (%d).", ret);
		return ppp_start_failure(ret);
	}
#endif

	net_if_carrier_on(ppp_iface);

	LOG_INF("PPP started.");

	k_thread_create(&ppp_data_passing_thread_id, ppp_data_passing_thread_stack,
			K_THREAD_STACK_SIZEOF(ppp_data_passing_thread_stack),
			ppp_data_passing_thread, NULL, NULL, NULL,
			K_PRIO_COOP(10), 0, K_NO_WAIT);
	k_thread_name_set(&ppp_data_passing_thread_id, "ppp_data_passing");

	return 0;
}

bool slm_ppp_is_stopped(void)
{
	return (atomic_get(&ppp_state) == PPP_STATE_STOPPED);
}

static void ppp_start(void)
{
	if (atomic_cas(&ppp_state, PPP_STATE_STOPPED, PPP_STATE_STARTING)) {
		if (ppp_start_internal()) {
			atomic_set(&ppp_state, PPP_STATE_STOPPED);
		} else {
			atomic_set(&ppp_state, PPP_STATE_RUNNING);
			send_status_notification();
		}
	}
}

static void ppp_stop_internal(void)
{
	LOG_DBG("Stopping PPP...");

	/* Bring the interface down before releasing pipes and carrier.
	 * This is needed for LCP to notify the remote endpoint that the link is going down.
	 */
	const int ret = net_if_down(ppp_iface);

	if (ret) {
		LOG_WRN("Failed to bring PPP interface down (%d).", ret);
	}

#if !defined(CONFIG_SLM_CMUX)
	modem_pipe_close(ppp_pipe);
#endif

	modem_ppp_release(&ppp_module);

#if defined(CONFIG_SLM_CMUX)
	slm_cmux_release(CMUX_PPP_CHANNEL);
#endif

	net_if_carrier_off(ppp_iface);

	close_ppp_sockets();

	k_thread_join(&ppp_data_passing_thread_id, K_SECONDS(1));

	LOG_INF("PPP stopped.");
}

static void ppp_stop(void)
{
	if (atomic_cas(&ppp_state, PPP_STATE_RUNNING, PPP_STATE_STOPPING)) {
		ppp_stop_internal();
		atomic_set(&ppp_state, PPP_STATE_STOPPED);
		send_status_notification();
	}
}

/* Automatically starts/stops PPP when the default PDN connection goes up/down. */
static void pdp_ctx_event_handler(uint8_t cid, enum pdn_event event, int reason)
{
	switch (event) {
	case PDN_EVENT_ACTIVATED:
		LOG_INF("Connection up. Starting PPP.");
		k_work_submit_to_queue(&slm_work_q, &ppp_restart_work.work);
		break;
	case PDN_EVENT_DEACTIVATED:
		LOG_DBG("Connection down.");
		ppp_stop();
		break;
	default:
		LOG_DBG("Default PDN connection event %d received.", event);
		break;
	}
}

/* We need to receive CGEV notifications at all times.
 * CGEREP AT commands are intercepted to prevent the user
 * from unsubcribing us and make that behavior invisible.
 */
AT_CMD_CUSTOM(at_cgerep_interceptor, "AT+CGEREP", at_cgerep_callback);

static int at_cgerep_callback(char *buf, size_t len, char *at_cmd)
{
	int ret;
	unsigned int subscribe;
	const bool set_cmd = (sscanf(at_cmd, "AT+CGEREP=%u", &subscribe) == 1);

	/* The modem interprets AT+CGEREP and AT+CGEREP= as AT+CGEREP=0.
	 * Prevent those forms, only allowing AT+CGEREP=0, for simplicty.
	 */
	if (!set_cmd && (!strcmp(at_cmd, "AT+CGEREP") || !strcmp(at_cmd, "AT+CGEREP="))) {
		LOG_ERR("The syntax %s is disallowed. Use AT+CGEREP=0 instead.", at_cmd);
		return -EINVAL;
	}
	if (!set_cmd || subscribe) {
		/* Forward the command to the modem only if not unsubscribing. */
		ret = slm_util_at_cmd_no_intercept(buf, len, at_cmd);
		if (ret) {
			return ret;
		}
		/* Modify the output of the read command to reflect the user's
		 * subscription status, not that of the SLM.
		 */
		if (at_cmd[strlen("AT+CGEREP")] == '?') {
			const size_t mode_idx = strlen("+CGEREP: ");

			if (mode_idx < len) {
				/* +CGEREP: <mode>,<bfr> */
				buf[mode_idx] = '0' + slm_fwd_cgev_notifs;
			}
		}
	} else { /* AT+CGEREP=0 */
		snprintf(buf, len, "%s", "OK\r\n");
	}

	if (set_cmd) {
		slm_fwd_cgev_notifs = subscribe;
	}
	return 0;
}

static void subscribe_cgev_notifications(void)
{
	char buf[sizeof("\r\nOK")];

	/* Bypass the CGEREP interception above as it is meant for commands received externally. */
	const int ret = slm_util_at_cmd_no_intercept(buf, sizeof(buf), "AT+CGEREP=1");

	if (ret) {
		LOG_ERR("Failed to subscribe to +CGEV notifications (%d).", ret);
	}
}

/* Notification subscriptions are reset on CFUN=0.
 * We intercept CFUN set commands to automatically subscribe.
 */
AT_CMD_CUSTOM(at_cfun_set_interceptor, "AT+CFUN=", at_cfun_set_callback);

static int at_cfun_set_callback(char *buf, size_t len, char *at_cmd)
{
	unsigned int mode;
	const int ret = slm_util_at_cmd_no_intercept(buf, len, at_cmd);

	/* sscanf() doesn't match if this is a test command (it also gets intercepted). */
	if (ret || sscanf(at_cmd, "AT+CFUN=%u", &mode) != 1) {
		/* The functional mode cannot have changed. */
		return ret;
	}

	if (mode == LTE_LC_FUNC_MODE_NORMAL || mode == LTE_LC_FUNC_MODE_ACTIVATE_LTE) {
		subscribe_cgev_notifications();
	} else if (mode == LTE_LC_FUNC_MODE_POWER_OFF) {
		/* Unsubscribe the user as would normally happen. */
		slm_fwd_cgev_notifs = false;
	}
	return 0;
}

static void ppp_controller(struct k_work *work)
{
	struct ppp_work *const ppp_work = CONTAINER_OF(work, struct ppp_work, work);

	switch (ppp_work->action) {
	case PPP_START:
		ppp_start();
		break;
	case PPP_RESTART:
		ppp_stop();
		ppp_start();
		break;
	case PPP_STOP:
		ppp_stop();
		break;
	default:
		LOG_ERR("Unknown PPP action: %d.", ppp_work->action);
		break;
	}
}

static void ppp_net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				       uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_PPP_PHASE_RUNNING:
		LOG_INF("Peer connected.");
		ppp_peer_connected = true;
		send_status_notification();
		break;
	case NET_EVENT_PPP_PHASE_DEAD:
		LOG_DBG("Peer not connected.");
		/* This event can come without prior NET_EVENT_PPP_PHASE_RUNNING. */
		if (!ppp_peer_connected) {
			break;
		}
		ppp_peer_connected = false;
		/* Also ignore this event when PPP is not running anymore. */
		if (!ppp_is_running()) {
			break;
		}
		send_status_notification();
		/* For the peer to be able to successfully reconnect
		 * (handshake issues observed with pppd and Windows dial-up),
		 * for some reason the Zephyr PPP link needs to be restarted.
		 */
		LOG_INF("Peer disconnected. Restarting PPP...");
		k_work_submit_to_queue(&slm_work_q, &ppp_restart_work.work);
		break;
	}
}

int slm_ppp_init(void)
{
#if !defined(CONFIG_SLM_CMUX)
	if (!device_is_ready(ppp_uart_dev)) {
		return -EAGAIN;
	}

	{
		static struct modem_backend_uart ppp_uart_backend;
		static uint8_t ppp_uart_backend_receive_buf[sizeof(ppp_data_buf)];
		static uint8_t ppp_uart_backend_transmit_buf[sizeof(ppp_data_buf)];

		const struct modem_backend_uart_config uart_backend_config = {
			.uart = ppp_uart_dev,
			.receive_buf = ppp_uart_backend_receive_buf,
			.receive_buf_size = sizeof(ppp_uart_backend_receive_buf),
			.transmit_buf = ppp_uart_backend_transmit_buf,
			.transmit_buf_size = sizeof(ppp_uart_backend_transmit_buf),
		};

		ppp_pipe = modem_backend_uart_init(&ppp_uart_backend, &uart_backend_config);
		if (!ppp_pipe) {
			return -ENOSYS;
		}
	}
#endif

	ppp_iface = modem_ppp_get_iface(&ppp_module);

	net_if_flag_set(ppp_iface, NET_IF_POINTOPOINT);

	pdn_default_ctx_cb_reg(pdp_ctx_event_handler);

	{
		static struct net_mgmt_event_callback ppp_net_mgmt_event_cb;

		net_mgmt_init_event_callback(&ppp_net_mgmt_event_cb, ppp_net_mgmt_event_handler,
					    NET_EVENT_PPP_PHASE_RUNNING | NET_EVENT_PPP_PHASE_DEAD);
		net_mgmt_add_event_callback(&ppp_net_mgmt_event_cb);
	}

	LOG_DBG("PPP initialized.");
	return 0;
}

SLM_AT_CMD_CUSTOM(xppp, "AT#XPPP", handle_at_ppp);
static int handle_at_ppp(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			 uint32_t param_count)
{
	int ret;
	unsigned int op;
	enum {
		OP_STOP,
		OP_START,
		OP_COUNT
	};

	if (cmd_type == AT_PARSER_CMD_TYPE_READ) {
		send_status_notification();
		return 0;
	}
	if (cmd_type != AT_PARSER_CMD_TYPE_SET || param_count != 2) {
		return -EINVAL;
	}

	ret = at_parser_num_get(parser, 1, &op);
	if (ret) {
		return ret;
	} else if (op >= OP_COUNT) {
		return -EINVAL;
	}

	/* Send "OK" first in case stopping PPP results in the CMUX AT channel switching. */
	rsp_send_ok();
	if (op == OP_START) {
		k_work_submit_to_queue(&slm_work_q, &ppp_start_work.work);
	} else {
		k_work_submit_to_queue(&slm_work_q, &ppp_stop_work.work);
	}
	return -SILENT_AT_COMMAND_RET;
}

static void ppp_data_passing_thread(void*, void*, void*)
{
	const size_t mtu = net_if_get_mtu(ppp_iface);
	struct pollfd fds[PPP_FDS_COUNT];

	for (size_t i = 0; i != ARRAY_SIZE(fds); ++i) {
		fds[i].fd = ppp_fds[i];
		fds[i].events = POLLIN;
	}

	while (true) {
		const int poll_ret = poll(fds, ARRAY_SIZE(fds), -1);

		if (poll_ret <= 0) {
			LOG_ERR("Sockets polling failed (%d, %d).", poll_ret, errno);
			ppp_stop();
			return;
		}

		for (size_t src = 0; src != ARRAY_SIZE(fds); ++src) {
			const short revents = fds[src].revents;

			if (!revents) {
				continue;
			}
			if (!(revents & POLLIN)) {
				/* POLLERR/POLLNVAL happen when the sockets are closed
				 * or when the connection goes down.
				 */
				if ((revents ^ POLLERR) && (revents ^ POLLNVAL)) {
					LOG_WRN("Unexpected event 0x%x on %s socket.",
						revents, ppp_socket_names[src]);
				}
				ppp_stop();
				return;
			}
			const ssize_t len = recv(fds[src].fd, ppp_data_buf, mtu, MSG_DONTWAIT);

			if (len <= 0) {
				if (len != -1 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
					LOG_ERR("Failed to receive data from %s socket (%d, %d).",
						ppp_socket_names[src], len, errno);
				}
				continue;
			}
			ssize_t send_ret;
			const size_t dst = (src == ZEPHYR_FD_IDX) ? MODEM_FD_IDX : ZEPHYR_FD_IDX;
			void *dst_addr = (dst == MODEM_FD_IDX) ? NULL : &ppp_zephyr_dst_addr;
			socklen_t addrlen = (dst == MODEM_FD_IDX) ? 0 : sizeof(ppp_zephyr_dst_addr);

			send_ret = sendto(fds[dst].fd, ppp_data_buf, len, 0, dst_addr, addrlen);
			if (send_ret == -1) {
				LOG_ERR("Failed to send %zd bytes to %s socket (%d).",
					len, ppp_socket_names[dst], errno);
			} else if (send_ret != len) {
				LOG_ERR("Only sent %zd out of %zd bytes to %s socket.",
					send_ret, len, ppp_socket_names[dst]);
			} else {
				LOG_DBG("Forwarded %zd bytes to %s socket.",
					send_ret, ppp_socket_names[dst]);
			}
		}
	}
}

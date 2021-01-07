/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <net/socket.h>
#include <modem/modem_info.h>
#include <sys/ring_buffer.h>
#include "slm_util.h"
#include "slm_native_tls.h"
#include "slm_at_host.h"
#include "slm_at_tcp_proxy.h"

LOG_MODULE_REGISTER(tcp_proxy, CONFIG_SLM_LOG_LEVEL);

#define THREAD_STACK_SIZE	(KB(3) + NET_IPV4_MTU)
#define THREAD_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO

/* max 2, listening and incoming sockets */
#define MAX_POLL_FD		2

/* Some features need future modem firmware support */
#define SLM_TCP_PROXY_FUTURE_FEATURE	0

/**@brief Proxy operations. */
enum slm_tcp_proxy_operation {
	AT_SERVER_STOP,
	AT_FILTER_CLEAR =  AT_SERVER_STOP,
	AT_CLIENT_DISCONNECT = AT_SERVER_STOP,
	AT_SERVER_START,
	AT_FILTER_SET =  AT_SERVER_START,
	AT_CLIENT_CONNECT = AT_SERVER_START,
	AT_SERVER_START_WITH_DATAMODE,
	AT_CLIENT_CONNECT_WITH_DATAMODE = AT_SERVER_START_WITH_DATAMODE
};

/**@brief Proxy roles. */
enum slm_tcp_proxy_role {
	AT_TCP_ROLE_CLIENT,
	AT_TCP_ROLE_SERVER
};

/**@brief List of supported AT commands. */
enum slm_tcp_proxy_at_cmd_type {
	AT_TCP_FILTER,
	AT_TCP_SERVER,
	AT_TCP_CLIENT,
	AT_TCP_SEND,
	AT_TCP_RECV,
	AT_TCP_PROXY_MAX
};

/** forward declaration of cmd handlers **/
static int handle_at_tcp_filter(enum at_cmd_type cmd_type);
static int handle_at_tcp_server(enum at_cmd_type cmd_type);
static int handle_at_tcp_client(enum at_cmd_type cmd_type);
static int handle_at_tcp_send(enum at_cmd_type cmd_type);
static int handle_at_tcp_recv(enum at_cmd_type cmd_type);

/**@brief SLM AT Command list type. */
static slm_at_cmd_list_t tcp_proxy_at_list[AT_TCP_PROXY_MAX] = {
	{AT_TCP_FILTER, "AT#XTCPFILTER", handle_at_tcp_filter},
	{AT_TCP_SERVER, "AT#XTCPSVR", handle_at_tcp_server},
	{AT_TCP_CLIENT, "AT#XTCPCLI", handle_at_tcp_client},
	{AT_TCP_SEND, "AT#XTCPSEND", handle_at_tcp_send},
	{AT_TCP_RECV, "AT#XTCPRECV", handle_at_tcp_recv},
};

static char ip_allowlist[CONFIG_SLM_TCP_FILTER_SIZE][INET_ADDRSTRLEN];
RING_BUF_DECLARE(data_buf, CONFIG_SLM_SOCKET_RX_MAX * 2);
static struct k_thread tcp_thread;
static struct k_work disconnect_work;
static K_THREAD_STACK_DEFINE(tcp_thread_stack, THREAD_STACK_SIZE);
K_TIMER_DEFINE(conn_timer, NULL, NULL);

static struct sockaddr_in remote;
static struct tcp_proxy_t {
	int sock;		/* Socket descriptor. */
	sec_tag_t sec_tag;	/* Security tag of the credential */
	int sock_peer;		/* Socket descriptor for peer. */
	int role;		/* Client or Server proxy */
	bool datamode;		/* Data mode flag*/
	bool filtermode;	/* Filtering mode flag */
} proxy;
static struct pollfd fds[MAX_POLL_FD];
static int nfds;

/* global functions defined in different files */
void rsp_send(const uint8_t *str, size_t len);
void enter_datamode(void);
bool exit_datamode(void);
bool check_uart_flowcontrol(void);

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_SLM_SOCKET_RX_MAX * 2];
extern uint8_t rx_data[CONFIG_SLM_SOCKET_RX_MAX];

/** forward declaration of thread function **/
static void tcpcli_thread_func(void *p1, void *p2, void *p3);
static void tcpsvr_thread_func(void *p1, void *p2, void *p3);

static int do_tcp_server_start(uint16_t port)
{
	int ret = 0;
	struct sockaddr_in local;
	int addr_len;
	char ipv4_addr[NET_IPV4_ADDR_LEN];
#if SLM_TCP_PROXY_FUTURE_FEATURE
	int addr_reuse = 1;
#endif

#if defined(CONFIG_SLM_NATIVE_TLS)
	if (proxy.sec_tag != INVALID_SEC_TAG) {
		ret = slm_tls_loadcrdl(proxy.sec_tag);
		if (ret < 0) {
			LOG_ERR("Fail to load credential: %d", ret);
			proxy.sec_tag = INVALID_SEC_TAG;
			goto exit;
		}
	}
#else
	if (proxy.sec_tag != INVALID_SEC_TAG) {
		LOG_ERR("Not supported");
		ret = -EINVAL;
		goto exit;
	}
#endif
	/* Open socket */
	if (proxy.sec_tag == INVALID_SEC_TAG) {
		proxy.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	} else {
		proxy.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);
	}
	if (proxy.sock < 0) {
		LOG_ERR("socket() failed: %d", -errno);
		ret = -errno;
		goto exit;
	}

	if (proxy.sec_tag != INVALID_SEC_TAG) {
		sec_tag_t sec_tag_list[1] = { proxy.sec_tag };

		ret = setsockopt(proxy.sock, SOL_TLS, TLS_SEC_TAG_LIST,
				sec_tag_list, sizeof(sec_tag_t));
		if (ret) {
			LOG_ERR("set tag list failed: %d", -errno);
			ret = -errno;
			goto exit;
		}
	}

#if SLM_TCP_PROXY_FUTURE_FEATURE
	/* Allow reuse of local addresses */
	ret = setsockopt(proxy.sock, SOL_SOCKET, SO_REUSEADDR,
			 &addr_reuse, sizeof(addr_reuse));
	if (ret != 0) {
		LOG_ERR("set reuse addr failed: %d", -errno);
		ret = -errno;
		goto exit;
	}
#endif
	/* Bind to local port */
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	if (!util_get_ipv4_addr(ipv4_addr)) {
		LOG_ERR("Unable to obtain local IPv4 address");
		ret = -ENETUNREACH;
		goto exit;
	}
	addr_len = strlen(ipv4_addr);
	if (addr_len == 0) {
		LOG_ERR("LTE not connected yet");
		ret = -ENETUNREACH;
		goto exit;
	}
	if (!check_for_ipv4(ipv4_addr, addr_len)) {
		LOG_ERR("Invalid local address");
		ret = -EINVAL;
		goto exit;
	}
	if (inet_pton(AF_INET, ipv4_addr, &local.sin_addr) != 1) {
		LOG_ERR("Parse local IP address failed: %d", -errno);
		ret = -EINVAL;
		goto exit;
	}

	ret = bind(proxy.sock, (struct sockaddr *)&local,
		 sizeof(struct sockaddr_in));
	if (ret) {
		LOG_ERR("bind() failed: %d", -errno);
		ret = -errno;
		goto exit;
	}

	/* Enable listen */
	ret = listen(proxy.sock, 1);
	if (ret < 0) {
		LOG_ERR("listen() failed: %d", -errno);
		ret = -EINVAL;
		goto exit;
	}

	k_thread_create(&tcp_thread, tcp_thread_stack,
			K_THREAD_STACK_SIZEOF(tcp_thread_stack),
			tcpsvr_thread_func, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);
	proxy.role = AT_TCP_ROLE_SERVER;
	sprintf(rsp_buf, "#XTCPSVR: %d,\"started\"\r\n", proxy.sock);
	rsp_send(rsp_buf, strlen(rsp_buf));

exit:
	if (ret < 0) {
		if (proxy.sock != INVALID_SOCKET) {
			close(proxy.sock);
		}
#if defined(CONFIG_SLM_NATIVE_TLS)
		if (proxy.sec_tag != INVALID_SEC_TAG) {
			if (slm_tls_unloadcrdl(proxy.sec_tag) != 0) {
				LOG_ERR("Fail to unload credential");
			}
		}
#endif
		slm_at_tcp_proxy_init();
		sprintf(rsp_buf, "#XTCPSVR: %d\r\n", ret);
		rsp_send(rsp_buf, strlen(rsp_buf));
	}

	return ret;
}

static int do_tcp_server_stop(void)
{
	if (proxy.sock_peer != INVALID_SOCKET) {
		close(proxy.sock_peer);
	}

	if (proxy.sock == INVALID_SOCKET) {
		LOG_WRN("Proxy server is not running");
		return -EINVAL;
	}
	close(proxy.sock);

	return 0;
}

static int do_tcp_client_connect(const char *url, uint16_t port)
{
	int ret;

	/* Open socket */
	if (proxy.sec_tag == INVALID_SEC_TAG) {
		proxy.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	} else {
		proxy.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);

	}
	if (proxy.sock < 0) {
		LOG_ERR("socket() failed: %d", -errno);
		ret = -errno;
		goto exit;
	}
	if (proxy.sec_tag != INVALID_SEC_TAG) {
		sec_tag_t sec_tag_list[1] = { proxy.sec_tag };

		ret = setsockopt(proxy.sock, SOL_TLS, TLS_SEC_TAG_LIST,
				sec_tag_list, sizeof(sec_tag_t));
		if (ret) {
			LOG_ERR("set tag list failed: %d", -errno);
			ret = -errno;
			goto exit;
		}
	}

	/* Connect to remote host */
	if (check_for_ipv4(url, strlen(url))) {
		remote.sin_family = AF_INET;
		remote.sin_port = htons(port);
		LOG_DBG("IPv4 Address %s", log_strdup(url));
		/* NOTE inet_pton() returns 1 as success */
		ret = inet_pton(AF_INET, url, &remote.sin_addr);
		if (ret != 1) {
			LOG_ERR("inet_pton() failed: %d", ret);
			ret = -EINVAL;
			goto exit;
		}
	} else {
		struct addrinfo *result;
		struct addrinfo hints = {
			.ai_family = AF_INET,
			.ai_socktype = SOCK_STREAM
		};

		ret = getaddrinfo(url, NULL, &hints, &result);
		if (ret || result == NULL) {
			LOG_ERR("getaddrinfo() failed: %d", ret);
			ret = -EINVAL;
			goto exit;
		}

		remote.sin_family = AF_INET;
		remote.sin_port = htons(port);
		remote.sin_addr.s_addr =
		((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
		/* Free the address. */
		freeaddrinfo(result);
	}

	ret = connect(proxy.sock, (struct sockaddr *)&remote,
		sizeof(struct sockaddr_in));
	if (ret < 0) {
		LOG_ERR("connect() failed: %d", -errno);
		ret = -errno;
		goto exit;
	}

	k_thread_create(&tcp_thread, tcp_thread_stack,
			K_THREAD_STACK_SIZEOF(tcp_thread_stack),
			tcpcli_thread_func, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);

	proxy.role = AT_TCP_ROLE_CLIENT;
	sprintf(rsp_buf, "#XTCPCLI: %d,\"connected\"\r\n", proxy.sock);
	rsp_send(rsp_buf, strlen(rsp_buf));

exit:
	if (ret < 0) {
		if (proxy.sock != INVALID_SOCKET) {
			close(proxy.sock);
		}
		slm_at_tcp_proxy_init();
		sprintf(rsp_buf, "#XTCPCLI: %d\r\n", ret);
		rsp_send(rsp_buf, strlen(rsp_buf));
	}

	return ret;
}

static int do_tcp_client_disconnect(void)
{
	if (proxy.sock == INVALID_SOCKET) {
		LOG_WRN("Client is not running");
		return -EINVAL;
	}
	close(proxy.sock);

	return 0;
}

static int do_tcp_send(const uint8_t *data, int datalen)
{
	int ret = 0;
	uint32_t offset = 0;
	int sock;

	if (proxy.role == AT_TCP_ROLE_CLIENT &&
	    proxy.sock != INVALID_SOCKET) {
		sock = proxy.sock;
	} else if (proxy.role == AT_TCP_ROLE_SERVER &&
		   proxy.sock_peer != INVALID_SOCKET) {
		sock = proxy.sock_peer;
		k_timer_stop(&conn_timer);
	} else {
		LOG_ERR("Not connected yet");
		return -EINVAL;
	}

	while (offset < datalen) {
		ret = send(sock, data + offset, datalen - offset, 0);
		if (ret < 0) {
			LOG_ERR("send() failed: %d", -errno);
			sprintf(rsp_buf, "#XTCPSEND: %d\r\n", -errno);
			rsp_send(rsp_buf, strlen(rsp_buf));
			if (errno != EAGAIN && errno != ETIMEDOUT) {
				if (proxy.role == AT_TCP_ROLE_CLIENT) {
					do_tcp_client_disconnect();
				} else {
					do_tcp_server_stop();
				}
			}
			ret = -errno;
			break;
		}
		offset += ret;
	}

	if (ret >= 0) {
		sprintf(rsp_buf, "#XTCPSEND: %d\r\n", offset);
		rsp_send(rsp_buf, strlen(rsp_buf));
		/* restart activity timer */
		if (proxy.role == AT_TCP_ROLE_SERVER) {
			k_timer_start(&conn_timer,
				      K_SECONDS(CONFIG_SLM_TCP_CONN_TIME),
				      K_NO_WAIT);
		}
		return 0;
	} else {
		return ret;
	}
}

static int do_tcp_send_datamode(const uint8_t *data, int datalen)
{
	int ret = 0;
	uint32_t offset = 0;
	int sock;

	if (proxy.role == AT_TCP_ROLE_CLIENT &&
	    proxy.sock != INVALID_SOCKET) {
		sock = proxy.sock;
	} else if (proxy.role == AT_TCP_ROLE_SERVER &&
		   proxy.sock_peer != INVALID_SOCKET) {
		sock = proxy.sock_peer;
		k_timer_stop(&conn_timer);
	} else {
		LOG_ERR("Not connected yet");
		return -EINVAL;
	}

	while (offset < datalen) {
		ret = send(sock, data + offset, datalen - offset, 0);
		if (ret < 0) {
			LOG_ERR("send() failed: %d", -errno);
			ret = -errno;
			break;
		}
		offset += ret;
	}

	/* restart activity timer */
	if (proxy.role == AT_TCP_ROLE_SERVER) {
		k_timer_start(&conn_timer, K_SECONDS(CONFIG_SLM_TCP_CONN_TIME),
			K_NO_WAIT);
	}

	return offset;
}

static int tcp_data_save(uint8_t *data, uint32_t length)
{
	if (ring_buf_space_get(&data_buf) < length) {
		return -1; /* RX overrun */
	}

	return ring_buf_put(&data_buf, data, length);
}

static void tcp_data_handle(uint8_t *data, uint32_t length)
{
	int ret;

	if (proxy.datamode) {
		rsp_send(data, length);
	} else if (slm_util_hex_check(data, length)) {
		uint8_t data_hex[length * 2];

		ret = slm_util_htoa(data, length, data_hex, length * 2);
		if (ret < 0) {
			LOG_ERR("hex convert error: %d", ret);
			return;
		}
		if (tcp_data_save(data_hex, ret) < 0) {
			sprintf(rsp_buf, "#XTCPDATA: \"overrun\"\r\n");
		} else {
			sprintf(rsp_buf, "#XTCPDATA: %d,%d\r\n",
				DATATYPE_HEXADECIMAL, ret);
		}
		rsp_send(rsp_buf, strlen(rsp_buf));
	} else {
		if (tcp_data_save(data, length) < 0) {
			sprintf(rsp_buf, "#XTCPDATA: \"overrun\"\r\n");
		} else {
			sprintf(rsp_buf, "#XTCPDATA: %d,%d\r\n",
			DATATYPE_PLAINTEXT, length);
		}
		rsp_send(rsp_buf, strlen(rsp_buf));
	}

}

static void tcp_terminate_connection(int cause)
{
	k_timer_stop(&conn_timer);

	if (proxy.datamode) {
		(void)exit_datamode();
	}
	close(proxy.sock_peer);
	proxy.sock_peer = INVALID_SOCKET;
	nfds--;
	/* Send URC for server-initiated disconnect */
	sprintf(rsp_buf,
		"#XTCPSVR: %d,\"disconnected\"\r\n", cause);
	rsp_send(rsp_buf, strlen(rsp_buf));
}

static void terminate_connection_wk(struct k_work *work)
{
	ARG_UNUSED(work);

	tcp_terminate_connection(-ENETDOWN);
}

static int tcpsvr_input(int infd)
{
	int ret;

	if (fds[infd].fd == proxy.sock) {
		socklen_t len = sizeof(struct sockaddr_in);
		char peer_addr[INET_ADDRSTRLEN];
		bool filtered = true;

		/* Accept incoming connection */
		ret = accept(proxy.sock,
				(struct sockaddr *)&remote, &len);
		if (ret < 0) {
			LOG_ERR("accept() failed: %d", -errno);
			return -errno;
		}
		if (nfds >= MAX_POLL_FD) {
			LOG_WRN("Full. Close connection.");
			close(ret);
			return -ECONNREFUSED;
		}
		LOG_DBG("accept(): %d", ret);

		/* Client IPv4 filtering */
		if (inet_ntop(AF_INET, &remote.sin_addr, peer_addr,
			INET_ADDRSTRLEN) == NULL) {
			LOG_ERR("inet_ntop() failed: %d", -errno);
			close(ret);
			return -errno;
		}
		if (proxy.filtermode) {
			for (int i = 0; i < CONFIG_SLM_TCP_FILTER_SIZE; i++) {
				if (strlen(ip_allowlist[i]) > 0 &&
				    strcmp(ip_allowlist[i], peer_addr) == 0) {
					filtered = false;
					break;
				}
			}
			if (filtered) {
				LOG_WRN("Connection filtered");
				close(ret);
				return -ECONNREFUSED;
			}
		}
		sprintf(rsp_buf, "#XTCPSVR: \"%s\",\"connected\"\r\n",
			peer_addr);
		rsp_send(rsp_buf, strlen(rsp_buf));
		proxy.sock_peer = ret;
		if (proxy.datamode) {
			enter_datamode();
		}
		LOG_DBG("New connection - %d", proxy.sock_peer);
		fds[nfds].fd = proxy.sock_peer;
		fds[nfds].events = POLLIN;
		nfds++;
		/* Start a one-shot timer to close the connection */
		k_timer_start(&conn_timer,
			      K_SECONDS(CONFIG_SLM_TCP_CONN_TIME),
			      K_NO_WAIT);
	} else {
		k_timer_stop(&conn_timer);
		ret = recv(fds[infd].fd, rx_data, sizeof(rx_data), 0);
		if (ret > 0) {
			tcp_data_handle(rx_data, ret);
		}
		if (ret < 0) {
			LOG_WRN("recv() error: %d", -errno);
		}
		/* Restart conn timer */
		LOG_DBG("restart timer: POLLIN");
		k_timer_start(&conn_timer,
			      K_SECONDS(CONFIG_SLM_TCP_CONN_TIME),
			      K_NO_WAIT);
	}

	return ret;
}

/* TCP server thread */
static void tcpsvr_thread_func(void *p1, void *p2, void *p3)
{
	int ret, current_size;
	bool in_datamode;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	fds[0].fd = proxy.sock;
	fds[0].events = POLLIN;
	nfds = 1;
	ring_buf_reset(&data_buf);
	while (true) {
		if (k_timer_status_get(&conn_timer) > 0) {
			LOG_WRN("Connecion timeout");
			tcp_terminate_connection(-ETIMEDOUT);
		}
		ret = poll(fds, nfds, MSEC_PER_SEC * CONFIG_SLM_TCP_POLL_TIME);
		if (ret < 0) {  /* IO error */
			LOG_WRN("poll() error: %d", -errno);
			ret = -EIO;
			goto exit;
		}
		if (ret == 0) {  /* timeout */
			continue;
		}
		current_size = nfds;
		for (int i = 0; i < current_size; i++) {
			LOG_DBG("Poll events 0x%08x", fds[i].revents);
			if ((fds[i].revents & POLLERR) == POLLERR) {
				LOG_ERR("POLLERR: %d", i);
				if (fds[i].fd == proxy.sock) {
					ret = -EIO;
					goto exit;
				}
				tcp_terminate_connection(-EIO);
				continue;
			}
			if ((fds[i].revents & POLLHUP) == POLLHUP) {
				LOG_WRN("POLLHUP: %d", i);
				if (fds[i].fd == proxy.sock) {
					ret = -ENETDOWN;
					goto exit;
				}
				tcp_terminate_connection(-ECONNRESET);
				continue;
			}
			if ((fds[i].revents & POLLNVAL) == POLLNVAL) {
				LOG_WRN("POLLNVAL: %d", i);
				if (fds[i].fd == proxy.sock) {
					ret = 0;
					goto exit;
				}
				tcp_terminate_connection(-ECONNABORTED);
				continue;
			}
			if ((fds[i].revents & POLLIN) == POLLIN) {
				ret = tcpsvr_input(i);
				if (ret < 0) {
					LOG_WRN("tcpsvr_input error: %d", ret);
				}
			}
		}
	}
exit:
	if (proxy.sock_peer != INVALID_SOCKET) {
		k_timer_stop(&conn_timer);
		ret = close(proxy.sock_peer);
		if (ret < 0) {
			LOG_WRN("close(%d) fail: %d", proxy.sock_peer, -errno);
		}
	}
	if (proxy.sock != INVALID_SOCKET) {
		ret = close(proxy.sock);
		if (ret < 0) {
			LOG_WRN("close(%d) fail: %d", proxy.sock, -errno);
		}
	}
#if defined(CONFIG_SLM_NATIVE_TLS)
	if (proxy.sec_tag != INVALID_SEC_TAG) {
		ret = slm_tls_unloadcrdl(proxy.sec_tag);
		if (ret < 0) {
			LOG_ERR("Fail to unload credential: %d", ret);
		}
	}
#endif
	in_datamode = proxy.datamode;
	slm_at_tcp_proxy_init();
	sprintf(rsp_buf, "#XTCPSVR: %d,\"stopped\"\r\n", ret);
	rsp_send(rsp_buf, strlen(rsp_buf));
	if (in_datamode) {
		if (exit_datamode()) {
			sprintf(rsp_buf, "#XTCPSVR: 0,\"datamode\"\r\n");
			rsp_send(rsp_buf, strlen(rsp_buf));
		}
	}
}

/* TCP client thread */
static void tcpcli_thread_func(void *p1, void *p2, void *p3)
{
	int ret;
	bool in_datamode;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	fds[0].fd = proxy.sock;
	fds[0].events = POLLIN;
	ring_buf_reset(&data_buf);
	while (true) {
		ret = poll(&fds[0], 1, MSEC_PER_SEC * CONFIG_SLM_TCP_POLL_TIME);
		if (ret < 0) {  /* IO error */
			LOG_WRN("poll() error: %d", ret);
			ret = -EIO;
			goto exit;
		}
		if (ret == 0) {  /* timeout */
			continue;
		}
		LOG_DBG("Poll events 0x%08x", fds[0].revents);
		if ((fds[0].revents & POLLERR) == POLLERR) {
			LOG_ERR("POLLERR");
			ret = -EIO;
			goto exit;
		}
		if ((fds[0].revents & POLLNVAL) == POLLNVAL) {
			LOG_INF("TCP client disconnected.");
			proxy.sock = INVALID_SOCKET;
			goto exit;
		}
		if ((fds[0].revents & POLLHUP) == POLLHUP) {
			LOG_INF("Peer disconnect: %d", fds[0].fd);
			goto exit;
		}
		if ((fds[0].revents & POLLIN) == POLLIN) {
			ret = recv(fds[0].fd, rx_data, sizeof(rx_data), 0);
			if (ret < 0) {
				LOG_WRN("recv() error: %d", -errno);
				continue;
			}
			if (ret == 0) {
				continue;
			}
			tcp_data_handle(rx_data, ret);
		}
	}
exit:
	if (proxy.sock != INVALID_SOCKET) {
		ret = close(proxy.sock);
		if (ret < 0) {
			LOG_WRN("close(%d) fail: %d", proxy.sock, -errno);
		}
	}
	in_datamode = proxy.datamode;
	slm_at_tcp_proxy_init();
	sprintf(rsp_buf, "#XTCPCLI: %d,\"disconnected\"\r\n", ret);
	rsp_send(rsp_buf, strlen(rsp_buf));
	if (in_datamode) {
		if (exit_datamode()) {
			sprintf(rsp_buf, "#XTCPCLI: 0,\"datamode\"\r\n");
			rsp_send(rsp_buf, strlen(rsp_buf));
		}
	}
}

/**@brief handle AT#XTCPFILTER commands
 *  AT#XTCPFILTER=<op>[,<IP_ADDR#1>[,<IP_ADDR#2>[,...]]]
 *  AT#XTCPFILTER?
 *  AT#XTCPFILTER=?
 */
static int handle_at_tcp_filter(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	int param_count = at_params_valid_count_get(&at_param_list);

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_short_get(&at_param_list, 1, &op);
		if (err) {
			return err;
		}
		if (op == AT_FILTER_SET) {
			char address[INET_ADDRSTRLEN];
			int size;

			if (param_count > (CONFIG_SLM_TCP_FILTER_SIZE + 2)) {
				return -EINVAL;
			}
			memset(ip_allowlist, 0x00, sizeof(ip_allowlist));
			for (int i = 2; i < param_count; i++) {
				size = INET_ADDRSTRLEN;
				err = util_string_get(&at_param_list, i,
							   address, &size);
				if (err) {
					return err;
				}
				if (!check_for_ipv4(address, size)) {
					return -EINVAL;
				}
				memcpy(ip_allowlist[i - 2], address, size);
			}
			proxy.filtermode = true;
			err = 0;
		} else if (op == AT_FILTER_CLEAR) {
			memset(ip_allowlist, 0x00, sizeof(ip_allowlist));
			proxy.filtermode = false;
			err = 0;
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		sprintf(rsp_buf, "#XTCPFILTER: %d", proxy.filtermode);
		for (int i = 0; i < CONFIG_SLM_TCP_FILTER_SIZE; i++) {
			if (strlen(ip_allowlist[i]) > 0) {
				strcat(rsp_buf, ",\"");
				strcat(rsp_buf, ip_allowlist[i]);
				strcat(rsp_buf, "\"");
			}
		}
		strcat(rsp_buf, "\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "#XTCPFILTER: (%d,%d)",
			AT_FILTER_CLEAR, AT_FILTER_SET);
		strcat(rsp_buf, ",<IP_ADDR#1>[,<IP_ADDR#2>[,...]]\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XTCPSVR commands
 *  AT#XTCPSVR=<op>[,<port>[,[sec_tag]]
 *  AT#XTCPSVR?
 *  AT#XTCPSVR=?
 */
static int handle_at_tcp_server(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	int param_count = at_params_valid_count_get(&at_param_list);

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_short_get(&at_param_list, 1, &op);
		if (err) {
			return err;
		}
		if (op == AT_SERVER_START ||
		    op == AT_SERVER_START_WITH_DATAMODE) {
			uint16_t port;

			if (proxy.sock != INVALID_SOCKET) {
				LOG_ERR("Server is already running.");
				return -EINVAL;
			}
			err = at_params_short_get(&at_param_list, 2, &port);
			if (err) {
				return err;
			}
			if (param_count > 3) {
				at_params_int_get(&at_param_list, 3,
						  &proxy.sec_tag);
			}
#if defined(CONFIG_SLM_DATAMODE_HWFC)
			if (op == AT_SERVER_START_WITH_DATAMODE &&
			    !check_uart_flowcontrol()) {
				return -EINVAL;
			}
#endif
			err = do_tcp_server_start(port);
			if (err == 0 && op == AT_SERVER_START_WITH_DATAMODE) {
				proxy.datamode = true;
			}
		} else if (op == AT_SERVER_STOP) {
			err = do_tcp_server_stop();
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		if (proxy.sock != INVALID_SOCKET &&
		    proxy.role == AT_TCP_ROLE_SERVER) {
			sprintf(rsp_buf, "#XTCPSVR: %d,%d,%d\r\n",
				proxy.sock, proxy.sock_peer, proxy.datamode);
		} else {
			sprintf(rsp_buf, "#XTCPSVR: %d,%d\r\n",
				INVALID_SOCKET, INVALID_SOCKET);
		}
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "#XTCPSVR: (%d,%d,%d),<port>,<sec_tag>\r\n",
			AT_SERVER_STOP, AT_SERVER_START,
			AT_SERVER_START_WITH_DATAMODE);
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XTCPCLI commands
 *  AT#XTCPCLI=<op>[,<url>,<port>[,[sec_tag]]
 *  AT#XTCPCLI?
 *  AT#XTCPCLI=?
 */
static int handle_at_tcp_client(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	int param_count = at_params_valid_count_get(&at_param_list);

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_short_get(&at_param_list, 1, &op);
		if (err) {
			return err;
		}
		if (op == AT_CLIENT_CONNECT ||
		    op == AT_CLIENT_CONNECT_WITH_DATAMODE) {
			uint16_t port;
			char url[TCPIP_MAX_URL];
			int size = TCPIP_MAX_URL;

			if (proxy.sock != INVALID_SOCKET) {
				LOG_ERR("Client is already running.");
				return -EINVAL;
			}
			err = util_string_get(&at_param_list,
						2, url, &size);
			if (err) {
				return err;
			}
			err = at_params_short_get(&at_param_list, 3, &port);
			if (err) {
				return err;
			}
			if (param_count > 4) {
				at_params_int_get(&at_param_list,
						  4, &proxy.sec_tag);
			}
#if defined(CONFIG_SLM_DATAMODE_HWFC)
			if (op == AT_CLIENT_CONNECT_WITH_DATAMODE &&
			    !check_uart_flowcontrol()) {
				return -EINVAL;
			}
#endif
			err = do_tcp_client_connect(url, port);
			if (err == 0 &&
			    op == AT_CLIENT_CONNECT_WITH_DATAMODE) {
				proxy.datamode = true;
				enter_datamode();
			}
		} else if (op == AT_CLIENT_DISCONNECT) {
			err = do_tcp_client_disconnect();
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		sprintf(rsp_buf, "#XTCPCLI: %d,%d\r\n",
			proxy.sock, proxy.datamode);
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf,
			"#XTCPCLI: (%d,%d,%d),<url>,<port>,<sec_tag>\r\n",
			AT_CLIENT_DISCONNECT, AT_CLIENT_CONNECT,
			AT_CLIENT_CONNECT_WITH_DATAMODE);
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XTCPSEND commands
 *  AT#XTCPSEND=<datatype>,<data>
 *  AT#XTCPSEND? READ command not supported
 *  AT#XTCPSEND=? TEST command not supported
 */
static int handle_at_tcp_send(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t datatype;
	char data[NET_IPV4_MTU];
	int size = NET_IPV4_MTU;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_short_get(&at_param_list, 1, &datatype);
		if (err) {
			return err;
		}
		err = util_string_get(&at_param_list, 2, data, &size);
		if (err) {
			return err;
		}
		if (datatype == DATATYPE_HEXADECIMAL) {
			uint8_t data_hex[size / 2];

			err = slm_util_atoh(data, size, data_hex, size / 2);
			if (err > 0) {
				err = do_tcp_send(data_hex, err);
			}
		} else {
			err = do_tcp_send(data, size);
		}
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XTCPRECV commands
 *  AT#XTCPRECV[=<length>]
 *  AT#XTCPRECV? READ command not supported
 *  AT#XTCPRECV=? TEST command not supported
 */
static int handle_at_tcp_recv(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t length = 0;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
	{
		uint32_t sz_send = 0;

		if (at_params_valid_count_get(&at_param_list) > 1) {
			err = at_params_short_get(&at_param_list, 1, &length);
			if (err) {
				return err;
			}
		}
		if (ring_buf_is_empty(&data_buf) == 0) {
			sz_send = ring_buf_get(&data_buf, rsp_buf,
					sizeof(rsp_buf));
			if (length > 0 && sz_send > length) {
				sz_send = length;
			}
			rsp_send(rsp_buf, sz_send);
			rsp_send("\r\n", 2);
		}
		sprintf(rsp_buf, "#XTCPRECV: %d\r\n", sz_send);
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
	} break;

	default:
		break;
	}

	return err;
}

/**@brief API to handle TCP proxy AT commands
 */
int slm_at_tcp_proxy_parse(const char *at_cmd)
{
	int ret = -ENOENT;
	enum at_cmd_type type;

	for (int i = 0; i < AT_TCP_PROXY_MAX; i++) {
		if (slm_util_cmd_casecmp(at_cmd,
			tcp_proxy_at_list[i].string)) {
			ret = at_parser_params_from_str(at_cmd, NULL,
						&at_param_list);
			if (ret) {
				LOG_ERR("Failed to parse AT command %d", ret);
				return -EINVAL;
			}
			type = at_parser_cmd_type_get(at_cmd);
			ret = tcp_proxy_at_list[i].handler(type);
			break;
		}
	}

	return ret;
}

/**@brief API to list TCP proxy AT commands
 */
void slm_at_tcp_proxy_clac(void)
{
	for (int i = 0; i < AT_TCP_PROXY_MAX; i++) {
		sprintf(rsp_buf, "%s\r\n", tcp_proxy_at_list[i].string);
		rsp_send(rsp_buf, strlen(rsp_buf));
	}
}

/**@brief API to initialize TCP proxy AT commands handler
 */
int slm_at_tcp_proxy_init(void)
{
	proxy.sock = INVALID_SOCKET;
	proxy.sock_peer = INVALID_SOCKET;
	proxy.role = INVALID_ROLE;
	proxy.datamode = false;
	proxy.sec_tag = INVALID_SEC_TAG;
	nfds = 0;
	for (int i = 0; i < MAX_POLL_FD; i++) {
		fds[i].fd = INVALID_SOCKET;
	}
	memset(ip_allowlist, 0x00, sizeof(ip_allowlist));
	proxy.filtermode = false;
	k_work_init(&disconnect_work, terminate_connection_wk);

	return 0;
}

/**@brief API to uninitialize TCP proxy AT commands handler
 */
int slm_at_tcp_proxy_uninit(void)
{
	if (proxy.role == AT_TCP_ROLE_CLIENT) {
		return do_tcp_client_disconnect();
	}
	if (proxy.role == AT_TCP_ROLE_SERVER) {
		return do_tcp_server_stop();
	}

	return 0;
}

/**@brief API to get datamode from external
 */
bool slm_tcp_get_datamode(void)
{
	return proxy.datamode;
}

/**@brief API to set datamode off from external
 */
void slm_tcp_set_datamode_off(void)
{
	if (proxy.role == AT_TCP_ROLE_CLIENT &&
	    proxy.sock != INVALID_SOCKET) {
		proxy.datamode = false;
	}
	if (proxy.role == AT_TCP_ROLE_SERVER &&
	    proxy.sock_peer != INVALID_SOCKET) {
		k_work_submit(&disconnect_work);
	}
}

/**@brief API to send TCP data in datamode
 */
int slm_tcp_send_datamode(const uint8_t *data, int len)
{
	int size = do_tcp_send_datamode(data, len);

	LOG_DBG("datamode %d sent", size);
	return size;
}

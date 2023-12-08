/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/net/socket.h>
#include "slm_util.h"
#include "slm_native_tls.h"
#include "slm_at_host.h"
#include "slm_at_tcp_proxy.h"

LOG_MODULE_REGISTER(slm_tcp, CONFIG_SLM_LOG_LEVEL);

#define THREAD_STACK_SIZE	KB(4)
#define THREAD_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO

/* Some features need future modem firmware support */
#define SLM_TCP_PROXY_FUTURE_FEATURE	0

/**@brief Proxy operations. */
enum slm_tcp_proxy_operation {
	SERVER_STOP,
	CLIENT_DISCONNECT = SERVER_STOP,
	SERVER_START,
	CLIENT_CONNECT = SERVER_START,
	SERVER_START6,
	CLIENT_CONNECT6 = SERVER_START6
};

/**@brief Proxy roles. */
enum slm_tcp_role {
	TCP_ROLE_CLIENT,
	TCP_ROLE_SERVER
};

static struct k_thread tcp_thread;
static K_THREAD_STACK_DEFINE(tcp_thread_stack, THREAD_STACK_SIZE);
static bool server_stop_pending;

static struct tcp_proxy {
	int sock;		/* Socket descriptor. */
	int family;		/* Socket address family */
	sec_tag_t sec_tag;	/* Security tag of the credential */
	int sock_peer;		/* Socket descriptor for peer. */
	enum slm_tcp_role role;	/* Client or Server proxy */
} proxy;

/** forward declaration of thread function **/
static void tcpcli_thread_func(void *p1, void *p2, void *p3);
static void tcpsvr_thread_func(void *p1, void *p2, void *p3);

static int do_tcp_server_start(uint16_t port)
{
	int ret;
	int reuseaddr = 1;

#if defined(CONFIG_SLM_NATIVE_TLS)
	if (proxy.sec_tag != INVALID_SEC_TAG) {
		ret = slm_tls_loadcrdl(proxy.sec_tag);
		if (ret < 0) {
			LOG_ERR("Fail to load credential: %d", ret);
			return -EAGAIN;
		}
	}
#else
#if !SLM_TCP_PROXY_FUTURE_FEATURE
/* TLS server not officially supported by modem yet */
	if (proxy.sec_tag != INVALID_SEC_TAG) {
		LOG_ERR("Not supported");
		return -ENOTSUP;
	}
#endif
#endif
	/* Open socket */
	if (proxy.sec_tag == INVALID_SEC_TAG) {
		ret = socket(proxy.family, SOCK_STREAM, IPPROTO_TCP);
	} else {
#if defined(CONFIG_SLM_NATIVE_TLS)
		ret = socket(proxy.family, SOCK_STREAM | SOCK_NATIVE_TLS, IPPROTO_TLS_1_2);
#else
		ret = socket(proxy.family, SOCK_STREAM, IPPROTO_TLS_1_2);
#endif
	}
	if (ret < 0) {
		LOG_ERR("socket() failed: %d", -errno);
		ret = -errno;
		goto exit_svr;
	}
	proxy.sock = ret;

	/* Config socket options */
	if (proxy.sec_tag != INVALID_SEC_TAG) {
		sec_tag_t sec_tag_list[1] = { proxy.sec_tag };

		ret = setsockopt(proxy.sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
				 sizeof(sec_tag_t));
		if (ret) {
			LOG_ERR("setsockopt(TLS_SEC_TAG_LIST) error: %d", -errno);
			ret = -errno;
			goto exit_svr;
		}
#if SLM_TCP_PROXY_FUTURE_FEATURE
/* TLS server not officially supported by modem yet */
		int tls_role = TLS_DTLS_ROLE_SERVER;
		int peer_verify = TLS_PEER_VERIFY_NONE;

		ret = setsockopt(proxy.sock, SOL_TLS, TLS_DTLS_ROLE, &tls_role, sizeof(int));
		if (ret) {
			LOG_ERR("setsockopt(TLS_DTLS_ROLE) error: %d", -errno);
			ret = -errno;
			goto exit_svr;
		}
		ret = setsockopt(proxy.sock, SOL_TLS, TLS_PEER_VERIFY, &peer_verify,
				 sizeof(peer_verify));
		if (ret) {
			LOG_ERR("setsockopt(TLS_PEER_VERIFY) error: %d", errno);
			ret = -errno;
			goto exit_svr;
		}
#endif
	}

	ret = setsockopt(proxy.sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int));
	if (ret < 0) {
		LOG_ERR("setsockopt(SO_REUSEADDR): %d", -errno);
		ret = -errno;
		goto exit_svr;
	}

	/* Bind to local port */
	if (proxy.family == AF_INET) {
		char ipv4_addr[INET_ADDRSTRLEN];

		util_get_ip_addr(0, ipv4_addr, NULL);
		if (!*ipv4_addr) {
			LOG_ERR("Unable to obtain local IPv4 address");
			ret = -ENETUNREACH;
			goto exit_svr;
		}

		struct sockaddr_in local = {
			.sin_family = AF_INET,
			.sin_port = htons(port)
		};

		if (inet_pton(AF_INET, ipv4_addr, &local.sin_addr) != 1) {
			LOG_ERR("Parse local IPv4 address failed: %d", -errno);
			ret = -EINVAL;
			goto exit_svr;
		}
		ret = bind(proxy.sock, (struct sockaddr *)&local, sizeof(struct sockaddr_in));
	} else {
		char ipv6_addr[INET6_ADDRSTRLEN];

		util_get_ip_addr(0, NULL, ipv6_addr);
		if (!*ipv6_addr) {
			LOG_ERR("Unable to obtain local IPv6 address");
			ret = -ENETUNREACH;
			goto exit_svr;
		}

		struct sockaddr_in6 local = {
			.sin6_family = AF_INET6,
			.sin6_port = htons(port)
		};

		if (inet_pton(AF_INET6, ipv6_addr, &local.sin6_addr) != 1) {
			LOG_ERR("Parse local IPv6 address failed: %d", -errno);
			ret = -EINVAL;
			goto exit_svr;
		}
		ret = bind(proxy.sock, (struct sockaddr *)&local, sizeof(struct sockaddr_in6));
	}
	if (ret) {
		LOG_ERR("bind() failed: %d", -errno);
		ret = -errno;
		goto exit_svr;
	}

	/* Enable listen */
	ret = listen(proxy.sock, 1);
	if (ret < 0) {
		LOG_ERR("listen() failed: %d", -errno);
		ret = -EINVAL;
		goto exit_svr;
	}

	k_thread_create(&tcp_thread, tcp_thread_stack,
			K_THREAD_STACK_SIZEOF(tcp_thread_stack),
			tcpsvr_thread_func, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);
	proxy.role = TCP_ROLE_SERVER;
	rsp_send("\r\n#XTCPSVR: %d,\"started\"\r\n", proxy.sock);

	return 0;

exit_svr:
#if defined(CONFIG_SLM_NATIVE_TLS)
	if (proxy.sec_tag != INVALID_SEC_TAG) {
		(void)slm_tls_unloadcrdl(proxy.sec_tag);
		proxy.sec_tag = INVALID_SEC_TAG;
	}
#endif
	if (proxy.sock != INVALID_SOCKET) {
		close(proxy.sock);
		proxy.sock = INVALID_SOCKET;
	}
	rsp_send("\r\n#XTCPSVR: %d,\"not started\"\r\n", ret);

	return ret;
}

static int do_tcp_server_stop(void)
{
	int ret;

	if (proxy.sock == INVALID_SOCKET) {
		return 0;
	}
#if defined(CONFIG_SLM_NATIVE_TLS)
	if (proxy.sec_tag != INVALID_SEC_TAG) {
		(void)slm_tls_unloadcrdl(proxy.sec_tag);
		proxy.sec_tag = INVALID_SEC_TAG;
	}
#endif
	server_stop_pending = true;
	if (proxy.sock_peer != INVALID_SOCKET) {
		(void)close(proxy.sock_peer);
		proxy.sock_peer = INVALID_SOCKET;
		rsp_send("\r\n#XTCPSVR: 0,\"disconnected\"\r\n");
	} else {
		ret = close(proxy.sock);
		if (ret < 0) {
			LOG_WRN("close() failed: %d", -errno);
			return ret;
		}
		proxy.sock = INVALID_SOCKET;
		proxy.family = AF_UNSPEC;
	}
	if (k_thread_join(&tcp_thread, K_SECONDS(CONFIG_SLM_TCP_POLL_TIME + 1)) != 0) {
		LOG_WRN("Wait for thread terminate failed");
	}

	return 0;
}

static int do_tcp_client_connect(const char *url, uint16_t port)
{
	int ret;
	struct sockaddr sa;

	/* Open socket */
	if (proxy.sec_tag == INVALID_SEC_TAG) {
		ret = socket(proxy.family, SOCK_STREAM, IPPROTO_TCP);
	} else {
#if defined(CONFIG_SLM_NATIVE_TLS)
		ret = socket(proxy.family, SOCK_STREAM | SOCK_NATIVE_TLS, IPPROTO_TLS_1_2);
#else
		ret = socket(proxy.family, SOCK_STREAM, IPPROTO_TLS_1_2);
#endif
	}
	if (ret < 0) {
		LOG_ERR("socket() failed: %d", -errno);
		return ret;
	}
	proxy.sock = ret;

	if (proxy.sec_tag != INVALID_SEC_TAG) {
		sec_tag_t sec_tag_list[1] = { proxy.sec_tag };
		int peer_verify = TLS_PEER_VERIFY_REQUIRED;

#if defined(CONFIG_SLM_NATIVE_TLS)
		ret = slm_tls_loadcrdl(proxy.sec_tag);
		if (ret < 0) {
			LOG_ERR("Fail to load credential: %d", ret);
			goto exit_cli;
		}
#endif
		ret = setsockopt(proxy.sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
				 sizeof(sec_tag_t));
		if (ret) {
			LOG_ERR("setsockopt(TLS_SEC_TAG_LIST) error: %d", -errno);
			ret = -errno;
			goto exit_cli;
		}
		ret = setsockopt(proxy.sock, SOL_TLS, TLS_PEER_VERIFY, &peer_verify,
				 sizeof(peer_verify));
		if (ret) {
			LOG_ERR("setsockopt(TLS_PEER_VERIFY) error: %d", errno);
			ret = -errno;
			goto exit_cli;
		}
	}

	/* Connect to remote host */
	ret = util_resolve_host(0, url, port, proxy.family, Z_LOG_OBJECT_PTR(slm_tcp), &sa);
	if (ret) {
		goto exit_cli;
	}
	if (sa.sa_family == AF_INET) {
		ret = connect(proxy.sock, &sa, sizeof(struct sockaddr_in));
	} else {
		ret = connect(proxy.sock, &sa, sizeof(struct sockaddr_in6));
	}
	if (ret) {
		LOG_ERR("connect() failed: %d", -errno);
		ret = -errno;
		goto exit_cli;
	}

	k_thread_create(&tcp_thread, tcp_thread_stack,
			K_THREAD_STACK_SIZEOF(tcp_thread_stack),
			tcpcli_thread_func, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);

	proxy.role = TCP_ROLE_CLIENT;
	rsp_send("\r\n#XTCPCLI: %d,\"connected\"\r\n", proxy.sock);

	return 0;

exit_cli:
#if defined(CONFIG_SLM_NATIVE_TLS)
	if (proxy.sec_tag != INVALID_SEC_TAG) {
		(void)slm_tls_unloadcrdl(proxy.sec_tag);
		proxy.sec_tag = INVALID_SEC_TAG;
	}
#endif
	close(proxy.sock);
	proxy.sock = INVALID_SOCKET;
	rsp_send("\r\n#XTCPCLI: %d,\"not connected\"\r\n", ret);

	return ret;
}

static int do_tcp_client_disconnect(void)
{
	int ret = 0;

	if (proxy.sock == INVALID_SOCKET) {
		return 0;
	}
#if defined(CONFIG_SLM_NATIVE_TLS)
	if (proxy.sec_tag != INVALID_SEC_TAG) {
		(void)slm_tls_unloadcrdl(proxy.sec_tag);
		proxy.sec_tag = INVALID_SEC_TAG;
	}
#endif
	ret = close(proxy.sock);
	if (ret < 0) {
		LOG_WRN("close() failed: %d", -errno);
		ret = -errno;
	} else {
		proxy.sock = INVALID_SOCKET;
	}
	if (ret == 0 &&
	    k_thread_join(&tcp_thread, K_SECONDS(CONFIG_SLM_TCP_POLL_TIME * 2)) != 0) {
		LOG_WRN("Wait for thread terminate failed");
	}
	rsp_send("\r\n#XTCPCLI: %d,\"disconnected\"\r\n", ret);

	return ret;
}

static int do_tcp_send(const uint8_t *data, int datalen)
{
	int ret = 0;
	uint32_t offset = 0;
	int sock = proxy.sock;

	if (proxy.role == TCP_ROLE_SERVER) {
		sock = proxy.sock_peer;
	}

	while (offset < datalen) {
		ret = send(sock, data + offset, datalen - offset, 0);
		if (ret < 0) {
			LOG_ERR("send() failed: %d, sent: %d", -errno, offset);
			ret = -errno;
			break;
		} else {
			offset += ret;
		}
	}

	if (ret >= 0) {
		rsp_send("\r\n#XTCPSEND: %d\r\n", offset);
		return 0;
	}

	return ret;
}

static int do_tcp_send_datamode(const uint8_t *data, int datalen)
{
	int ret = 0;
	uint32_t offset = 0;
	int sock = proxy.sock;

	if (proxy.role == TCP_ROLE_SERVER) {
		sock = proxy.sock_peer;
	}

	while (offset < datalen) {
		ret = send(sock, data + offset, datalen - offset, 0);
		if (ret < 0) {
			LOG_ERR("send() failed: %d, sent: %d", -errno, offset);
			break;
		} else {
			offset += ret;
		}
	}

	return (offset > 0) ? offset : -1;
}

static int tcp_datamode_callback(uint8_t op, const uint8_t *data, int len, uint8_t flags)
{
	int ret = 0;

	ARG_UNUSED(flags);

	if (op == DATAMODE_SEND) {
		ret = do_tcp_send_datamode(data, len);
		LOG_INF("datamode send: %d", ret);
	} else if (op == DATAMODE_EXIT) {
		LOG_DBG("datamode exit");
	}

	return ret;
}

/* Server-initiated disconnect */
static void tcpsvr_terminate_connection(int cause)
{
	if (in_datamode()) {
		(void)exit_datamode_handler(cause);
	}
	if (proxy.sock_peer != INVALID_SOCKET) {
		if (close(proxy.sock_peer) < 0) {
			LOG_WRN("close() error: %d", -errno);
		}
		proxy.sock_peer = INVALID_SOCKET;
		rsp_send("\r\n#XTCPSVR: %d,\"disconnected\"\r\n", cause);
	}
}

/* TCP server thread */
static void tcpsvr_thread_func(void *p1, void *p2, void *p3)
{
	int ret;
	struct pollfd fds[2];

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	fds[0].fd = proxy.sock;
	fds[0].events = POLLIN;
	/** The field fd contains a file descriptor for an open file.  If this field is negative,
	 * then the corresponding events field is ignored and the revents field returns zero.
	 */
	fds[1].fd = INVALID_SOCKET;
	fds[1].events = POLLIN;
	while (true) {
		ret = poll(fds, 2, MSEC_PER_SEC * CONFIG_SLM_TCP_POLL_TIME);
		if (server_stop_pending) {  /* server wait to stop */
			ret = 0;
			break;
		}
		if (ret < 0) { /* IO error */
			LOG_WRN("poll() error: %d", -errno);
			ret = -EIO;
			break;
		}
		if (ret == 0) { /* timeout */
			continue;
		}
		LOG_DBG("fds[0] events 0x%08x", fds[0].revents);
		LOG_DBG("fds[1] events 0x%08x", fds[1].revents);
		/* Listening socket events must be handled first*/
		if (fds[0].revents) {
			if ((fds[0].revents & POLLERR) == POLLERR) {
				LOG_ERR("0: POLLERR");
				ret = -EIO;
				break;
			}
			if ((fds[0].revents & POLLHUP) == POLLHUP) {
				LOG_WRN("0: POLLHUP");
				ret = -ECONNRESET;
				break;
			}
			if ((fds[0].revents & POLLNVAL) == POLLNVAL) {
				/* server stopped by AT command */
				LOG_WRN("0: POLLNVAL");
				ret = -ENETDOWN;
				break;
			}
			if ((fds[0].revents & POLLIN) != POLLIN) {
				/* Ignore and check whether there are client events */
				goto client_events;
			}

			/* Accept incoming connection */
			char peer_addr[INET6_ADDRSTRLEN] = {0};
			socklen_t len;

			if (proxy.family == AF_INET) {
				struct sockaddr_in client;

				len = sizeof(struct sockaddr_in);
				ret = accept(proxy.sock, (struct sockaddr *)&client, &len);
				if (ret == -1) {
					LOG_WRN("accept(ipv4) error: %d", -errno);
					goto client_events;
				}
				(void)inet_ntop(AF_INET, &client.sin_addr, peer_addr,
					sizeof(peer_addr));
			} else {
				struct sockaddr_in6 client;

				len = sizeof(struct sockaddr_in6);
				ret = accept(proxy.sock, (struct sockaddr *)&client, &len);
				if (ret == -1) {
					LOG_WRN("accept(ipv6) error: %d", -errno);
					goto client_events;
				}
				(void)inet_ntop(AF_INET6, &client.sin6_addr, peer_addr,
					sizeof(peer_addr));
			}
			if (fds[1].fd >= 0) {
				LOG_WRN("Full. Close connection.");
				close(ret);
				goto client_events;
			}
			proxy.sock_peer = ret;
			fds[1].fd = proxy.sock_peer;
			rsp_send("\r\n#XTCPSVR: \"%s\",\"connected\"\r\n", peer_addr);
			LOG_DBG("New connection - %d", proxy.sock_peer);
		}
client_events:
		/* Incoming socket events */
		if (fds[1].revents) {
			/* Process POLLIN first to get the data, even if there are errors. */
			if ((fds[1].revents & POLLIN) == POLLIN) {
				ret = recv(fds[1].fd, (void *)slm_data_buf,
					   sizeof(slm_data_buf), 0);
				if (ret < 0) {
					LOG_ERR("recv() error: %d", -errno);
					tcpsvr_terminate_connection(-errno);
					fds[1].fd = INVALID_SOCKET;
					continue;
				}
				if (ret > 0) {
					if (!in_datamode()) {
						rsp_send("\r\n#XTCPDATA: %d\r\n", ret);
					}
					data_send(slm_data_buf, ret);
				}
			}
			if ((fds[1].revents & POLLERR) == POLLERR) {
				LOG_ERR("1: POLLERR");
				tcpsvr_terminate_connection(-EIO);
				fds[1].fd = INVALID_SOCKET;
				continue;
			}
			if ((fds[1].revents & POLLHUP) == POLLHUP) {
				LOG_ERR("1: POLLHUP");
				tcpsvr_terminate_connection(0);
				fds[1].fd = INVALID_SOCKET;
				continue;
			}
			if ((fds[1].revents & POLLNVAL) == POLLNVAL) {
				LOG_WRN("1: POLLNVAL");
				tcpsvr_terminate_connection(-EBADF);
				fds[1].fd = INVALID_SOCKET;
			}
		}
	}

#if defined(CONFIG_SLM_NATIVE_TLS)
	if (proxy.sec_tag != INVALID_SEC_TAG) {
		(void)slm_tls_unloadcrdl(proxy.sec_tag);
		proxy.sec_tag = INVALID_SEC_TAG;
	}
#endif
	tcpsvr_terminate_connection(ret);
	if (proxy.sock != INVALID_SOCKET) {
		(void)close(proxy.sock);
		proxy.sock = INVALID_SOCKET;
	}
	rsp_send("\r\n#XTCPSVR: %d,\"stopped\"\r\n", ret);
	server_stop_pending = false;
	LOG_INF("TCP server thread terminated");
}

/* TCP client thread */
static void tcpcli_thread_func(void *p1, void *p2, void *p3)
{
	int ret;
	static struct pollfd fds;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	fds.fd = proxy.sock;
	fds.events = POLLIN;
	while (true) {
		ret = poll(&fds, 1, MSEC_PER_SEC * CONFIG_SLM_TCP_POLL_TIME);
		if (ret < 0) {  /* IO error */
			LOG_WRN("poll() error: %d", ret);
			ret = -EIO;
			break;
		}
		if (ret == 0) {  /* timeout */
			continue;
		}
		LOG_DBG("Poll events 0x%08x", fds.revents);
		if ((fds.revents & POLLERR) == POLLERR) {
			LOG_ERR("POLLERR");
			ret = -EIO;
			break;
		}
		if ((fds.revents & POLLNVAL) == POLLNVAL) {
			/* client disconnected locally by AT command */
			LOG_WRN("POLLNVAL");
			ret = -ENETDOWN;
			break;
		}
		if ((fds.revents & POLLHUP) == POLLHUP) {
			/* client disconnected by remote or lose LTE connection */
			LOG_WRN("POLLHUP");
			ret = -ECONNRESET;
			break;
		}
		if ((fds.revents & POLLIN) != POLLIN) {
			continue;
		}
		ret = recv(fds.fd, (void *)slm_data_buf, sizeof(slm_data_buf), 0);
		if (ret < 0) {
			LOG_WRN("recv() error: %d", -errno);
			continue;
		}
		if (ret == 0) {
			continue;
		}
		if (in_datamode()) {
			data_send(slm_data_buf, ret);
		} else {
			rsp_send("\r\n#XTCPDATA: %d\r\n", ret);
			data_send(slm_data_buf, ret);
		}
	}

	if (in_datamode()) {
		(void)exit_datamode_handler(ret);
	}
	if (proxy.sock != INVALID_SOCKET) {
		(void)close(proxy.sock);
		proxy.sock = INVALID_SOCKET;
		rsp_send("\r\n#XTCPCLI: %d,\"disconnected\"\r\n", ret);
	}

	LOG_INF("TCP client thread terminated");
}

/* Handles AT#XTCPSVR commands. */
int handle_at_tcp_server(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	uint16_t port;
	int param_count = at_params_valid_count_get(&slm_at_param_list);

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&slm_at_param_list, 1, &op);
		if (err) {
			return err;
		}
		if (op == SERVER_START || op == SERVER_START6) {
			if (proxy.sock != INVALID_SOCKET) {
				LOG_ERR("Server is running.");
				return -EINVAL;
			}
			err = at_params_unsigned_short_get(&slm_at_param_list, 2, &port);
			if (err) {
				return err;
			}
			proxy.sec_tag = INVALID_SEC_TAG;
			if (param_count > 3) {
				err = at_params_int_get(&slm_at_param_list, 3, &proxy.sec_tag);
				if (err) {
					return err;
				}
			}
			proxy.family = (op == SERVER_START) ? AF_INET : AF_INET6;
			err = do_tcp_server_start(port);
		} else if (op == SERVER_STOP) {
			err = do_tcp_server_stop();
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		rsp_send("\r\n#XTCPSVR: %d,%d,%d\r\n",
			proxy.sock, proxy.sock_peer, proxy.family);
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XTCPSVR: (%d,%d,%d),<port>,<sec_tag>\r\n",
			SERVER_STOP, SERVER_START, SERVER_START6);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/* Handles AT#XTCPCLI commands. */
int handle_at_tcp_client(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	int param_count = at_params_valid_count_get(&slm_at_param_list);

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&slm_at_param_list, 1, &op);
		if (err) {
			return err;
		}
		if (op == CLIENT_CONNECT || op == CLIENT_CONNECT6) {
			uint16_t port;
			char url[SLM_MAX_URL];
			int size = SLM_MAX_URL;

			if (proxy.sock != INVALID_SOCKET) {
				LOG_ERR("Client is connected.");
				return -EINVAL;
			}
			err = util_string_get(&slm_at_param_list, 2, url, &size);
			if (err) {
				return err;
			}
			err = at_params_unsigned_short_get(&slm_at_param_list, 3, &port);
			if (err) {
				return err;
			}
			proxy.sec_tag = INVALID_SEC_TAG;
			if (param_count > 4) {
				err = at_params_int_get(&slm_at_param_list, 4, &proxy.sec_tag);
				if (err) {
					return err;
				}
			}
			proxy.family = (op == CLIENT_CONNECT) ? AF_INET : AF_INET6;
			err = do_tcp_client_connect(url, port);
		} else if (op == CLIENT_DISCONNECT) {
			err = do_tcp_client_disconnect();
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		rsp_send("\r\n#XTCPCLI: %d,%d\r\n", proxy.sock, proxy.family);
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XTCPCLI: (%d,%d,%d),<url>,<port>,<sec_tag>\r\n",
			CLIENT_DISCONNECT, CLIENT_CONNECT, CLIENT_CONNECT6);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/* Handles AT#XTCPSEND command. */
int handle_at_tcp_send(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char data[SLM_MAX_PAYLOAD_SIZE + 1] = {0};
	int size;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&slm_at_param_list) > 1) {
			size = sizeof(data);
			err = util_string_get(&slm_at_param_list, 1, data, &size);
			if (err) {
				return err;
			}
			err = do_tcp_send(data, size);
		} else {
			err = enter_datamode(tcp_datamode_callback);
		}
		break;

	default:
		break;
	}

	return err;
}

/* Handles AT#XTCPHANGUP commands. */
int handle_at_tcp_hangup(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	int handle;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (proxy.role != TCP_ROLE_SERVER || proxy.sock_peer == INVALID_SOCKET) {
			return -EINVAL;
		}
		err = at_params_int_get(&slm_at_param_list, 1, &handle);
		if (err) {
			return err;
		}
		if (handle != proxy.sock_peer) {
			return -EINVAL;
		}
		tcpsvr_terminate_connection(0);
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XTCPHANGUP: <handle>\r\n");
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief API to initialize TCP proxy AT commands handler
 */
int slm_at_tcp_proxy_init(void)
{
	proxy.sock      = INVALID_SOCKET;
	proxy.family    = AF_UNSPEC;
	proxy.sock_peer = INVALID_SOCKET;
	proxy.role      = INVALID_ROLE;
	proxy.sec_tag   = INVALID_SEC_TAG;

	return 0;
}

/**@brief API to uninitialize TCP proxy AT commands handler
 */
int slm_at_tcp_proxy_uninit(void)
{
	if (proxy.role == TCP_ROLE_CLIENT) {
		return do_tcp_client_disconnect();
	}
	if (proxy.role == TCP_ROLE_SERVER) {
		return do_tcp_server_stop();
	}

	return 0;
}

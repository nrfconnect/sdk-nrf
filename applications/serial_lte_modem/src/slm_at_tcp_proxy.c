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
#include <zephyr/net/tls_credentials.h>
#include <zephyr/posix/sys/eventfd.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_socket.h"
#include "slm_at_tcp_proxy.h"
#if defined(CONFIG_SLM_NATIVE_TLS)
#include "slm_native_tls.h"
#endif

LOG_MODULE_REGISTER(slm_tcp, CONFIG_SLM_LOG_LEVEL);

#define THREAD_STACK_SIZE	KB(4)
#define THREAD_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO

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

/**@brief Commands conveyed to threads. */
enum proxy_event {
	PROXY_CLOSE = 0,
	PROXY_CLOSE_PEER,
	PROXY_EVENT_COUNT
};
K_MSGQ_DEFINE(proxy_event_queue, sizeof(enum proxy_event), PROXY_EVENT_COUNT, 1);

static struct k_thread tcp_thread;
static K_THREAD_STACK_DEFINE(tcp_thread_stack, THREAD_STACK_SIZE);

static struct tcp_proxy {
	int sock;		/* Socket descriptor. */
	int family;		/* Socket address family */
	sec_tag_t sec_tag;	/* Security tag of the credential */
	int sock_peer;		/* Socket descriptor for peer. */
	enum slm_tcp_role role;	/* Client or Server proxy */
	int peer_verify;	/* Peer verification level for TLS connection. */
	bool hostname_verify;	/* Verify hostname against the certificate. */
	int efd;		/* Event file descriptor for signaling threads. */
} proxy;

/** forward declaration of thread function **/
static void tcpcli_thread_func(void *p1, void *p2, void *p3);
static void tcpsvr_thread_func(void *p1, void *p2, void *p3);

static int do_tcp_server_start(uint16_t port)
{
	int ret;
	int reuseaddr = 1;

	/* Open socket */
	if (proxy.sec_tag == INVALID_SEC_TAG) {
		ret = zsock_socket(proxy.family, SOCK_STREAM, IPPROTO_TCP);
	} else {
		ret = zsock_socket(proxy.family, SOCK_STREAM, IPPROTO_TLS_1_2);
	}
	if (ret < 0) {
		LOG_ERR("zsock_socket() failed: %d", -errno);
		ret = -errno;
		goto exit_svr;
	}
	proxy.sock = ret;

	if (proxy.sec_tag != INVALID_SEC_TAG) {
#ifndef CONFIG_SLM_NATIVE_TLS
		LOG_ERR("Not supported");
		return -ENOTSUP;
#else
		ret = slm_native_tls_load_credentials(proxy.sec_tag);
		if (ret < 0) {
			LOG_ERR("Failed to load sec tag: %d (%d)", proxy.sec_tag, ret);
			return ret;
		}
		int tls_native = 1;

		/* Must be the first socket option to set. */
		ret = zsock_setsockopt(proxy.sock, SOL_TLS, TLS_NATIVE, &tls_native,
					sizeof(tls_native));
		if (ret) {
			ret = errno;
			goto exit_svr;
		}
#endif
		sec_tag_t sec_tag_list[1] = { proxy.sec_tag };

		ret = zsock_setsockopt(proxy.sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
				 sizeof(sec_tag_t));
		if (ret) {
			LOG_ERR("zsock_setsockopt(TLS_SEC_TAG_LIST) error: %d", -errno);
			ret = -errno;
			goto exit_svr;
		}
	}

	ret = zsock_setsockopt(proxy.sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int));
	if (ret < 0) {
		LOG_ERR("zsock_setsockopt(SO_REUSEADDR): %d", -errno);
		ret = -errno;
		goto exit_svr;
	}

	/* Bind to local port */
	ret = slm_bind_to_local_addr(proxy.sock, proxy.family, port);
	if (ret) {
		goto exit_svr;
	}

	/* Enable listen */
	ret = zsock_listen(proxy.sock, 1);
	if (ret < 0) {
		LOG_ERR("zsock_listen() failed: %d", -errno);
		ret = -EINVAL;
		goto exit_svr;
	}

	proxy.efd = eventfd(0, 0);
	proxy.role = TCP_ROLE_SERVER;
	k_thread_create(&tcp_thread, tcp_thread_stack,
			K_THREAD_STACK_SIZEOF(tcp_thread_stack),
			tcpsvr_thread_func, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);
	rsp_send("\r\n#XTCPSVR: %d,\"started\"\r\n", proxy.sock);

	return 0;

exit_svr:
	if (proxy.sock != INVALID_SOCKET) {
		zsock_close(proxy.sock);
		proxy.sock = INVALID_SOCKET;
	}
	rsp_send("\r\n#XTCPSVR: %d,\"not started\"\r\n", ret);

	return ret;
}

static int do_tcp_proxy_close(void)
{
	int ret;
	const enum proxy_event event = PROXY_CLOSE;

	if (proxy.efd == INVALID_SOCKET) {
		return 0;
	}
	ret = k_msgq_put(&proxy_event_queue, &event, K_NO_WAIT);
	if (ret) {
		return ret;
	}
	ret = eventfd_write(proxy.efd, 1);
	if (ret < 0) {
		return -errno;
	}
	ret = k_thread_join(&tcp_thread, K_SECONDS(CONFIG_SLM_TCP_POLL_TIME));
	if (ret) {
		LOG_WRN("Thread terminate failed: %d", ret);

		/* Attempt to make the thread exit by closing the sockets. */
		if (proxy.sock != INVALID_SOCKET) {
			zsock_close(proxy.sock);
			proxy.sock = INVALID_SOCKET;
		}
		if (proxy.sock_peer != INVALID_SOCKET) {
			zsock_close(proxy.sock_peer);
			proxy.sock_peer = INVALID_SOCKET;
		}
	}
	k_msgq_purge(&proxy_event_queue);
	zsock_close(proxy.efd);
	proxy.efd = INVALID_SOCKET;

	return ret;
}

static int do_tcp_client_connect(const char *url, uint16_t port)
{
	int ret;
	struct sockaddr sa;

	/* Open socket */
	if (proxy.sec_tag == INVALID_SEC_TAG) {
		ret = zsock_socket(proxy.family, SOCK_STREAM, IPPROTO_TCP);
	} else {
		ret = zsock_socket(proxy.family, SOCK_STREAM, IPPROTO_TLS_1_2);
	}
	if (ret < 0) {
		LOG_ERR("zsock_socket() failed: %d", -errno);
		return ret;
	}
	proxy.sock = ret;

	if (proxy.sec_tag != INVALID_SEC_TAG) {
#if defined(CONFIG_SLM_NATIVE_TLS)
		ret = slm_native_tls_load_credentials(proxy.sec_tag);
		if (ret < 0) {
			LOG_ERR("Failed to load sec tag: %d (%d)", proxy.sec_tag, ret);
			goto exit_cli;
		}
		int tls_native = 1;

		/* Must be the first socket option to set. */
		ret = zsock_setsockopt(proxy.sock, SOL_TLS, TLS_NATIVE, &tls_native,
				       sizeof(tls_native));
		if (ret) {
			ret = errno;
			goto exit_cli;
		}
#endif
		sec_tag_t sec_tag_list[1] = { proxy.sec_tag };

		ret = zsock_setsockopt(proxy.sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
				 sizeof(sec_tag_t));
		if (ret) {
			LOG_ERR("zsock_setsockopt(TLS_SEC_TAG_LIST) error: %d", -errno);
			ret = -errno;
			goto exit_cli;
		}
		ret = zsock_setsockopt(proxy.sock, SOL_TLS, TLS_PEER_VERIFY, &proxy.peer_verify,
				 sizeof(proxy.peer_verify));
		if (ret) {
			LOG_ERR("zsock_setsockopt(TLS_PEER_VERIFY) error: %d", errno);
			ret = -errno;
			goto exit_cli;
		}
		if (proxy.hostname_verify) {
			ret = zsock_setsockopt(proxy.sock, SOL_TLS, TLS_HOSTNAME, url, strlen(url));
		} else {
			ret = zsock_setsockopt(proxy.sock, SOL_TLS, TLS_HOSTNAME, NULL, 0);
		}
		if (ret) {
			LOG_ERR("zsock_setsockopt(TLS_HOSTNAME) error: %d", errno);
			ret = -errno;
			goto exit_cli;
		}
	}

	/* Connect to remote host */
	ret = util_resolve_host(0, url, port, proxy.family, &sa);
	if (ret) {
		goto exit_cli;
	}
	if (sa.sa_family == AF_INET) {
		ret = zsock_connect(proxy.sock, &sa, sizeof(struct sockaddr_in));
	} else {
		ret = zsock_connect(proxy.sock, &sa, sizeof(struct sockaddr_in6));
	}
	if (ret) {
		LOG_ERR("zsock_connect() failed: %d", -errno);
		ret = -errno;
		goto exit_cli;
	}

	proxy.efd = eventfd(0, 0);
	proxy.role = TCP_ROLE_CLIENT;
	k_thread_create(&tcp_thread, tcp_thread_stack,
			K_THREAD_STACK_SIZEOF(tcp_thread_stack),
			tcpcli_thread_func, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);

	rsp_send("\r\n#XTCPCLI: %d,\"connected\"\r\n", proxy.sock);

	return 0;

exit_cli:
	zsock_close(proxy.sock);
	proxy.sock = INVALID_SOCKET;
	rsp_send("\r\n#XTCPCLI: %d,\"not connected\"\r\n", ret);

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
		ret = zsock_send(sock, data + offset, datalen - offset, 0);
		if (ret < 0) {
			LOG_ERR("zsock_send() failed: %d, sent: %d", -errno, offset);
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
		ret = zsock_send(sock, data + offset, datalen - offset, 0);
		if (ret < 0) {
			LOG_ERR("zsock_send() failed: %d, sent: %d", -errno, offset);
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
		LOG_DBG("datamode send: %d", ret);
	} else if (op == DATAMODE_EXIT) {
		LOG_DBG("datamode exit");
		if ((flags & SLM_DATAMODE_FLAGS_EXIT_HANDLER) != 0) {
			/* Datamode exited unexpectedly. */
			rsp_send(CONFIG_SLM_DATAMODE_TERMINATOR);
		}
	}

	return ret;
}

/* Handle peer socket closure from the server side.
 * - Call only from TCP server thread.
 */
static void tcpsvr_terminate_connection(int cause)
{
	if (proxy.sock_peer != INVALID_SOCKET) {
		if (zsock_close(proxy.sock_peer) < 0) {
			LOG_WRN("sock %d zsock_close() error: %d", proxy.sock_peer, -errno);
		}
		proxy.sock_peer = INVALID_SOCKET;

		if (in_datamode()) {
			exit_datamode_handler(cause);
		} else {
			rsp_send("\r\n#XTCPSVR: %d,\"disconnected\"\r\n", cause);
		}
	}
}

/* TCP server thread */
static void tcpsvr_thread_func(void *p1, void *p2, void *p3)
{
	enum {
		SOCK_LISTEN,
		SOCK_PEER,
		EVENT_FD,
		FD_COUNT
	};

	int ret;
	struct zsock_pollfd fds[FD_COUNT];

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);


	fds[SOCK_LISTEN].fd = proxy.sock;
	fds[SOCK_LISTEN].events = ZSOCK_POLLIN;
	/** The field fd contains a file descriptor for an open file.  If this field is negative,
	 * then the corresponding events field is ignored and the revents field returns zero.
	 */
	fds[SOCK_PEER].fd = INVALID_SOCKET;
	fds[SOCK_PEER].events = ZSOCK_POLLIN;
	fds[EVENT_FD].fd = proxy.efd;
	fds[EVENT_FD].events = ZSOCK_POLLIN;
	while (true) {
		ret = zsock_poll(fds, ARRAY_SIZE(fds), MSEC_PER_SEC * CONFIG_SLM_TCP_POLL_TIME);
		if (ret < 0) { /* IO error */
			LOG_WRN("zsock_poll() error: %d", -errno);
			ret = -EIO;
			break;
		}
		if (ret == 0) { /* timeout */
			continue;
		}
		LOG_DBG("listen events 0x%08x", fds[SOCK_LISTEN].revents);
		LOG_DBG("peer events 0x%08x", fds[SOCK_PEER].revents);
		LOG_DBG("efd events 0x%08x", fds[EVENT_FD].revents);
		/* Listening socket events must be handled first*/
		if (fds[SOCK_LISTEN].revents) {
			if ((fds[SOCK_LISTEN].revents & ZSOCK_POLLERR) != 0) {
				LOG_WRN("SOCK_LISTEN (%d): ZSOCK_POLLERR", fds[SOCK_LISTEN].fd);
				ret = -EIO;
				break;
			}
			if ((fds[SOCK_LISTEN].revents & ZSOCK_POLLHUP) != 0) {
				LOG_WRN("SOCK_LISTEN (%d): ZSOCK_POLLHUP", fds[SOCK_LISTEN].fd);
				ret = -ECONNRESET;
				break;
			}
			if ((fds[SOCK_LISTEN].revents & ZSOCK_POLLNVAL) != 0) {
				LOG_WRN("SOCK_LISTEN (%d): ZSOCK_POLLNVAL", fds[SOCK_LISTEN].fd);
				ret = -ENETDOWN;
				break;
			}
			if ((fds[SOCK_LISTEN].revents & ZSOCK_POLLIN) == 0) {
				/* Ignore and check whether there are client events */
				goto client_events;
			}

			/* Accept incoming connection */
			char peer_addr[INET6_ADDRSTRLEN] = {0};
			socklen_t len;

			if (proxy.family == AF_INET) {
				struct sockaddr_in client;

				len = sizeof(struct sockaddr_in);
				ret = zsock_accept(proxy.sock, (struct sockaddr *)&client, &len);
				if (ret == -1) {
					LOG_WRN("zsock_accept(ipv4) error: %d", -errno);
					goto client_events;
				}
				(void)zsock_inet_ntop(AF_INET, &client.sin_addr, peer_addr,
					sizeof(peer_addr));
			} else {
				struct sockaddr_in6 client;

				len = sizeof(struct sockaddr_in6);
				ret = zsock_accept(proxy.sock, (struct sockaddr *)&client, &len);
				if (ret == -1) {
					LOG_WRN("zsock_accept(ipv6) error: %d", -errno);
					goto client_events;
				}
				(void)zsock_inet_ntop(AF_INET6, &client.sin6_addr, peer_addr,
					sizeof(peer_addr));
			}
			if (fds[SOCK_PEER].fd >= 0) {
				LOG_WRN("Full. Close connection.");
				zsock_close(ret);
				goto client_events;
			}
			proxy.sock_peer = ret;
			fds[SOCK_PEER].fd = proxy.sock_peer;
			rsp_send("\r\n#XTCPSVR: \"%s\",\"connected\"\r\n", peer_addr);
			LOG_DBG("New connection - %d", proxy.sock_peer);
		}
client_events:
		/* Incoming socket events */
		if (fds[SOCK_PEER].revents) {
			/* Process ZSOCK_POLLIN first to get the data, even if there are errors. */
			if ((fds[SOCK_PEER].revents & ZSOCK_POLLIN) == ZSOCK_POLLIN) {
				ret = zsock_recv(fds[SOCK_PEER].fd, (void *)slm_data_buf,
					   sizeof(slm_data_buf), ZSOCK_MSG_DONTWAIT);
				if (ret < 0 && errno != EAGAIN) {
					LOG_ERR("zsock_recv() error: %d", -errno);
					tcpsvr_terminate_connection(-errno);
					fds[SOCK_PEER].fd = INVALID_SOCKET;
				}
				if (ret > 0) {
					if (!in_datamode()) {
						rsp_send("\r\n#XTCPDATA: %d\r\n", ret);
					}
					data_send(slm_data_buf, ret);
				}
			}
			if ((fds[SOCK_PEER].revents & ZSOCK_POLLERR) != 0) {
				LOG_WRN("SOCK_PEER (%d): ZSOCK_POLLERR", fds[SOCK_PEER].fd);
				tcpsvr_terminate_connection(-EIO);
				fds[SOCK_PEER].fd = INVALID_SOCKET;
			}
			if ((fds[SOCK_PEER].revents & ZSOCK_POLLHUP) != 0) {
				LOG_DBG("SOCK_PEER (%d): ZSOCK_POLLHUP", fds[SOCK_PEER].fd);
				tcpsvr_terminate_connection(0);
				fds[SOCK_PEER].fd = INVALID_SOCKET;
			}
			if ((fds[SOCK_PEER].revents & ZSOCK_POLLNVAL) != 0) {
				LOG_WRN("SOCK_PEER (%d): ZSOCK_POLLNVAL", fds[SOCK_PEER].fd);
				tcpsvr_terminate_connection(-EBADF);
				fds[SOCK_PEER].fd = INVALID_SOCKET;
			}
		}
		/* Events from AT-commands. */
		if ((fds[EVENT_FD].revents & ZSOCK_POLLIN) != 0) {
			eventfd_t value;

			ret = eventfd_read(fds[EVENT_FD].fd, &value);
			if (ret) {
				LOG_ERR("eventfd_read() failed");
				break;
			}
			enum proxy_event event;

			while ((ret = k_msgq_get(&proxy_event_queue, &event, K_NO_WAIT)) == 0) {
				if (event == PROXY_CLOSE) {
					break;
				} else if (event == PROXY_CLOSE_PEER) {
					LOG_DBG("Close peer");
					tcpsvr_terminate_connection(0);
					fds[SOCK_PEER].fd = INVALID_SOCKET;
				}
			}
			if (!ret && event == PROXY_CLOSE) {
				LOG_DBG("Close proxy");
				break;
			}
		}
		if (fds[EVENT_FD].revents & (ZSOCK_POLLERR | ZSOCK_POLLHUP | ZSOCK_POLLNVAL)) {
			LOG_ERR("efd: unexpected event: %d", fds[EVENT_FD].revents);
			break;
		}
	}

	tcpsvr_terminate_connection(ret);
	zsock_close(proxy.sock);
	proxy.sock = INVALID_SOCKET;

	if (in_datamode()) {
		exit_datamode_handler(ret);
	} else {
		rsp_send("\r\n#XTCPSVR: %d,\"stopped\"\r\n", ret);
	}
	LOG_INF("TCP server thread terminated");
}

/* TCP client thread */
static void tcpcli_thread_func(void *p1, void *p2, void *p3)
{
	enum {
		SOCK,
		EVENT_FD,
		FD_COUNT
	};

	int ret;
	struct zsock_pollfd fds[FD_COUNT];

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	fds[SOCK].fd = proxy.sock;
	fds[SOCK].events = ZSOCK_POLLIN;
	fds[EVENT_FD].fd = proxy.efd;
	fds[EVENT_FD].events = ZSOCK_POLLIN;
	while (true) {
		ret = zsock_poll(fds, ARRAY_SIZE(fds), MSEC_PER_SEC * CONFIG_SLM_TCP_POLL_TIME);
		if (ret < 0) {
			LOG_WRN("zsock_poll() error: %d", ret);
			ret = -EIO;
			break;
		}
		if (ret == 0) {
			/* timeout */
			continue;
		}
		LOG_DBG("sock events 0x%08x", fds[SOCK].revents);
		LOG_DBG("efd events 0x%08x", fds[EVENT_FD].revents);
		if ((fds[SOCK].revents & ZSOCK_POLLIN) != 0) {
			ret = zsock_recv(fds[SOCK].fd, (void *)slm_data_buf, sizeof(slm_data_buf),
				   ZSOCK_MSG_DONTWAIT);
			if (ret < 0 && errno != EAGAIN) {
				LOG_WRN("zsock_recv() error: %d", -errno);
			} else if (ret > 0) {
				if (!in_datamode()) {
					rsp_send("\r\n#XTCPDATA: %d\r\n", ret);
				}
				data_send(slm_data_buf, ret);
			}
		}
		if ((fds[SOCK].revents & ZSOCK_POLLERR) != 0) {
			LOG_WRN("SOCK (%d): ZSOCK_POLLERR", fds[SOCK].fd);
			ret = -EIO;
			break;
		}
		if ((fds[SOCK].revents & ZSOCK_POLLNVAL) != 0) {
			LOG_WRN("SOCK (%d): ZSOCK_POLLNVAL", fds[SOCK].fd);
			ret = -ENETDOWN;
			break;
		}
		if ((fds[SOCK].revents & ZSOCK_POLLHUP) != 0) {
			/* Lose LTE connection / remote end close */
			LOG_WRN("SOCK (%d): ZSOCK_POLLHUP", fds[SOCK].fd);
			ret = -ECONNRESET;
			break;
		}
		/* Events from AT-commands. */
		if ((fds[EVENT_FD].revents & ZSOCK_POLLIN) != 0) {
			eventfd_t value;

			/* AT-command event can only close the client. */
			LOG_DBG("Close proxy");
			eventfd_read(fds[EVENT_FD].fd, &value);
			ret = 0;
			break;
		}
		if (fds[EVENT_FD].revents & (ZSOCK_POLLERR | ZSOCK_POLLHUP | ZSOCK_POLLNVAL)) {
			LOG_ERR("efd: unexpected event: %d", fds[EVENT_FD].revents);
			break;
		}
	}

	zsock_close(proxy.sock);
	proxy.sock = INVALID_SOCKET;

	if (in_datamode()) {
		exit_datamode_handler(ret);
	} else {
		rsp_send("\r\n#XTCPCLI: %d,\"disconnected\"\r\n", ret);
	}

	LOG_INF("TCP client thread terminated");
}

SLM_AT_CMD_CUSTOM(xtcpsvr, "AT#XTCPSVR", handle_at_tcp_server);
static int handle_at_tcp_server(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
				uint32_t param_count)
{
	int err = -EINVAL;
	uint16_t op;
	uint16_t port;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		err = at_parser_num_get(parser, 1, &op);
		if (err) {
			return err;
		}
		if (op == SERVER_START || op == SERVER_START6) {
			if (proxy.sock != INVALID_SOCKET || proxy.efd != INVALID_SOCKET) {
				LOG_ERR("Proxy is running.");
				return -EINVAL;
			}
			err = at_parser_num_get(parser, 2, &port);
			if (err) {
				return err;
			}
			proxy.sec_tag = INVALID_SEC_TAG;
			if (param_count > 3) {
				err = at_parser_num_get(parser, 3, &proxy.sec_tag);
				if (err) {
					return err;
				}
			}
			proxy.family = (op == SERVER_START) ? AF_INET : AF_INET6;
			err = do_tcp_server_start(port);
		} else if (op == SERVER_STOP) {
			err = do_tcp_proxy_close();
		} break;

	case AT_PARSER_CMD_TYPE_READ:
		rsp_send("\r\n#XTCPSVR: %d,%d,%d\r\n",
			proxy.sock, proxy.sock_peer, proxy.family);
		err = 0;
		break;

	case AT_PARSER_CMD_TYPE_TEST:
		rsp_send("\r\n#XTCPSVR: (%d,%d,%d),<port>,<sec_tag>\r\n",
			SERVER_STOP, SERVER_START, SERVER_START6);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

SLM_AT_CMD_CUSTOM(xtcpcli, "AT#XTCPCLI", handle_at_tcp_client);
static int handle_at_tcp_client(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
				uint32_t param_count)
{
	int err = -EINVAL;
	uint16_t op;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		err = at_parser_num_get(parser, 1, &op);
		if (err) {
			return err;
		}
		if (op == CLIENT_CONNECT || op == CLIENT_CONNECT6) {
			uint16_t port;
			char url[SLM_MAX_URL];
			int size = SLM_MAX_URL;

			if (proxy.sock != INVALID_SOCKET || proxy.efd != INVALID_SOCKET) {
				LOG_ERR("Proxy is running.");
				return -EINVAL;
			}
			err = util_string_get(parser, 2, url, &size);
			if (err) {
				return err;
			}
			if (at_parser_num_get(parser, 3, &port)) {
				return -EINVAL;
			}
			proxy.sec_tag = INVALID_SEC_TAG;
			if (param_count > 4) {
				if (at_parser_num_get(parser, 4, &proxy.sec_tag)) {
					return -EINVAL;
				}
			}
			proxy.peer_verify = TLS_PEER_VERIFY_REQUIRED;
			if (param_count > 5) {
				if (at_parser_num_get(parser, 5, &proxy.peer_verify) ||
				    (proxy.peer_verify != TLS_PEER_VERIFY_NONE &&
				     proxy.peer_verify != TLS_PEER_VERIFY_OPTIONAL &&
				     proxy.peer_verify != TLS_PEER_VERIFY_REQUIRED)) {
					return -EINVAL;
				}
			}
			proxy.hostname_verify = true;
			if (param_count > 6) {
				uint16_t hostname_verify;

				if (at_parser_num_get(parser, 6, &hostname_verify) ||
				    (hostname_verify != 0 && hostname_verify != 1)) {
					return -EINVAL;
				}
				proxy.hostname_verify = (bool)hostname_verify;
			}

			proxy.family = (op == CLIENT_CONNECT) ? AF_INET : AF_INET6;
			err = do_tcp_client_connect(url, port);
		} else if (op == CLIENT_DISCONNECT) {
			err = do_tcp_proxy_close();
		} break;

	case AT_PARSER_CMD_TYPE_READ:
		rsp_send("\r\n#XTCPCLI: %d,%d\r\n", proxy.sock, proxy.family);
		err = 0;
		break;

	case AT_PARSER_CMD_TYPE_TEST:
		rsp_send("\r\n#XTCPCLI: (%d,%d,%d),<url>,<port>,"
			 "<sec_tag>,<peer_verify>,<hostname_verify>\r\n",
			 CLIENT_DISCONNECT, CLIENT_CONNECT, CLIENT_CONNECT6);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

SLM_AT_CMD_CUSTOM(xtcpsend, "AT#XTCPSEND", handle_at_tcp_send);
static int handle_at_tcp_send(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			      uint32_t param_count)
{
	int err = -EINVAL;
	char data[SLM_MAX_PAYLOAD_SIZE + 1] = {0};
	int size;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		if (param_count > 1) {
			size = sizeof(data);
			err = util_string_get(parser, 1, data, &size);
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

SLM_AT_CMD_CUSTOM(xtcphangup, "AT#XTCPHANGUP", handle_at_tcp_hangup);
static int handle_at_tcp_hangup(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
				uint32_t)
{
	int err = -EINVAL;
	int handle;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		if (proxy.role != TCP_ROLE_SERVER || proxy.sock_peer == INVALID_SOCKET) {
			return -EINVAL;
		}
		err = at_parser_num_get(parser, 1, &handle);
		if (err) {
			return err;
		}
		if (handle != proxy.sock_peer || proxy.efd == INVALID_SOCKET) {
			return -EINVAL;
		}
		const enum proxy_event event = PROXY_CLOSE_PEER;

		err = k_msgq_put(&proxy_event_queue, &event, K_NO_WAIT);
		if (err) {
			return err;
		}
		err = eventfd_write(proxy.efd, 1);
		if (err < 0) {
			err = -errno;
		}
		break;

	case AT_PARSER_CMD_TYPE_TEST:
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
	proxy.sock	= INVALID_SOCKET;
	proxy.family	= AF_UNSPEC;
	proxy.sock_peer	= INVALID_SOCKET;
	proxy.role	= INVALID_ROLE;
	proxy.sec_tag	= INVALID_SEC_TAG;
	proxy.efd	= INVALID_SOCKET;

	return 0;
}

/**@brief API to uninitialize TCP proxy AT commands handler
 */
int slm_at_tcp_proxy_uninit(void)
{

	return do_tcp_proxy_close();
}

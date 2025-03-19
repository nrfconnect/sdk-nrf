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
#include "slm_at_udp_proxy.h"
#if defined(CONFIG_SLM_NATIVE_TLS)
#include "slm_native_tls.h"
#endif

LOG_MODULE_REGISTER(slm_udp, CONFIG_SLM_LOG_LEVEL);

#define THREAD_STACK_SIZE	KB(4)
#define THREAD_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO

/*
 * Known limitation in this version
 * - Multiple concurrent
 */

/**@brief Proxy operations. */
enum slm_udp_proxy_operation {
	SERVER_STOP,
	CLIENT_DISCONNECT = SERVER_STOP,
	SERVER_START,
	CLIENT_CONNECT = SERVER_START,
	SERVER_START6,
	CLIENT_CONNECT6 = SERVER_START6
};

static struct k_thread udp_thread;
static K_THREAD_STACK_DEFINE(udp_thread_stack, THREAD_STACK_SIZE);
static k_tid_t udp_thread_id;

/**@brief Proxy roles. */
enum slm_udp_role {
	UDP_ROLE_CLIENT,
	UDP_ROLE_SERVER
};

static struct udp_proxy {
	int sock;		/* Socket descriptor. */
	int family;		/* Socket address family */
	sec_tag_t sec_tag;	/* Security tag of the credential */
	int peer_verify;	/* Peer verification level for DTLS connection. */
	bool hostname_verify;	/* Verify hostname against the certificate. */
	int dtls_cid;		/* DTLS connection identifier. */
	enum slm_udp_role role;	/* Client or Server proxy */
	int efd;		/* Event file descriptor for signaling threads. */
	struct sockaddr_storage remote; /* remote host */
} proxy;

/** forward declaration of thread function **/
static void udp_thread_func(void *p1, void *p2, void *p3);

static int do_udp_server_start(uint16_t port)
{
	int ret;

	/* Open socket */
	if (proxy.sec_tag == INVALID_SEC_TAG) {
		ret = zsock_socket(proxy.family, SOCK_DGRAM, IPPROTO_UDP);
	} else {
		ret = zsock_socket(proxy.family, SOCK_DGRAM, IPPROTO_DTLS_1_2);
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
		ret = -ENOTSUP;
		goto exit_svr;
#else
		ret = slm_native_tls_load_credentials(proxy.sec_tag);
		if (ret < 0) {
			LOG_ERR("Failed to load sec tag: %d (%d)", proxy.sec_tag, ret);
			goto exit_svr;
		}
		const int tls_native = 1;

		/* Must be the first socket option to set. */
		ret = zsock_setsockopt(proxy.sock, SOL_TLS, TLS_NATIVE, &tls_native,
					sizeof(tls_native));
		if (ret) {
			LOG_ERR("zsock_setsockopt(TLS_NATIVE) error: %d", -errno);
			ret = -errno;
			goto exit_svr;
		}
		sec_tag_t sec_tag_list[1] = { proxy.sec_tag };

		ret = zsock_setsockopt(proxy.sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
				 sizeof(sec_tag_t));
		if (ret) {
			LOG_ERR("zsock_setsockopt(TLS_SEC_TAG_LIST) error: %d", -errno);
			ret = -errno;
			goto exit_svr;
		}
		int tls_role = TLS_DTLS_ROLE_SERVER;

		ret = zsock_setsockopt(proxy.sock, SOL_TLS, TLS_DTLS_ROLE, &tls_role, sizeof(int));
		if (ret) {
			LOG_ERR("zsock_setsockopt(TLS_DTLS_ROLE) error: %d", -errno);
			ret = -errno;
			goto exit_svr;
		}
#endif
	}
	int reuseaddr = 1;

	ret = zsock_setsockopt(proxy.sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int));
	if (ret < 0) {
		LOG_ERR("zsock_setsockopt(SO_REUSEADDR) error: %d", -errno);
		ret = -errno;
		goto exit_svr;
	}

	/* Bind to local port */
	ret = slm_bind_to_local_addr(proxy.sock, proxy.family, port);
	if (ret) {
		goto exit_svr;
	}

	proxy.efd = eventfd(0, 0);
	memset(&proxy.remote, 0, sizeof(proxy.remote));
	proxy.role = UDP_ROLE_SERVER;
	udp_thread_id = k_thread_create(&udp_thread, udp_thread_stack,
			K_THREAD_STACK_SIZEOF(udp_thread_stack),
			udp_thread_func, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);

	rsp_send("\r\n#XUDPSVR: %d,\"started\"\r\n", proxy.sock);

	return 0;

exit_svr:
	if (proxy.sock != INVALID_SOCKET) {
		zsock_close(proxy.sock);
		proxy.sock = INVALID_SOCKET;
	}
	rsp_send("\r\n#UDPSVR: %d,\"not started\"\r\n", ret);

	return ret;
}

static int do_udp_proxy_close(void)
{
	int ret = 0;

	if (proxy.efd == INVALID_SOCKET) {
		return 0;
	}
	ret = eventfd_write(proxy.efd, 1);
	if (ret < 0) {
		return -errno;
	}
	ret = k_thread_join(&udp_thread, K_SECONDS(CONFIG_SLM_UDP_POLL_TIME));
	if (ret) {
		LOG_WRN("Thread terminate failed: %d", ret);

		/* Attempt to make the thread exit by closing the socket. */
		if (proxy.sock != INVALID_SOCKET) {
			zsock_close(proxy.sock);
			proxy.sock = INVALID_SOCKET;
		}
	}
	zsock_close(proxy.efd);
	proxy.efd = INVALID_SOCKET;

	return ret;
}

static int do_udp_client_connect(const char *url, uint16_t port)
{
	int ret;
	struct sockaddr sa;
	const bool using_cid = (proxy.dtls_cid != INVALID_DTLS_CID);
	const bool using_dtls = (proxy.sec_tag != INVALID_SEC_TAG);

	/* Open socket */
	ret = zsock_socket(proxy.family, SOCK_DGRAM, using_dtls ? IPPROTO_DTLS_1_2 : IPPROTO_UDP);
	if (ret < 0) {
		LOG_ERR("zsock_socket() failed: %d", -errno);
		return -errno;
	}
	proxy.sock = ret;

	if (using_dtls) {
#if defined(CONFIG_SLM_NATIVE_TLS)
		ret = slm_native_tls_load_credentials(proxy.sec_tag);
		if (ret < 0) {
			LOG_ERR("Failed to load sec tag: %d (%d)", proxy.sec_tag, ret);
			goto cli_exit;
		}
		int tls_native = 1;

		/* Must be the first socket option to set. */
		ret = zsock_setsockopt(proxy.sock, SOL_TLS, TLS_NATIVE, &tls_native,
				       sizeof(tls_native));
		if (ret) {
			LOG_ERR("zsock_setsockopt(TLS_NATIVE) error: %d", -errno);
			ret = errno;
			goto cli_exit;
		}
#endif
		sec_tag_t sec_tag_list[1] = { proxy.sec_tag };

		ret = zsock_setsockopt(proxy.sock, SOL_TLS, TLS_SEC_TAG_LIST,
				sec_tag_list, sizeof(sec_tag_t));
		if (ret) {
			LOG_ERR("set tag list failed: %d", -errno);
			ret = -errno;
			goto cli_exit;
		}
		struct timeval timeout;

		/* Set a timeout shorter than the default one which makes the SLM
		 * irresponsive for too long when the connection to a server fails.
		 */
		timeout = (struct timeval){ .tv_sec = 10 };
		ret = zsock_setsockopt(proxy.sock, SOL_SOCKET, SO_SNDTIMEO, &timeout,
				       sizeof(timeout));
		if (ret) {
			ret = -errno;
			LOG_ERR("Setting timeout failed: %d", ret);
			goto cli_exit;
		}

		if (using_cid) {
			if (zsock_setsockopt(proxy.sock, SOL_TLS, TLS_DTLS_CID,
							&proxy.dtls_cid, sizeof(proxy.dtls_cid))) {
				ret = -errno;
				LOG_WRN("Setting DTLS CID (%d) failed: %d", proxy.dtls_cid, ret);
				goto cli_exit;
			}
		}
		ret = zsock_setsockopt(proxy.sock, SOL_TLS, TLS_PEER_VERIFY, &proxy.peer_verify,
				 sizeof(proxy.peer_verify));
		if (ret) {
			LOG_ERR("zsock_setsockopt(TLS_PEER_VERIFY) error: %d", errno);
			ret = -errno;
			goto cli_exit;
		}
		if (proxy.hostname_verify) {
			ret = zsock_setsockopt(proxy.sock, SOL_TLS, TLS_HOSTNAME, url, strlen(url));
		} else {
			ret = zsock_setsockopt(proxy.sock, SOL_TLS, TLS_HOSTNAME, NULL, 0);
		}
		if (ret) {
			LOG_ERR("zsock_setsockopt(TLS_HOSTNAME) error: %d", errno);
			ret = -errno;
			goto cli_exit;
		}
	}

	/* Connect to remote host */
	ret = util_resolve_host(0, url, port, proxy.family, &sa);
	if (ret) {
		goto cli_exit;
	}

	LOG_INF("Connecting to %s:%d (%s)...", url, port, using_dtls ? "DTLS" : "UDP");
	const size_t size = (sa.sa_family == AF_INET) ?
		sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);

	ret = zsock_connect(proxy.sock, &sa, size);
	if (ret < 0) {
		LOG_ERR("zsock_connect() failed: %d", -errno);
		ret = -errno;
		goto cli_exit;
	}
	LOG_INF("Connected.");

	proxy.efd = eventfd(0, 0);
	proxy.role = UDP_ROLE_CLIENT;
	udp_thread_id = k_thread_create(&udp_thread, udp_thread_stack,
			K_THREAD_STACK_SIZEOF(udp_thread_stack),
			udp_thread_func, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);

	rsp_send("\r\n#XUDPCLI: %d,\"connected\"\r\n", proxy.sock);

	return 0;

cli_exit:
	zsock_close(proxy.sock);
	proxy.sock = INVALID_SOCKET;
	rsp_send("\r\n#XUDPCLI: %d,\"not connected\"\r\n", ret);

	return ret;
}

static int do_udp_send(const uint8_t *data, int datalen)
{
	int ret = 0;
	uint32_t offset = 0;

	while (offset < datalen) {
		if (proxy.role == UDP_ROLE_SERVER) {
			/* send to remembered remote */
			ret = zsock_sendto(proxy.sock, data + offset, datalen - offset, 0,
					(struct sockaddr *)&proxy.remote, sizeof(proxy.remote));
		} else {
			ret = zsock_send(proxy.sock, data + offset, datalen - offset, 0);
		}
		if (ret < 0) {
			LOG_ERR("zsock_send()/zsock_sendto() failed: %d, sent: %d", -errno, offset);
			ret = -errno;
			break;
		} else {
			offset += ret;
		}
	}

	if (ret >= 0) {
		rsp_send("\r\n#XUDPSEND: %d\r\n", offset);
		return 0;
	}

	return ret;
}

static int do_udp_send_datamode(const uint8_t *data, int datalen)
{
	int ret = 0;
	uint32_t offset = 0;

	while (offset < datalen) {
		if (proxy.role == UDP_ROLE_SERVER) {
			/* send to remembered remote */
			ret = zsock_sendto(proxy.sock, data + offset, datalen - offset, 0,
					(struct sockaddr *)&proxy.remote, sizeof(proxy.remote));
		} else {
			ret = zsock_send(proxy.sock, data + offset, datalen - offset, 0);
		}
		if (ret < 0) {
			LOG_ERR("zsock_send()/zsock_sendto() failed: %d, sent: %d", -errno, offset);
			break;
		} else {
			offset += ret;
		}
	}

	return (offset > 0) ? offset : -1;
}

static void udp_thread_func(void *p1, void *p2, void *p3)
{
	enum {
		SOCK,
		EVENT_FD,
		FD_COUNT
	};

	int ret;
	struct zsock_pollfd fds[FD_COUNT];
	char peer_addr[INET6_ADDRSTRLEN];
	uint16_t peer_port;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	fds[SOCK].fd = proxy.sock;
	fds[SOCK].events = ZSOCK_POLLIN;
	fds[EVENT_FD].fd = proxy.efd;
	fds[EVENT_FD].events = ZSOCK_POLLIN;

	do {
		ret = zsock_poll(fds, ARRAY_SIZE(fds), MSEC_PER_SEC * CONFIG_SLM_UDP_POLL_TIME);
		if (ret < 0) {  /* IO error */
			LOG_WRN("zsock_poll() error: %d", ret);
			continue;
		}
		if (ret == 0) {  /* timeout */
			continue;
		}
		LOG_DBG("sock events 0x%08x", fds[SOCK].revents);
		LOG_DBG("efd events 0x%08x", fds[EVENT_FD].revents);
		if ((fds[SOCK].revents & ZSOCK_POLLIN) != 0) {
			unsigned int size = sizeof(proxy.remote);

			memset(&proxy.remote, 0, sizeof(proxy.remote));
			ret = zsock_recvfrom(proxy.sock, (void *)slm_data_buf,
					sizeof(slm_data_buf), ZSOCK_MSG_DONTWAIT,
					(struct sockaddr *)&proxy.remote, &size);
			if (ret < 0 && errno != EAGAIN) {
				LOG_WRN("zsock_recv() error: %d", -errno);
			} else if (ret > 0) {
				if (!in_datamode()) {
					util_get_peer_addr((struct sockaddr *)&proxy.remote,
							   peer_addr, &peer_port);
					rsp_send("\r\n#XUDPDATA: %d,\"%s\",%d\r\n", ret, peer_addr,
						 peer_port);
				}
				data_send(slm_data_buf, ret);
			}
		}
		if ((fds[SOCK].revents & ZSOCK_POLLERR) != 0) {
			int value;
			socklen_t len = sizeof(int);

			ret = zsock_getsockopt(proxy.sock, SOL_SOCKET, SO_ERROR, &value, &len);
			if (ret) {
				LOG_ERR("%d : zsock_getsockopt(SO_ERROR) error: %d", fds[SOCK].fd,
					-errno);
				ret = -EIO;
				break;
			}
			if (proxy.role == UDP_ROLE_SERVER && proxy.sec_tag != INVALID_SEC_TAG &&
			    value == ECONNABORTED) {
				util_get_peer_addr((struct sockaddr *)&proxy.remote, peer_addr,
						   &peer_port);
				LOG_WRN("DTLS client timed out: \"%s\",%d\r\n", peer_addr,
					peer_port);
			} else {
				LOG_WRN("%d : ZSOCK_POLLERR", fds[SOCK].fd);
				ret = -EIO;
				break;
			}
		}
		if ((fds[SOCK].revents & ZSOCK_POLLNVAL) != 0) {
			LOG_WRN("%d : ZSOCK_POLLNVAL", fds[SOCK].fd);
			ret = -ENETDOWN;
			break;
		}
		if ((fds[SOCK].revents & ZSOCK_POLLHUP) != 0) {
			if (proxy.role == UDP_ROLE_SERVER && proxy.sec_tag != INVALID_SEC_TAG) {
				util_get_peer_addr((struct sockaddr *)&proxy.remote, peer_addr,
						   &peer_port);
				LOG_INF("DTLS client disconnected: \"%s\",%d\r\n", peer_addr,
					peer_port);
			} else {
				/* Lose LTE connection / remote end close (with DTLS) */
				LOG_WRN("%d : ZSOCK_POLLHUP", fds[SOCK].fd);
				ret = -ECONNRESET;
				break;
			}
		}
		/* Events from AT-commands. */
		if ((fds[EVENT_FD].revents & ZSOCK_POLLIN) != 0) {
			LOG_DBG("Close proxy");
			ret = 0;
			break;
		}
		if (fds[EVENT_FD].revents & (ZSOCK_POLLERR | ZSOCK_POLLHUP | ZSOCK_POLLNVAL)) {
			LOG_ERR("efd: unexpected event: %d", fds[EVENT_FD].revents);
			break;
		}

	} while (true);

	zsock_close(proxy.sock);
	proxy.sock = INVALID_SOCKET;

	if (in_datamode()) {
		exit_datamode_handler(ret);
	} else if (proxy.role == UDP_ROLE_CLIENT) {
		rsp_send("\r\n#XUDPCLI: %d,\"disconnected\"\r\n", ret);
	} else {
		rsp_send("\r\n#XUDPSVR: %d,\"stopped\"\r\n", ret);
	}

	LOG_INF("UDP thread terminated");
}

static int udp_datamode_callback(uint8_t op, const uint8_t *data, int len, uint8_t flags)
{
	int ret = 0;

	if (op == DATAMODE_SEND) {
		if ((flags & SLM_DATAMODE_FLAGS_MORE_DATA) != 0) {
			LOG_ERR("Datamode buffer overflow");
			exit_datamode_handler(-EOVERFLOW);
			return -EOVERFLOW;
		}
		ret = do_udp_send_datamode(data, len);
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

static bool socket_is_in_use(void)
{
	if (proxy.sock == INVALID_SOCKET) {
		return false;
	}
	LOG_ERR("The UDP socket is already in use by the %s it first.",
		(proxy.role == UDP_ROLE_CLIENT) ? "client. Disconnect" : "server. Stop");
	return true;
}

SLM_AT_CMD_CUSTOM(xudpsvr, "AT#XUDPSVR", handle_at_udp_server);
static int handle_at_udp_server(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
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
			if (socket_is_in_use()) {
				return -EINVAL;
			}
			err = at_parser_num_get(parser, 2, &port);
			if (err) {
				return err;
			}
			proxy.sec_tag = INVALID_SEC_TAG;
			if (param_count > 3 &&
			    at_parser_num_get(parser, 3, &proxy.sec_tag)) {
				return -EINVAL;
			}
			proxy.family = (op == SERVER_START) ? AF_INET : AF_INET6;
			err = do_udp_server_start(port);
		} else if (op == SERVER_STOP) {
			err = do_udp_proxy_close();
		} break;

	case AT_PARSER_CMD_TYPE_READ:
		rsp_send("\r\n#XUDPSVR: %d,%d\r\n", proxy.sock, proxy.family);
		err = 0;
		break;

	case AT_PARSER_CMD_TYPE_TEST:
		rsp_send("\r\n#XUDPSVR: (%d,%d,%d),<port>,<sec_tag>\r\n",
			SERVER_STOP, SERVER_START, SERVER_START6);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

SLM_AT_CMD_CUSTOM(xudpcli, "AT#XUDPCLI", handle_at_udp_client);
static int handle_at_udp_client(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
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

			if (socket_is_in_use()) {
				return -EINVAL;
			}
			err = util_string_get(parser, 2, url, &size);
			if (err) {
				return err;
			}
			err = at_parser_num_get(parser, 3, &port);
			if (err) {
				return err;
			}
			proxy.sec_tag = INVALID_SEC_TAG;

			if (param_count > 4) {
				if (at_parser_num_get(parser, 4, &proxy.sec_tag)
				|| proxy.sec_tag == INVALID_SEC_TAG || proxy.sec_tag < 0) {
					return -EINVAL;
				}
			}
			proxy.dtls_cid = INVALID_DTLS_CID;
			if (param_count > 5) {
				if (at_parser_num_get(parser, 5, &proxy.dtls_cid)
				|| !(proxy.dtls_cid == TLS_DTLS_CID_DISABLED
					|| proxy.dtls_cid == TLS_DTLS_CID_SUPPORTED
					|| proxy.dtls_cid == TLS_DTLS_CID_ENABLED)) {
					return -EINVAL;
				}
			}
			proxy.peer_verify = TLS_PEER_VERIFY_REQUIRED;
			if (param_count > 6) {
				if (at_parser_num_get(parser, 6, &proxy.peer_verify) ||
				    (proxy.peer_verify != TLS_PEER_VERIFY_NONE &&
				     proxy.peer_verify != TLS_PEER_VERIFY_OPTIONAL &&
				     proxy.peer_verify != TLS_PEER_VERIFY_REQUIRED)) {
					return -EINVAL;
				}
			}
			proxy.hostname_verify = true;
			if (param_count > 7) {
				uint16_t hostname_verify;

				if (at_parser_num_get(parser, 7, &hostname_verify) ||
				    (hostname_verify != 0 && hostname_verify != 1)) {
					return -EINVAL;
				}
				proxy.hostname_verify = (bool)hostname_verify;
			}
			proxy.family = (op == CLIENT_CONNECT) ? AF_INET : AF_INET6;
			err = do_udp_client_connect(url, port);
		} else if (op == CLIENT_DISCONNECT) {
			err = do_udp_proxy_close();
		} break;

	case AT_PARSER_CMD_TYPE_READ:
		rsp_send("\r\n#XUDPCLI: %d,%d\r\n", proxy.sock, proxy.family);
		err = 0;
		break;

	case AT_PARSER_CMD_TYPE_TEST:
		rsp_send("\r\n#XUDPCLI: (%d,%d,%d),<url>,<port>,<sec_tag>,"
			 "<use_dtls_cid>,<peer_verify>,<hostname_verify>\r\n",
			 CLIENT_DISCONNECT, CLIENT_CONNECT, CLIENT_CONNECT6);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

SLM_AT_CMD_CUSTOM(xudpsend, "AT#XUDPSEND", handle_at_udp_send);
static int handle_at_udp_send(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			      uint32_t param_count)
{
	int err = -EINVAL;
	char data[SLM_MAX_PAYLOAD_SIZE + 1] = {0};
	int size;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		if (proxy.sock == INVALID_SOCKET) {
			LOG_ERR("Not connected yet");
			return -ENOTCONN;
		}
		if (param_count > 1) {
			size = sizeof(data);
			err = util_string_get(parser, 1, data, &size);
			if (err) {
				return err;
			}
			err = do_udp_send(data, size);
		} else {
			err = enter_datamode(udp_datamode_callback);
		}
		break;

	default:
		break;
	}

	return err;
}

/**@brief API to initialize UDP Proxy AT commands handler
 */
int slm_at_udp_proxy_init(void)
{
	proxy.sock	= INVALID_SOCKET;
	proxy.sec_tag	= INVALID_SEC_TAG;
	proxy.efd	= INVALID_SOCKET;

	return 0;
}

/**@brief API to uninitialize UDP Proxy AT commands handler
 */
int slm_at_udp_proxy_uninit(void)
{
	return do_udp_proxy_close();
}

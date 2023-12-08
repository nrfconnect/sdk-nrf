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
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_udp_proxy.h"

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
	int dtls_cid;		/* DTLS connection identifier. */
	enum slm_udp_role role;	/* Client or Server proxy */
	union {			/* remote host */
		struct sockaddr_in remote;   /* IPv4 host */
		struct sockaddr_in6 remote6; /* IPv6 host */
	};
} proxy;

/** forward declaration of thread function **/
static void udp_thread_func(void *p1, void *p2, void *p3);

static int do_udp_server_start(uint16_t port)
{
	int ret;

	/* Open socket */
	ret = socket(proxy.family, SOCK_DGRAM, IPPROTO_UDP);
	if (ret < 0) {
		LOG_ERR("socket() failed: %d", -errno);
		return -errno;
	}

	proxy.sock = ret;
	/* Bind to local port */
	if (proxy.family == AF_INET) {
		char ipv4_addr[INET_ADDRSTRLEN];

		util_get_ip_addr(0, ipv4_addr, NULL);
		if (!*ipv4_addr) {
			LOG_ERR("Unable to obtain local IPv4 address");
			close(proxy.sock);
			return -EAGAIN;
		}

		struct sockaddr_in local = {
			.sin_family = AF_INET,
			.sin_port = htons(port)
		};

		if (inet_pton(AF_INET, ipv4_addr, &local.sin_addr) != 1) {
			LOG_ERR("Parse local IPv4 address failed: %d", -errno);
			close(proxy.sock);
			return -EINVAL;
		}
		ret = bind(proxy.sock, (struct sockaddr *)&local, sizeof(struct sockaddr_in));
	} else {
		char ipv6_addr[INET6_ADDRSTRLEN];

		util_get_ip_addr(0, NULL, ipv6_addr);
		if (!*ipv6_addr) {
			LOG_ERR("Unable to obtain local IPv6 address");
			close(proxy.sock);
			return -EAGAIN;
		}

		struct sockaddr_in6 local = {
			.sin6_family = AF_INET6,
			.sin6_port = htons(port)
		};

		if (inet_pton(AF_INET6, ipv6_addr, &local.sin6_addr) != 1) {
			LOG_ERR("Parse local IPv6 address failed: %d", -errno);
			close(proxy.sock);
			return -EINVAL;
		}
		ret = bind(proxy.sock, (struct sockaddr *)&local, sizeof(struct sockaddr_in6));
	}
	if (ret) {
		LOG_ERR("bind() failed: %d", -errno);
		close(proxy.sock);
		return -errno;
	}

	udp_thread_id = k_thread_create(&udp_thread, udp_thread_stack,
			K_THREAD_STACK_SIZEOF(udp_thread_stack),
			udp_thread_func, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);

	proxy.role = UDP_ROLE_SERVER;
	rsp_send("\r\n#XUDPSVR: %d,\"started\"\r\n", proxy.sock);

	return 0;
}

static int do_udp_server_stop(void)
{
	int ret = 0;

	if (proxy.sock == INVALID_SOCKET) {
		return 0;
	}
	ret = close(proxy.sock);
	if (ret < 0) {
		LOG_WRN("close() failed: %d", -errno);
		ret = -errno;
	} else {
		proxy.sock = INVALID_SOCKET;
		if (proxy.family == AF_INET) {
			memset(&proxy.remote, 0, sizeof(struct sockaddr_in));
		} else {
			memset(&proxy.remote6, 0, sizeof(struct sockaddr_in6));
		}
		slm_at_udp_proxy_init();
	}
	if (ret == 0 &&
	    k_thread_join(&udp_thread, K_SECONDS(CONFIG_SLM_UDP_POLL_TIME * 2)) != 0) {
		LOG_WRN("Wait for thread terminate failed");
	}
	rsp_send("\r\n#XUDPSVR: %d,\"stopped\"\r\n", ret);

	return ret;
}

static int do_udp_client_connect(const char *url, uint16_t port)
{
	int ret;
	struct sockaddr sa;
	struct timeval timeout;
	const bool using_cid = (proxy.dtls_cid != INVALID_DTLS_CID);
	const bool using_dtls = (proxy.sec_tag != INVALID_SEC_TAG);

	/* Open socket */
	ret = socket(proxy.family, SOCK_DGRAM, using_dtls ? IPPROTO_DTLS_1_2 : IPPROTO_UDP);
	if (ret < 0) {
		LOG_ERR("socket() failed: %d", -errno);
		return -errno;
	}
	proxy.sock = ret;

	/* Set a timeout shorter than the default one which makes the SLM
	 * irresponsive for too long when the connection to a server fails.
	 */
	timeout = (struct timeval){ .tv_sec = 10 };
	ret = setsockopt(proxy.sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
	if (ret) {
		ret = -errno;
		LOG_ERR("Setting timeout failed: %d", ret);
		goto cli_exit;
	}

	if (using_dtls) {
		sec_tag_t sec_tag_list[1] = { proxy.sec_tag };

		ret = setsockopt(proxy.sock, SOL_TLS, TLS_SEC_TAG_LIST,
				sec_tag_list, sizeof(sec_tag_t));
		if (ret) {
			LOG_ERR("set tag list failed: %d", -errno);
			ret = -errno;
			goto cli_exit;
		}

		if (using_cid) {
			if (setsockopt(proxy.sock, SOL_TLS, TLS_DTLS_CID,
							&proxy.dtls_cid, sizeof(proxy.dtls_cid))) {
				ret = -errno;
				LOG_WRN("Setting DTLS CID (%d) failed: %d", proxy.dtls_cid, ret);
				goto cli_exit;
			}
		}
	}

	/* Connect to remote host */
	ret = util_resolve_host(0, url, port, proxy.family, Z_LOG_OBJECT_PTR(slm_udp), &sa);
	if (ret) {
		goto cli_exit;
	}

	LOG_INF("Connecting to %s:%d (%s)...", url, port, using_dtls ? "DTLS" : "UDP");
	const size_t size = (sa.sa_family == AF_INET) ?
		sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);

	ret = connect(proxy.sock, &sa, size);
	if (ret < 0) {
		LOG_ERR("connect() failed: %d", -errno);
		ret = -errno;
		goto cli_exit;
	}
	LOG_INF("Connected.");

	if (using_cid) {
		int cid_status;
		socklen_t cid_status_size = sizeof(cid_status);

		/* Check the connection identifier status, and fail
		 * if it does not correspond to what was requested.
		 */
		ret = getsockopt(proxy.sock, SOL_TLS, TLS_DTLS_CID_STATUS,
							&cid_status, &cid_status_size);
		if (ret) {
			LOG_ERR("Getting DTLS CID status failed: %d", ret);
			ret = -errno;
			goto cli_exit;
		} else {
			if (proxy.dtls_cid == TLS_DTLS_CID_ENABLED
				&& !(cid_status == TLS_DTLS_CID_STATUS_BIDIRECTIONAL
					|| cid_status == TLS_DTLS_CID_STATUS_DOWNLINK)) {
				LOG_ERR("DTLS CID status (%d) not satisfactory for"
					" requested DTLS CID (%d).", cid_status, proxy.dtls_cid);
				ret = -ENOTSUP;
				goto cli_exit;
			} else {
				LOG_INF("DTLS CID status: %d", cid_status);
			}
		}
	}

	udp_thread_id = k_thread_create(&udp_thread, udp_thread_stack,
			K_THREAD_STACK_SIZEOF(udp_thread_stack),
			udp_thread_func, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);

	proxy.role = UDP_ROLE_CLIENT;
	rsp_send("\r\n#XUDPCLI: %d,\"connected\"\r\n", proxy.sock);

	return 0;

cli_exit:
	close(proxy.sock);
	proxy.sock = INVALID_SOCKET;
	rsp_send("\r\n#XUDPCLI: %d,\"not connected\"\r\n", ret);

	return ret;
}

static int do_udp_client_disconnect(void)
{
	int ret = 0;

	if (proxy.sock == INVALID_SOCKET) {
		return 0;
	}
	ret = close(proxy.sock);
	if (ret < 0) {
		LOG_WRN("close() failed: %d", -errno);
		ret = -errno;
	} else {
		proxy.sock = INVALID_SOCKET;
	}
	if (ret == 0 &&
	    k_thread_join(&udp_thread, K_SECONDS(CONFIG_SLM_UDP_POLL_TIME * 2)) != 0) {
		LOG_WRN("Wait for thread terminate failed");
	}
	rsp_send("\r\n#XUDPCLI: %d,\"disconnected\"\r\n", ret);

	return ret;
}

static int do_udp_send(const uint8_t *data, int datalen)
{
	int ret = 0;
	uint32_t offset = 0;

	while (offset < datalen) {
		if (proxy.role == UDP_ROLE_SERVER) {
			/* send to rememberd remote */
			if (proxy.family == AF_INET) {
				ret = sendto(proxy.sock, data + offset, datalen - offset, 0,
					(struct sockaddr *)&(proxy.remote),
					sizeof(struct sockaddr_in));
			} else {
				ret = sendto(proxy.sock, data + offset, datalen - offset, 0,
					(struct sockaddr *)&(proxy.remote6),
					sizeof(struct sockaddr_in6));
			}
		} else {
			ret = send(proxy.sock, data + offset, datalen - offset, 0);
		}
		if (ret < 0) {
			LOG_ERR("send()/sendto() failed: %d, sent: %d", -errno, offset);
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
			/* send to rememberd remote */
			if (proxy.family == AF_INET) {
				ret = sendto(proxy.sock, data + offset, datalen - offset, 0,
					(struct sockaddr *)&(proxy.remote),
					sizeof(struct sockaddr_in));
			} else {
				ret = sendto(proxy.sock, data + offset, datalen - offset, 0,
					(struct sockaddr *)&(proxy.remote6),
					sizeof(struct sockaddr_in6));
			}
		} else {
			ret = send(proxy.sock, data + offset, datalen - offset, 0);
		}
		if (ret < 0) {
			LOG_ERR("send()/sendto() failed: %d, sent: %d", -errno, offset);
			break;
		} else {
			offset += ret;
		}
	}

	return (offset > 0) ? offset : -1;
}

static void udp_thread_func(void *p1, void *p2, void *p3)
{
	int ret;
	struct pollfd fds;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	fds.fd = proxy.sock;
	fds.events = POLLIN;

	do {
		ret = poll(&fds, 1, MSEC_PER_SEC * CONFIG_SLM_UDP_POLL_TIME);
		if (ret < 0) {  /* IO error */
			LOG_WRN("poll() error: %d", ret);
			continue;
		}
		if (ret == 0) {  /* timeout */
			continue;
		}
		LOG_DBG("Poll events 0x%08x", fds.revents);
		if ((fds.revents & POLLERR) == POLLERR) {
			LOG_WRN("POLLERR");
			ret = -EIO;
			break;
		}
		if ((fds.revents & POLLNVAL) == POLLNVAL) {
			/* UDP client or server closed */
			LOG_WRN("POLLNVAL");
			ret = -ENETDOWN;
			break;
		}
		if ((fds.revents & POLLHUP) == POLLHUP) {
			/* Lose LTE connection */
			LOG_WRN("POLLHUP");
			ret = -ECONNRESET;
			break;
		}
		if ((fds.revents & POLLIN) != POLLIN) {
			continue;
		}

		if (proxy.role == UDP_ROLE_SERVER) {
			/* remember remote from last recvfrom */
			if (proxy.family == AF_INET) {
				int size = sizeof(struct sockaddr_in);

				memset(&proxy.remote, 0, sizeof(struct sockaddr_in));
				ret = recvfrom(
					proxy.sock, (void *)slm_data_buf, sizeof(slm_data_buf),
					0, (struct sockaddr *)&(proxy.remote), &size);
			} else {
				int size = sizeof(struct sockaddr_in6);

				memset(&proxy.remote6, 0, sizeof(struct sockaddr_in6));
				ret = recvfrom(
					proxy.sock, (void *)slm_data_buf, sizeof(slm_data_buf),
					0, (struct sockaddr *)&(proxy.remote6), &size);
			}
		} else {
			ret = recv(proxy.sock, (void *)slm_data_buf, sizeof(slm_data_buf), 0);
		}
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
			rsp_send("\r\n#XUDPDATA: %d\r\n", ret);
			data_send(slm_data_buf, ret);
		}
	} while (true);

	if (in_datamode()) {
		exit_datamode_handler(ret);
	}
	if (proxy.sock != INVALID_SOCKET) {
		close(proxy.sock);
		proxy.sock = INVALID_SOCKET;
		if (proxy.role == UDP_ROLE_CLIENT) {
			rsp_send("\r\n#XUDPCLI: %d,\"disconnected\"\r\n", ret);
		} else {
			rsp_send("\r\n#XUDPSVR: %d,\"stopped\"\r\n", ret);
		}
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
		LOG_INF("datamode send: %d", ret);
	} else if (op == DATAMODE_EXIT) {
		LOG_DBG("datamode exit");
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

/* Handles AT#XUDPSVR commands. */
int handle_at_udp_server(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	uint16_t port;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&slm_at_param_list, 1, &op);
		if (err) {
			return err;
		}
		if (op == SERVER_START || op == SERVER_START6) {
			if (socket_is_in_use()) {
				return -EINVAL;
			}
			err = at_params_unsigned_short_get(&slm_at_param_list, 2, &port);
			if (err) {
				return err;
			}
			proxy.family = (op == SERVER_START) ? AF_INET : AF_INET6;
			err = do_udp_server_start(port);
		} else if (op == SERVER_STOP) {
			err = do_udp_server_stop();
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		rsp_send("\r\n#XUDPSVR: %d,%d\r\n", proxy.sock, proxy.family);
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XUDPSVR: (%d,%d,%d),<port>,<sec_tag>\r\n",
			SERVER_STOP, SERVER_START, SERVER_START6);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/* Handles AT#XUDPCLI commands. */
int handle_at_udp_client(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;

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

			if (socket_is_in_use()) {
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
			proxy.dtls_cid = INVALID_DTLS_CID;
			const uint32_t param_count = at_params_valid_count_get(&slm_at_param_list);

			if (param_count > 4) {
				if (at_params_int_get(&slm_at_param_list, 4, &proxy.sec_tag)
				|| proxy.sec_tag == INVALID_SEC_TAG || proxy.sec_tag < 0) {
					return -EINVAL;
				}
				if (param_count > 5) {
					if (at_params_int_get(
						&slm_at_param_list, 5, &proxy.dtls_cid)
					|| !(proxy.dtls_cid == TLS_DTLS_CID_DISABLED
						|| proxy.dtls_cid == TLS_DTLS_CID_SUPPORTED
						|| proxy.dtls_cid == TLS_DTLS_CID_ENABLED)) {
						return -EINVAL;
					}
				}
			}
			proxy.family = (op == CLIENT_CONNECT) ? AF_INET : AF_INET6;
			err = do_udp_client_connect(url, port);
		} else if (op == CLIENT_DISCONNECT) {
			err = do_udp_client_disconnect();
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		rsp_send("\r\n#XUDPCLI: %d,%d\r\n", proxy.sock, proxy.family);
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XUDPCLI: (%d,%d,%d),<url>,<port>,<sec_tag>,<use_dtls_cid>\r\n",
			CLIENT_DISCONNECT, CLIENT_CONNECT, CLIENT_CONNECT6);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/* Handles AT#XUDPSEND command. */
int handle_at_udp_send(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char data[SLM_MAX_PAYLOAD_SIZE + 1] = {0};
	int size;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (proxy.sock == INVALID_SOCKET) {
			LOG_ERR("Not connected yet");
			return -ENOTCONN;
		}
		if (at_params_valid_count_get(&slm_at_param_list) > 1) {
			size = sizeof(data);
			err = util_string_get(&slm_at_param_list, 1, data, &size);
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
	proxy.sock     = INVALID_SOCKET;
	proxy.sec_tag  = INVALID_SEC_TAG;

	return 0;
}

/**@brief API to uninitialize UDP Proxy AT commands handler
 */
int slm_at_udp_proxy_uninit(void)
{
	int ret = 0;

	if (proxy.sock != INVALID_SOCKET) {
		k_thread_abort(udp_thread_id);
		ret = close(proxy.sock);
		if (ret < 0) {
			LOG_WRN("close() failed: %d", -errno);
			ret = -errno;
		}
		proxy.sock = INVALID_SOCKET;
	}

	return ret;
}

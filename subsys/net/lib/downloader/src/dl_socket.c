/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net/socket.h>
#include <zephyr/net/socket_ncs.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/net_log.h>
#include <stdio.h>
#include <stdbool.h>

#include "dl_socket.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(downloader, CONFIG_DOWNLOADER_LOG_LEVEL);

/* Delay before retrying when all addresses failed, within the connection timeout */
#define RETRY_DELAY K_SECONDS(5)

#define MS_TO_TIMEVAL(ms) \
	.tv_sec = (ms / 1000), \
	.tv_usec = (ms % 1000) * 1000 \

static const char *str_family(int family)
{
	switch (family) {
	case NET_AF_UNSPEC:
		return "Unspec";
	case NET_AF_INET:
		return "IPv4";
	case NET_AF_INET6:
		return "IPv6";
	default:
		__ASSERT(false, "Unsupported family");
		return "Unknown";
	}
}

static int socket_sectag_set(int fd, const int *const sec_tag_list, uint8_t sec_tag_count)
{
	int err;
	int verify;

	verify = ZSOCK_TLS_PEER_VERIFY_REQUIRED;

	err = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY, &verify,
			       sizeof(verify));
	if (err) {
		LOG_ERR("Failed to setup peer verification, errno %d", errno);
		return -errno;
	}

	LOG_INF("Setting up TLS credentials, sec tag count %u", sec_tag_count);
	err = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST, sec_tag_list,
			       sizeof(sec_tag_t) * sec_tag_count);
	if (err) {
		LOG_ERR("Failed to setup socket security tag list, errno %d", errno);
		return -errno;
	}

	return 0;
}

static int socket_ZSOCK_TLS_HOSTNAME_set(int fd, const char *const hostname)
{
	__ASSERT_NO_MSG(hostname);

	int err;

	err = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_HOSTNAME, hostname,
			       strlen(hostname));
	if (err) {
		LOG_ERR("Failed to setup TLS hostname (%s), errno %d", hostname, errno);
		return -errno;
	}

	return 0;
}

static int socket_pdn_id_set(int fd, uint32_t pdn_id)
{
	int err;

	LOG_INF("Binding to PDN ID: %d", pdn_id);
	err = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, SO_BINDTOPDN, &pdn_id,
			       sizeof(pdn_id));
	if (err) {
		LOG_ERR("Failed to bind socket to PDN ID %d, err %d", pdn_id, errno);
		return -ENETDOWN;
	}

	return 0;
}

static int socket_dtls_cid_enable(int fd)
{
	int err;
	uint32_t dtls_cid = ZSOCK_TLS_DTLS_CID_ENABLED;

	err = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_DTLS_CID, &dtls_cid,
			       sizeof(dtls_cid));
	if (err) {
		err = -errno;
		LOG_ERR("Failed to enable ZSOCK_TLS_DTLS_CID: %d", err);
		/* Not fatal, so continue */
	}

	return err;
}

static int socket_tls_native_enable(int fd)
{
	int err;
	int tls_native = 1;

	err = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_NATIVE, &tls_native,
			       sizeof(tls_native));
	if (err < 0) {
		err = -errno;
		LOG_ERR("Failed to set native TLS: %d", err);
		return err;
	}

	LOG_DBG("Enabled native TLS");

	return 0;
}

static int socket_if_name_set(int fd, const char *if_name)
{
	int err;
	struct net_ifreq ifname = { 0 };

	strncpy(ifname.ifr_name, if_name, sizeof(ifname.ifr_name) - 1);
	ifname.ifr_name[sizeof(ifname.ifr_name) - 1] = '\0';

	err = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE, &ifname,
			       sizeof(struct net_ifreq));
	if (err < 0) {
		err = -errno;
		NET_ERR("Failed to bind to interface %s: %d", ifname.ifr_name, err);
		return err;
	}

	NET_DBG("Bound to interface %s", ifname.ifr_name);

	return 0;
}

static bool is_ip_address(const char *hostname)
{
	struct net_sockaddr sa;

	if (zsock_inet_pton(NET_AF_INET, hostname, sa.data) == 1) {
		return true;
	} else if (zsock_inet_pton(NET_AF_INET6, hostname, sa.data) == 1) {
		return true;
	}

	return false;
}

/* connect_by_name callback, called for each resolved address */
static int socket_create_and_connect(const struct net_sockaddr *addr, net_socklen_t addrlen,
				     void *user_data)
{
	struct downloader *dl = user_data;
	struct downloader_host_cfg *dl_host_cfg = &dl->host_cfg;
	int proto = dl->conn.proto;
	int fd;
	int err;

	LOG_DBG("family: %s, type: %d, proto: %d", str_family(addr->sa_family), dl->conn.type,
		proto);

	fd = zsock_socket(addr->sa_family, dl->conn.type, proto);
	if (fd < 0) {
		err = -errno;
		LOG_ERR("Failed to create socket, errno %d", -err);
		return err;
	}

	LOG_DBG("Socket opened, fd %d", fd);

	if (IS_ENABLED(CONFIG_NET_SOCKETS_OFFLOAD_DISPATCHER) && dl_host_cfg->set_native_tls) {
		err = socket_tls_native_enable(fd);
		if (err < 0) {
			goto cleanup;
		}
	}

	if (dl_host_cfg->if_name != NULL) {
		err = socket_if_name_set(fd, dl_host_cfg->if_name);
		if (err < 0) {
			goto cleanup;
		}
	}

	if (dl_host_cfg->pdn_id) {
		err = socket_pdn_id_set(fd, dl_host_cfg->pdn_id);
		if (err) {
			goto cleanup;
		}
	}

	if ((proto == NET_IPPROTO_TLS_1_2 || proto == NET_IPPROTO_DTLS_1_2) &&
	    (dl_host_cfg->sec_tag_list != NULL) && (dl_host_cfg->sec_tag_count > 0)) {
		err = socket_sectag_set(fd, dl_host_cfg->sec_tag_list, dl_host_cfg->sec_tag_count);
		if (err) {
			goto cleanup;
		}

		if (!is_ip_address(dl->hostname)) {
			err = socket_ZSOCK_TLS_HOSTNAME_set(fd, dl->hostname);
			if (err) {
				goto cleanup;
			}
		}

		if (proto == NET_IPPROTO_DTLS_1_2 && dl_host_cfg->cid) {
			LOG_DBG("enabling CID");
			err = socket_dtls_cid_enable(fd);
			if (err) {
				goto cleanup;
			}
		}
	}

	err = zsock_connect(fd, addr, addrlen);
	if (err) {
		err = -errno;
		/* Make sure that ECONNRESET is not returned as it has a special meaning
		 * in the downloader API
		 */
		if (err == -ECONNRESET) {
			err = -ECONNREFUSED;
		}
	}

cleanup:
	if (err) {
		dl_socket_close(&fd);
		return err;
	}

	return fd;
}

static int conn_name_init(struct downloader *dl, int proto, int type, uint16_t port)
{
	struct connect_by_name_cfg *cfg = &dl->conn.cfg;
	int err;

	*cfg = (struct connect_by_name_cfg){
		.host = dl->hostname,
		.port = port,
		.socktype = type & ~SOCK_NATIVE_TLS,
		.family = dl->host_cfg.family,
		.connect_cb = socket_create_and_connect,
		.user_data = dl,
	};

	if (dl->host_cfg.pdn_id) {
		(void)snprintf(dl->conn.pdn_serv, sizeof(dl->conn.pdn_serv), "%d",
			       dl->host_cfg.pdn_id);
		cfg->service = dl->conn.pdn_serv;
		cfg->ai_flags = AI_PDNSERV;
	}

	dl->conn.proto = proto;
	dl->conn.type = type;
	err = connect_by_name_init(&dl->conn.ctx, cfg);
	dl->conn.initialized = (err == 0);

	return err;
}

/* Called with dl->mutex held */
static bool stopping(struct downloader *dl)
{
	return dl->state == DOWNLOADER_STOPPING || dl->state == DOWNLOADER_DEINITIALIZING;
}

int dl_socket_connect(struct downloader *dl, int *fd, int proto, int type, uint16_t port)
{
	k_timepoint_t deadline = sys_timepoint_calc(K_SECONDS(CONFIG_DOWNLOADER_CONNECT_TIMEOUT));
	k_timepoint_t retry;
	int ret;

	while (true) {
		k_mutex_lock(&dl->mutex, K_FOREVER);
		if (stopping(dl)) {
			k_mutex_unlock(&dl->mutex);
			return -ECANCELED;
		}
		ret = dl->conn.initialized ? 0 : conn_name_init(dl, proto, type, port);
		k_mutex_unlock(&dl->mutex);
		if (ret) {
			LOG_ERR("Invalid connection parameters, err %d", ret);
			return ret;
		}

		ret = connect_by_name(&dl->conn.ctx);
		if (ret >= 0 || sys_timepoint_expired(deadline)) {
			break;
		}

		/* Retry on connectivity change or after the delay; also bounds cancel latency */
		retry = sys_timepoint_calc(RETRY_DELAY);
		if (sys_timepoint_cmp(deadline, retry) < 0) {
			retry = deadline;
		}
		connect_by_name_wait_change(&dl->conn.ctx, sys_timepoint_timeout(retry));
	}
	if (ret < 0) {
		LOG_ERR("Failed to connect to %s, err %d", dl->hostname, ret);
		return ret;
	}

	*fd = ret;

	return 0;
}

void dl_socket_host_reset(struct downloader *dl)
{
	if (dl->conn.initialized) {
		connect_by_name_deinit(&dl->conn.ctx);
		dl->conn.initialized = false;
	}
}

void dl_socket_failed(struct downloader *dl)
{
	if (dl->conn.initialized) {
		connect_by_name_failed(&dl->conn.ctx);
	}
}

const struct net_sockaddr *dl_socket_peer(struct downloader *dl)
{
	if (!dl->conn.initialized) {
		return NULL;
	}

	return connect_by_name_get_peer(&dl->conn.ctx, NULL);
}

int dl_socket_close(int *fd)
{
	int err = 0;

	if (*fd >= 0) {
		err = zsock_close(*fd);
		if (err && errno != EBADF) {
			err = -errno;
			LOG_ERR("Failed to close socket, errno %d", -err);
		}

		LOG_DBG("Socket closed, fd %d", *fd);
		*fd = -1;
	}

	return err;
}

int dl_socket_send(int fd, void *buf, size_t len)
{
	int sent;
	size_t off = 0;

	while (len) {
		sent = zsock_send(fd, (uint8_t *)buf + off, len, 0);
		if (sent < 0) {
			return -errno;
		}

		off += sent;
		len -= sent;
	}

	return 0;
}

int dl_socket_send_timeout_set(int fd, uint32_t timeout_ms)
{
	int err;
	struct timeval timeo = {
		MS_TO_TIMEVAL(timeout_ms),
	};

	if (timeout_ms <= 0) {
		return 0;
	}

	err = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_SNDTIMEO, &timeo, sizeof(timeo));
	if (err) {
		LOG_WRN("Failed to set socket timeout, errno %d", errno);
		return -errno;
	}

	return 0;
}

int dl_socket_recv_timeout_set(int fd, uint32_t timeout_ms)
{
	int err;
	struct timeval timeo = {
		MS_TO_TIMEVAL(timeout_ms),
	};

	if (fd == -1) {
		return -EINVAL;
	}

	err = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &timeo, sizeof(timeo));
	if (err) {
		LOG_WRN("Failed to set socket timeout, errno %d", errno);
		return -errno;
	}

	return 0;
}

ssize_t dl_socket_recv(int fd, void *buf, size_t len)
{
	int err = 0;

	if (fd == -1) {
		return -EINVAL;
	}

	err = zsock_recv(fd, buf, len, 0);
	if (err < 0) {
		return -errno;
	}

	return err;
}
